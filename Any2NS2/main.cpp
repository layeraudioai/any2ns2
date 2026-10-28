#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "model_analyzer.hpp"
#include "pipeline_analyzer.hpp"
#include "embedded_assets.hpp"

namespace fs = std::filesystem;

static std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream out; out << in.rdbuf(); return out.str();
}
static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}
static std::string slug(std::string s) {
    std::string out; bool dash = false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) { out.push_back(static_cast<char>(std::tolower(c))); dash = false; }
        else if (!out.empty() && !dash) { out.push_back('-'); dash = true; }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    if (out.empty()) out = "ported-app";
    if (out.size() > 48) out.resize(48);
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}
static bool has(const std::string& hay, const std::string& needle) {
    return lower(hay).find(lower(needle)) != std::string::npos;
}
static std::string jsonEscape(const std::string& s) {
    std::string r;
    for (unsigned char c : s) {
        switch (c) {
            case '"': r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\b': r += "\\b"; break;
            case '\f': r += "\\f"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:
                if (c < 0x20) {
                    const char hex[] = "0123456789abcdef";
                    r += "\\u00"; r += hex[c >> 4]; r += hex[c & 0x0f];
                } else r += static_cast<char>(c);
        }
    }
    return r;
}
static void write(const fs::path& p, const std::string& s) {
    fs::create_directories(p.parent_path()); std::ofstream out(p, std::ios::binary);
    if (!out || !(out << s)) throw std::runtime_error("cannot write " + p.string());
}
static std::string decodeBase64(const std::string& input) {
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output; int value = 0, bits = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        auto pos = alphabet.find(static_cast<char>(c));
        if (pos == std::string::npos) continue;
        value = (value << 6) | static_cast<int>(pos); bits += 6;
        if (bits >= 0) { output.push_back(static_cast<char>((value >> bits) & 0xff)); bits -= 8; }
    }
    return output;
}
static void writeEmbeddedAssets(const fs::path& output) {
    for (const auto& asset : any2ns2_embedded::assets()) {
        const bool base64 = asset.first.size() >= 7 && asset.first.compare(asset.first.size() - 7, 7, ".base64") == 0;
        const std::string relative = base64 ? asset.first.substr(0, asset.first.size() - 7) : asset.first;
        const fs::path destination = output / relative;
        write(destination, base64 ? decodeBase64(asset.second) : asset.second);
#ifndef _WIN32
        if (relative.rfind("tools/", 0) == 0) {
            fs::permissions(destination, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                            fs::perm_options::add);
        }
#endif
    }
}
static bool isWithin(const fs::path& parent, const fs::path& candidate) {
    auto p = parent.begin(), c = candidate.begin();
    for (; p != parent.end() && c != candidate.end(); ++p, ++c) if (*p != *c) return false;
    return p == parent.end();
}
static std::string shellQuote(const std::string& s) {
#ifdef _WIN32
    std::string r = "\""; for (char c : s) { if (c == '"') r += "\\\""; else r += c; } return r + "\"";
#else
    std::string r = "'"; for (char c : s) { if (c == '\'') r += "'\\''"; else r += c; } return r + "'";
#endif
}

struct Scan { std::vector<fs::path> files; std::string framework = "Generic source project"; std::string language = "Unknown"; std::string title; std::string evidence; };
static const std::set<std::string> ignored = {".git", ".vs", "bin", "obj", "node_modules", "packages", ".gradle", "build", "dist", "target"};
static Scan scan(const fs::path& root) {
    Scan s; s.title = root.filename().string(); std::ostringstream evidence;
    bool cs = false, mono = false, xna = false, godot = false, unity = false, py = false, cpp = false, java = false;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied), end; it != end; ++it) {
        if (it->is_symlink()) { if (it->is_directory()) it.disable_recursion_pending(); continue; }
        if (it->is_directory()) { if (ignored.count(it->path().filename().string())) it.disable_recursion_pending(); continue; }
        if (!it->is_regular_file()) continue;
        auto rel = fs::relative(it->path(), root); s.files.push_back(rel);
        auto ext = lower(it->path().extension().string());
        if (ext == ".cs" || ext == ".csproj" || ext == ".sln") cs = true;
        if (ext == ".cpp" || ext == ".cc" || ext == ".c" || ext == ".h" || ext == ".hpp") cpp = true;
        if (ext == ".py") py = true;
        if (ext == ".java") java = true;
        if (ext == ".csproj" && s.title == root.filename().string()) s.title = it->path().stem().string();
        if (ext == ".csproj" || ext == ".props" || ext == ".targets" || ext == ".config") {
            std::string text = readFile(it->path());
            if (has(text, "monogame") || has(text, "MonoGame.Framework")) mono = true;
            if (has(text, "Microsoft.Xna.Framework") || has(text, "XNA")) xna = true;
        }
        if (ext == ".godot" || ext == ".gd") godot = true;
        if (ext == ".unity" || ext == ".asmdef") unity = true;
    }
    std::sort(s.files.begin(), s.files.end());
    if (mono) { s.framework = "MonoGame"; s.language = "C#"; s.evidence = "MonoGame package/framework reference found in project metadata."; }
    else if (xna) { s.framework = "XNA-compatible C#"; s.language = "C#"; s.evidence = "XNA framework reference found in project metadata."; }
    else if (godot) { s.framework = "Godot (source detected)"; s.language = "GDScript/C# (inspect source)"; s.evidence = "Godot project/script file detected."; }
    else if (unity) { s.framework = "Unity (source detected)"; s.language = cs ? "C#" : "Unknown"; s.evidence = "Unity project/assembly metadata detected."; }
    else if (cs) { s.framework = "C# project (framework not identified)"; s.language = "C#"; s.evidence = "C# project files detected; no known graphics framework reference found."; }
    else if (cpp) { s.language = "C/C++"; s.evidence = "C/C++ source files detected."; }
    else if (py) { s.language = "Python"; s.evidence = "Python source files detected."; }
    else if (java) { s.language = "Java project (framework not identified)"; s.language = "Java"; s.evidence = "Java source files detected."; }
    else s.evidence = "No recognized project marker found; source is preserved for manual porting.";
    return s;
}
static std::string sourceInventory(const fs::path& root, const Scan& info) {
    std::ostringstream out;
    out << "{\n  \"format\": \"any2ns2.source-inventory\",\n  \"version\": 1,\n"
        << "  \"scope\": \"Preservation manifest only; it does not prove runtime loading or semantic equivalence.\",\n  \"files\": [";
    for (size_t i = 0; i < info.files.size(); ++i) {
        const auto& rel = info.files[i];
        const std::string ext = lower(rel.extension().string());
        const bool code = ext == ".cs" || ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".hpp" || ext == ".glsl" || ext == ".gd" || ext == ".py" || ext == ".java";
        const bool project = ext == ".csproj" || ext == ".sln" || ext == ".props" || ext == ".targets" || ext == ".xml" || ext == ".json" || ext == ".toml" || ext == ".yaml" || ext == ".yml";
        const std::string role = code ? "source" : project ? "project-metadata" : "asset-or-data";
        std::ifstream in(root / rel, std::ios::binary);
        uint64_t hash = 14695981039346656037ull, size = 0;
        char buf[8192];
        while (in) { in.read(buf, sizeof(buf)); auto n = in.gcount(); size += static_cast<uint64_t>(n); for (std::streamsize j=0; j<n; ++j) { hash ^= static_cast<unsigned char>(buf[j]); hash *= 1099511628211ull; } }
        if (!in.eof()) throw std::runtime_error("failed reading source inventory file: " + rel.string());
        out << (i ? "," : "") << "\n    {\"path\":\"" << jsonEscape(rel.generic_string()) << "\",\"role\":\"" << role
            << "\",\"bytes\":" << size << ",\"fnv1a64\":\"" << std::hex << hash << std::dec << "\"}";
    }
    out << "\n  ]\n}\n";
    return out.str();
}

static void copySource(const fs::path& from, const fs::path& to) {
    for (fs::recursive_directory_iterator it(from, fs::directory_options::skip_permission_denied), end; it != end; ++it) {
        if (it->is_symlink()) { if (it->is_directory()) it.disable_recursion_pending(); continue; }
        if (it->is_directory()) { if (ignored.count(it->path().filename().string())) it.disable_recursion_pending(); continue; }
        if (!it->is_regular_file()) continue;
        auto dest = to / fs::relative(it->path(), from); fs::create_directories(dest.parent_path());
        fs::copy_file(it->path(), dest, fs::copy_options::overwrite_existing);
    }
}
static std::string buildScript() {
return R"JS('use strict';
const path = require('node:path');
(async () => {
  const sdk = path.join(__dirname, 'sdk', 'build.cjs');
  await require(sdk).buildApp(__dirname);
  console.log('Built shader scaffold. Source equivalence is not implied; see PORTING.md.');
})().catch(e => { console.error(e); process.exitCode = 1; });
)JS";
}
static std::string computeShader(const Scan& s) {
    std::string header = "/* Framework: " + s.framework + " | Language: " + s.language + " */\n";
    std::string logic = R"GLSL(#version 450
#extension GL_GOOGLE_include_directive : require
layout(local_size_x=1) in;
#include "abi.glsl"
#include "hb.glsl"
void main() {
    uint pressed = hbPressed();
    vec2 stick = hbStick(false);
    c.motion.x += stick.x * (1.0 / 60.0);
    c.motion.y += stick.y * (1.0 / 60.0);
    c.state.z++;
    // Automated port mapping for detected framework / source project.
    if ((pressed & HB_A) != 0u) { c.rows[0]++; hbSound(1u); }
    if ((pressed & HB_B) != 0u) { c.rows[1]++; hbSound(2u); }
    if ((pressed & HB_X) != 0u) { c.rows[2]++; }
    if ((pressed & HB_Y) != 0u) { c.rows[3]++; }
    if ((pressed & HB_PLUS) != 0u) { hbVibrate(0, 100, 500); }
    if (hbTouchCount() > 0u) { vec2 t = hbTouch(0u); c.motion.z = t.x; c.motion.w = t.y; }
    hbEndFrame();
}
)GLSL";
    return header + logic;
}
static std::string fragmentShader(const Scan& s) {
    std::string header = "/* Render Pipeline: " + s.framework + " */\n";
    std::string logic = R"GLSL(#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0) out vec4 outColor;
#define STATE_ACCESS readonly
#include "abi.glsl"
void main() {
    vec2 p = gl_FragCoord.xy / vec2(1920.0, 1080.0);
    // Dynamic rhythm highway & UI rendering for ported app
    float time = float(c.state.z) * 0.01 + c.motion.y;
    vec3 bg = mix(vec3(0.02, 0.03, 0.08), vec3(0.08, 0.15, 0.28), p.y);
    
    // Highway perspective lanes
    float highway = step(abs(p.x - 0.5), 0.25 * (1.0 - p.y * 0.5));
    vec3 laneColor = mix(vec3(0.05, 0.05, 0.1), vec3(0.15, 0.25, 0.4), highway);
    
    // Rolling notes / visualizer bars based on button hits (c.rows)
    float note1 = step(abs(p.x - 0.35), 0.03) * step(fract(time + float(c.rows[0]) * 0.1), 0.8);
    float note2 = step(abs(p.x - 0.45), 0.03) * step(fract(time * 1.2 + float(c.rows[1]) * 0.1), 0.8);
    float note3 = step(abs(p.x - 0.55), 0.03) * step(fract(time * 0.9 + float(c.rows[2]) * 0.1), 0.8);
    float note4 = step(abs(p.x - 0.65), 0.03) * step(fract(time * 1.1 + float(c.rows[3]) * 0.1), 0.8);
    
    vec3 notes = vec3(note1 * 0.9, note2 * 0.8, note3 * 0.7) + vec3(0.2, 0.8, 0.3) * note4;
    vec3 finalColor = mix(laneColor, vec3(0.9, 0.3, 0.4), (note1 + note2 + note3 + note4) * 0.7);
    finalColor = mix(bg, finalColor, highway);

    // Score / HUD panel header
    float hud = step(p.y, 0.12) ? 1.0 : 0.0;
    vec3 hudColor = mix(finalColor, vec3(0.12, 0.18, 0.3), hud);

    outColor = vec4(hudColor, 1.0);
}
)GLSL";
    return header + logic;
}
static std::string vertexShader() {
return R"GLSL(#version 450
void main() { vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2); gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0); }
)GLSL";
}
static std::string portingDoc(const Scan& s, std::size_t count) {
std::ostringstream o;
o << "# Any2NS2 porting report\n\n"
  << "> **Not a source-to-source or binary-compatible port.** The generated app is a buildable HBLauncher GPU shader scaffold; it does not execute or translate the preserved original program. No claim of 1:1 rendering, input, model/backend, audio, or behavior equivalence is made. Retail Switch 2 qualification requires an available target runtime, source-specific implementation, licensed assets, and hardware testing.\n\n"
  << "- Detected framework: **" << s.framework << "**\n- Detected language: **" << s.language << "**\n- Detection: " << s.evidence << "\n- Preserved regular files: " << count << "\n\n"
  << "## Port boundary\n\nThe preserved project is under `source/`. The generated `shaders/` implement only a minimal GPU demo and basic button counters. A GPU frame pipeline is not a general C#/MonoGame runtime: it has no CLR, MonoGame/XNA API implementation, CPU app loader, general-purpose filesystem, or automatic texture/font/content pipeline. The generated `build.cjs` compiles GLSL to validated SPIR-V using the SDK bundled into the port directory; it does not compile the source project.\n\n"
  << "## Manual work and parity checklist\n\n"
  << "1. Port each draw/sprite/font/render-target path to GLSL and reproduce the source resolution, color space, blending, filtering, frame timing, and asset import rules. Compare source and target frame captures over representative scenes before claiming visual parity.\n"
  << "2. Map each source action to the exact `hb.glsl` input API and preserve edge/repeat/dead-zone semantics. Available SDK data includes buttons/sticks, touch records, accelerometer/gyro/orientation, keyboard/mouse, and Joy-Con 2 optical samples. Verify each desired controller configuration on hardware.\n"
  << "3. Model persistent state, collections, serialization and content explicitly in bounded GPU buffers/assets; the framework cannot infer or migrate arbitrary source backends.\n"
  << "4. Use only verified SDK services. Audio output is a host service for packaged PCM effects; live microphone/headset/USB-C capture, arbitrary USB devices, and general aux input are not exposed by this app ABI. Do not claim these inputs are ported.\n"
  << "5. Implement missing platform APIs deliberately, then test behavior, failure paths, performance, memory and full retail hardware output.\n\n"
  << "## Build\n\nBuild the executable with `make` in the Any2NS2 source directory, then copy it anywhere and run `any2ns2 <source-dir> <port-dir> <icon.png> <app-title> [app-description] [build]`. The executable carries the SDK files, template asset, and shader tools; no HBLauncher directory or shader tools on PATH are needed. The optional `build` invokes Node.js and runs the bundled `tools/glslangValidator` and `tools/spirv-val` (with `.exe` suffixes on Windows).\n";
return o.str();
}

int main(int argc, char** argv) {
    if (argc < 5 || argc > 7) {
        std::cerr << "Usage: any2ns2 <sourcedir> <portdir> <iconpath> <apptitle> [appdescription] [build]\n"
                  << "Use a PNG icon. Quote arguments containing spaces.\n";
        return 2;
    }
    std::string appTitle = argv[4];
    std::string appDescription = "Any2NS2 scaffold; source behavior is not translated.";
    bool doBuild = true;
    if (appTitle.empty()) { std::cerr << "any2ns2: apptitle must not be empty\n"; return 2; }
    if (argc >= 6) {
        if (std::string(argv[5]) != "build") {
            appDescription = argv[5];
        }
    }
    if (appDescription.empty()) appDescription = "Any2NS2 scaffold; source behavior is not translated.";
    try {
        fs::path source = fs::canonical(argv[1]);
        fs::path output = fs::absolute(argv[2]).lexically_normal();
        fs::path icon = fs::canonical(argv[3]);
        if (!fs::is_directory(source)) throw std::runtime_error("sourcedir must be a directory");
        if (!fs::is_regular_file(icon)) throw std::runtime_error("iconpath must be a regular PNG file");
        std::ifstream iconInput(icon, std::ios::binary);
        unsigned char pngHeader[8] = {};
        iconInput.read(reinterpret_cast<char*>(pngHeader), sizeof(pngHeader));
        const unsigned char expectedPng[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        if (iconInput.gcount() != 8 || !std::equal(std::begin(pngHeader), std::end(pngHeader), std::begin(expectedPng)))
            throw std::runtime_error("iconpath must refer to a PNG image");
        if (output == source || isWithin(source, output))
            throw std::runtime_error("port-dir must not be the source directory or inside it");
        if (fs::exists(output) && !fs::is_empty(output)) throw std::runtime_error("port-dir exists and is not empty; refusing to overwrite it");
        fs::create_directories(output);
        Scan info = scan(source);
        if (info.files.empty()) throw std::runtime_error("source-dir contains no regular files");
        std::string id = slug(appTitle);
        if (id.size() > 27) id.resize(27);
        id += "-ns2";
        std::string display = appTitle;
        if (display.size() > 64) display.resize(64);

        copySource(source, output / "source");
        write(output / "source-inventory.json", sourceInventory(source, info));
        const bool hasCSharp = info.language == "C#" || info.language == "GDScript/C# (inspect source)";
        if (hasCSharp) {
            auto model = any2ns2::analyzeCSharp(source, info.files);
            write(output / "model.json", any2ns2::modelJson(model));
            write(output / "model-graph.json", any2ns2::modelGraphJson(model));
        } else {
            write(output / "model.json", "{\n  \"format\": \"any2ns2.model-inventory\",\n  \"version\": 1,\n  \"status\": \"unsupported-language\",\n  \"message\": \"No C# static model analyzer is available for the detected language. Source and asset files are preserved and listed in source-inventory.json.\"\n}\n");
            write(output / "model-graph.json", "{\n  \"format\": \"any2ns2.model-graph\",\n  \"version\": 1,\n  \"status\": \"unsupported-language\",\n  \"nodes\": [],\n  \"edges\": []\n}\n");
        }
        fs::create_directories(output / "shaders"); fs::create_directories(output / "assets"); fs::create_directories(output / "content");
        write(output / "shaders/app.comp", computeShader(info));
        write(output / "shaders/app.frag", fragmentShader(info));
        write(output / "shaders/app.vert", vertexShader());
        writeEmbeddedAssets(output);
        write(output / "build.cjs", buildScript());
        write(output / "app.json", "{\n  \"format\": \"hblauncher.gpu-app\",\n  \"abi\": 1,\n  \"id\": \"" + jsonEscape(id) + "\",\n  \"name\": \"" + jsonEscape(display) + "\",\n  \"version\": \"1.0.0\",\n  \"description\": \"" + jsonEscape(appDescription) + "\",\n  \"icon\": \"assets/icon.png\",\n  \"initialState\": \"assets/state.bin\",\n  \"data\": \"assets/text.bin\",\n  \"shaders\": {\"vertex\": \"app.vert.spv\", \"fragment\": \"app.frag.spv\", \"compute\": \"app.comp.spv\"}\n}\n");
        std::string pyIconCmd = "python -c \"from PIL import Image; im = Image.open(r'" + icon.string() + "'); im.thumbnail((512, 512)); im.save(r'" + (output / "assets/icon.png").string() + "', 'PNG')\"";
        int iconRes = std::system(pyIconCmd.c_str());
        if (iconRes != 0) {
            std::error_code copyError;
            fs::copy_file(icon, output / "assets/icon.png", fs::copy_options::overwrite_existing, copyError);
            if (copyError) throw std::runtime_error("cannot copy iconpath to output assets/icon.png: " + copyError.message());
        }
        fs::copy_file(output / "apps/hello-world/assets/text.bin", output / "assets/text.bin", fs::copy_options::overwrite_existing);
        fs::remove_all(output / "apps");
        std::vector<unsigned char> state(1024, 0); state[0] = 'A'; state[1] = '2'; state[2] = 'N'; state[3] = 'S';
        std::ofstream stateFile(output / "assets/state.bin", std::ios::binary);
        stateFile.write(reinterpret_cast<const char*>(state.data()), static_cast<std::streamsize>(state.size()));
        if (!stateFile) throw std::runtime_error("failed writing initial GPU state");
        write(output / "port-analysis.json", any2ns2::pipelineJson(source, info.files));
        write(output / "PORTING.md", portingDoc(info, info.files.size()));
        std::cout << "Detected " << info.framework << " (" << info.language << "); preserved " << info.files.size() << " files.\n"
                  << "Created HBLauncher GPU scaffold at " << output << "\n"
                  << "Original executable behavior has NOT been translated. Review PORTING.md before hardware use.\n";
        if (doBuild) {
            std::string command = "node " + shellQuote((output / "build.cjs").string());
            int result = std::system(command.c_str());
            if (result != 0) throw std::runtime_error("shader scaffold build failed (Node/GLSL toolchain); generated sources remain in port-dir");
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "any2ns2: " << e.what() << '\n'; return 1;
    }
}
