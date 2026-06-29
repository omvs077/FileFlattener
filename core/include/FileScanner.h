#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
#include "FilterEngine.h"

struct ScannedFile {
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;
    uint64_t sizeBytes = 0;
};

struct ScanResult {
    std::vector<ScannedFile> files;
    uint64_t totalSizeBytes = 0;
    size_t skippedSymlinks = 0;
    size_t totalDirectories = 0;
    size_t filteredOutCount = 0; // files/dirs excluded by FilterEngine
};

class FileScanner {
public:
    static constexpr int kMaxDepth = 25;
    static constexpr size_t kMaxFiles = SIZE_MAX;
    static constexpr uint64_t kMaxTotalBytes = 20ull * 1024 * 1024 * 1024;

    bool validateRoot(const std::filesystem::path& root, std::string& errorMsg);

    // filter == nullptr means no filtering (all files included).
    bool scan(const std::filesystem::path& root, ScanResult& outResult, std::string& errorMsg, const FilterEngine* filter = nullptr);

private:
    bool isBlockedSystemPath(const std::filesystem::path& canonical);
};
