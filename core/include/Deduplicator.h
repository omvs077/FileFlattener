#pragma once
#include "FileScanner.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

struct DuplicateGroup {
    std::string sha256;
    std::vector<size_t> fileIndices;
};

struct DedupResult {
    std::unordered_map<std::string, std::vector<size_t>> nameCollisions;
    std::vector<DuplicateGroup> contentDuplicates;
};

// progress(filesHashedSoFar, totalFilesNeedingHash)
using DedupProgressCallback = std::function<void(size_t, size_t)>;

class Deduplicator {
public:
    bool process(
        const ScanResult& scan,
        DedupResult& outResult,
        std::string& errorMsg,
        DedupProgressCallback onProgress = nullptr
    );

private:
    std::string sha256File(const std::filesystem::path& path, std::string& errorMsg);
};
