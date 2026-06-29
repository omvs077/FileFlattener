#pragma once
#include <filesystem>
#include <string>
#include <vector>

enum class PresetType {
    StandardBackup,
    AiReadySourceOnly,
    MediaOnly,
    DocumentsOnly
};

class FilterEngine {
public:
    // excludePatterns: comma/space separated list like ".git, node_modules, *.log"
    explicit FilterEngine(const std::string& excludePatternsCsv, PresetType preset);

    // Returns true if this directory should be skipped entirely (no recursion).
    bool shouldExcludeDirectory(const std::filesystem::path& dirName) const;

    // Returns true if this file should be skipped (not scanned/included).
    bool shouldExcludeFile(const std::filesystem::path& relativePath) const;

private:
    std::vector<std::string> m_literalPatterns; // exact name match (folders or files), e.g. ".git", "node_modules"
    std::vector<std::string> m_globSuffixes;     // "*.log" -> ".log"
    PresetType m_preset;

    static bool matchesPresetExtension(const std::string& ext, PresetType preset);
};
