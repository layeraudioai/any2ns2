#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace any2ns2 {
namespace fs = std::filesystem;
struct ApiEvidence { std::string feature, symbol, path; size_t line; };
inline std::string pipelineJson(const fs::path& root, const std::vector<fs::path>& files) {
    const std::vector<std::pair<std::string, std::vector<std::string>>> groups = {
        {"view", {"SpriteBatch", "SpriteFont", "GraphicsDevice", "Texture2D", "RenderTarget2D", "DrawString", "Effect"}},
        {"controller", {"GamePad.GetState", "Keyboard.GetState", "Mouse.GetState", "TouchPanel.GetState"}},
        {"audio-output", {"SoundEffect", "SoundEffectInstance", "DynamicSoundEffectInstance", "MediaPlayer"}},
        {"audio-input", {"Microphone", "AudioCapture", "WasapiCapture", "WaveInEvent"}}
    };
    std::vector<ApiEvidence> evidence;
    for (const auto& rel : files) {
        std::string ext = rel.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (ext != ".cs" && ext != ".cpp" && ext != ".h" && ext != ".hpp" && ext != ".gd" && ext != ".java" && ext != ".py") continue;
        std::ifstream in(root / rel);
        std::string line; size_t number = 0;
        while (std::getline(in, line)) {
            ++number;
            for (const auto& group : groups) for (const auto& symbol : group.second) {
                if (line.find(symbol) != std::string::npos)
                    evidence.push_back({group.first, symbol, rel.generic_string(), number});
            }
        }
    }
    auto quote = [](const std::string& value) {
        std::string out = "\"";
        for (unsigned char c : value) {
            if (c == '\\' || c == '\"') { out += '\\'; out += static_cast<char>(c); }
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else if (c >= 0x20) out += static_cast<char>(c);
        }
        return out + "\"";
    };
    std::ostringstream out;
    out << "{\n  \"format\": \"any2ns2.port-analysis\",\n  \"version\": 1,\n"
        << "  \"scope\": \"Lexical API hints only; no source execution, semantic resolution, translation, or parity claim.\",\n"
        << "  \"target\": {\"rendering\": \"1920x1080@60 target requested; runtime throughput not guaranteed by this analysis\", \"audioOutput\": \"PCM effects path exists; source audio semantics are not translated\", \"audioInput\": \"enumeration exists; recording path unverified and not exposed as a port\"},\n"
        << "  \"evidence\": [";
    for (size_t i = 0; i < evidence.size(); ++i) {
        const auto& item = evidence[i];
        out << (i ? "," : "") << "\n    {\"feature\":" << quote(item.feature)
            << ",\"symbol\":" << quote(item.symbol) << ",\"path\":" << quote(item.path)
            << ",\"line\":" << item.line << "}";
    }
    out << "\n  ],\n  \"status\": \"analysis-only\"\n}\n";
    return out.str();
}
} // namespace any2ns2
