#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace artgen::gui {

struct LanguageInfo {
    std::string code;      // e.g. "en", "fr"
    std::string name;      // e.g. "English", "Français"
    std::string flag;      // two-letter country code for flag drawing, e.g. "gb", "fr"
};

// Scans ui/ for ui_strings.*.json files and returns metadata for each.
inline std::vector<LanguageInfo> scan_languages(const std::string& dir = "ui") {
    std::vector<LanguageInfo> result;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        auto fn = entry.path().filename().string();
        // Match "ui_strings.XX.json" — at least one char between the dots
        const std::string prefix = "ui_strings.";
        const std::string suffix = ".json";
        if (fn.size() <= prefix.size() + suffix.size()) continue;
        if (fn.substr(0, prefix.size()) != prefix) continue;
        if (fn.substr(fn.size() - suffix.size()) != suffix) continue;

        std::string code = fn.substr(prefix.size(),
                                     fn.size() - prefix.size() - suffix.size());
        if (code.empty()) continue;

        // Try to read name and flag from _meta block
        LanguageInfo info;
        info.code = code;
        info.name = code;   // fallback
        info.flag = code;   // fallback
        std::ifstream f(entry.path());
        if (f) {
            nlohmann::json j;
            try {
                f >> j;
                if (j.contains("_meta")) {
                    auto& m = j["_meta"];
                    if (m.contains("language")) info.name = m["language"].get<std::string>();
                    if (m.contains("flag"))     info.flag = m["flag"].get<std::string>();
                }
            } catch (...) {}
        }
        result.push_back(std::move(info));
    }
    std::sort(result.begin(), result.end(),
              [](const LanguageInfo& a, const LanguageInfo& b){ return a.code < b.code; });
    return result;
}

// Loads ui/ui_strings.<lang>.json and provides label/tooltip lookups.
// Falls back to the raw key string if a translation is missing.
class UiStrings {
public:
    explicit UiStrings(const std::string& lang = "en") { load(lang); }

    void load(const std::string& lang) {
        m_lang = lang;
        m_data = {};
        std::string path = "ui/ui_strings." + lang + ".json";
        std::ifstream f(path);
        if (f) {
            try { f >> m_data; } catch (...) {}
        }
    }

    const std::string& current_lang() const { return m_lang; }

    // Parameter label / tooltip
    const char* label(const char* key) const   { return get("params", key, "label",   key); }
    const char* tooltip(const char* key) const { return get("params", key, "tooltip", ""); }

    // Algorithm label / tooltip
    const char* algo_label(const char* key) const   { return get("algorithms", key, "label",   key); }
    const char* algo_tooltip(const char* key) const { return get("algorithms", key, "tooltip", ""); }

private:
    std::string    m_lang;
    nlohmann::json m_data;
    mutable std::string m_buf;

    const char* get(const char* section, const char* key,
                    const char* field, const char* fallback) const {
        try {
            m_buf = m_data.at(section).at(key).at(field).get<std::string>();
            return m_buf.c_str();
        } catch (...) {
            return fallback;
        }
    }
};

} // namespace artgen::gui
