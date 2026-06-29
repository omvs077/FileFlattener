#include "FilterEngine.h"
#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace fs = std::filesystem;

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

static std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

FilterEngine::FilterEngine(const std::string& excludePatternsCsv, PresetType preset)
    : m_preset(preset)
{
    std::stringstream ss(excludePatternsCsv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::string clean = trim(token);
        if (clean.empty()) continue;

        if (clean.front() == '*') {
            // glob suffix pattern, e.g. "*.log" -> ".log"
            m_globSuffixes.push_back(toLower(clean.substr(1)));
        } else {
            m_literalPatterns.push_back(toLower(clean));
        }
    }
}

bool FilterEngine::shouldExcludeDirectory(const fs::path& dirName) const {
    std::string name = toLower(dirName.string());
    for (const auto& pattern : m_literalPatterns) {
        if (name == pattern) return true;
    }
    return false;
}

bool FilterEngine::matchesPresetExtension(const std::string& ext, PresetType preset) {
    static const std::unordered_set<std::string> sourceExts = {
        ".cpp", ".h", ".hpp", ".c", ".cc", ".py", ".js", ".ts", ".jsx", ".tsx",
        ".java", ".cs", ".go", ".rs", ".rb", ".php", ".swift", ".kt", ".kts",
        ".txt", ".md", ".json", ".yaml", ".yml", ".xml", ".html", ".css",
        ".sh", ".bat", ".ps1", ".cmake", ".toml", ".ini", ".cfg", ".sql"
    };
    static const std::unordered_set<std::string> mediaExts = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg", ".tiff",
        ".mp4", ".mov", ".avi", ".mkv", ".webm", ".wmv",
        ".mp3", ".wav", ".flac", ".aac", ".ogg", ".m4a"
    };
    static const std::unordered_set<std::string> documentExts = {
        ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        ".txt", ".md", ".rtf", ".odt", ".csv"
    };

    switch (preset) {
        case PresetType::StandardBackup:
            return true; // no filtering
        case PresetType::AiReadySourceOnly:
            return sourceExts.count(ext) > 0;
        case PresetType::MediaOnly:
            return mediaExts.count(ext) > 0;
        case PresetType::DocumentsOnly:
            return documentExts.count(ext) > 0;
    }
    return true;
}

bool FilterEngine::shouldExcludeFile(const fs::path& relativePath) const {
    std::string filename = toLower(relativePath.filename().string());
    std::string ext = toLower(relativePath.extension().string());

    for (const auto& pattern : m_literalPatterns) {
        if (filename == pattern) return true;
    }
    for (const auto& suffix : m_globSuffixes) {
        if (filename.size() >= suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return true;
        }
    }

    if (!matchesPresetExtension(ext, m_preset)) {
        return true;
    }

    return false;
}
