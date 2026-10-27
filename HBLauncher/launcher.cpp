#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct AppManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string icon;
    fs::path path;
};

static std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    std::ostringstream out; out << in.rdbuf(); return out.str();
}

// Simple JSON string property extractor for app.json manifests
static std::string extractJsonField(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

static std::vector<AppManifest> discoverApps(const fs::path& root) {
    std::vector<AppManifest> apps;
    if (!fs::exists(root) || !fs::is_directory(root)) return apps;
    
    // Check root and subdirectories like apps/ or parent directories
    for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file() && entry.path().filename() == "app.json") {
            std::string content = readFile(entry.path());
            AppManifest app;
            app.path = entry.path().parent_path();
            app.id = extractJsonField(content, "id");
            app.name = extractJsonField(content, "name");
            app.version = extractJsonField(content, "version");
            app.description = extractJsonField(content, "description");
            app.icon = extractJsonField(content, "icon");
            if (app.name.empty()) app.name = app.path.filename().string();
            apps.push_back(app);
        }
    }
    return apps;
}

int main(int argc, char** argv) {
    fs::path searchRoot = (argc > 1) ? argv[1] : ".";
    std::cout << "HBLauncher for NS2 - App & Port Discovery Scanner\n"
              << "Scanning directory: " << fs::absolute(searchRoot) << "\n\n";

    auto apps = discoverApps(searchRoot);
    if (apps.empty()) {
        std::cout << "No HBLauncher apps or ports found.\n"
                  << "Use 'any2ns2' to port projects or place apps in HBLauncher/apps/.\n";
        return 0;
    }

    std::cout << "Discovered " << apps.size() << " HBLauncher app(s) / port(s):\n";
    for (size_t i = 0; i < apps.size(); ++i) {
        const auto& app = apps[i];
        std::cout << "[" << (i + 1) << "] " << app.name << " (ID: " << app.id << ")\n"
                  << "    Version:     " << (app.version.empty() ? "1.0.0" : app.version) << "\n"
                  << "    Description: " << (app.description.empty() ? "No description provided." : app.description) << "\n"
                  << "    Path:        " << fs::absolute(app.path) << "\n"
                  << "    Icon:        " << app.icon << "\n\n";
    }
    std::cout << "Ready to launch via HBLauncher GPU runtime execution.\n";
    return 0;
}
