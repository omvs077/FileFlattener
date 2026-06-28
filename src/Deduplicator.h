#pragma once
#include "FileScanner.h"
#include <unordered_map>
#include <vector>
#include <string>

struct DuplicateGroup {
    std::string sha256;
    std::vector<size_t> fileIndices;
};

struct DedupResult {
    std::unordered_map<std::string, std::vector<size_t>> nameCollisions;
    std::vector<DuplicateGroup> contentDuplicates;
};

class Deduplicator {
public:
    bool process(const ScanResult& scan, DedupResult& outResult, std::string& errorMsg);

private:
    std::string sha256File(const std::filesystem::path& path, std::string& errorMsg);
};
