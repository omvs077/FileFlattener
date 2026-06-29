#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

struct ScannedFile {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath; // relative to scan root
    uint64_t sizeBytes = 0;
};

struct ScanResult {
    std::vector<ScannedFile> files;
    uint64_t totalSizeBytes = 0;
    size_t skippedSymlinks = 0;
    size_t totalDirectories = 0;
};

class FileScanner {
public:
    // Hard safety thresholds (Phase 2 anti-abuse rules)
    static constexpr int kMaxDepth = 25;
    static constexpr size_t kMaxFiles = SIZE_MAX; // effectively unbounded
    static constexpr uint64_t kMaxTotalBytes = 20ull * 1024 * 1024 * 1024; // 20 GB

    // Returns false if validation fails; errorMsg is filled in.
    bool validateRoot(const std::filesystem::path& root, std::string& errorMsg);

    // Performs the DFS scan. Returns false on hard-limit violation mid-scan.
    bool scan(const std::filesystem::path& root, ScanResult& outResult, std::string& errorMsg);

private:
    bool isBlockedSystemPath(const std::filesystem::path& canonical);
};
