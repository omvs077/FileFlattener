#include "Deduplicator.h"
#include <fstream>
#include <picosha2.h>

namespace fs = std::filesystem;

std::string Deduplicator::sha256File(const fs::path& path, std::string& errorMsg) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        errorMsg = "Failed to open file for hashing: " + path.string();
        return "";
    }

    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(file, hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

bool Deduplicator::process(const ScanResult& scan, DedupResult& outResult, std::string& errorMsg, DedupProgressCallback onProgress) {
    outResult = DedupResult{};

    std::unordered_map<std::string, std::vector<size_t>> byFilename;
    for (size_t i = 0; i < scan.files.size(); ++i) {
        std::string fname = scan.files[i].relativePath.filename().string();
        byFilename[fname].push_back(i);
    }
    for (auto& kv : byFilename) {
        if (kv.second.size() > 1) {
            outResult.nameCollisions[kv.first] = kv.second;
        }
    }

    std::unordered_map<uint64_t, std::vector<size_t>> bySize;
    for (size_t i = 0; i < scan.files.size(); ++i) {
        bySize[scan.files[i].sizeBytes].push_back(i);
    }

    // Count how many files actually need hashing (size-collision groups only)
    size_t totalToHash = 0;
    for (auto& sizeGroup : bySize) {
        if (sizeGroup.second.size() > 1) totalToHash += sizeGroup.second.size();
    }
    size_t hashedSoFar = 0;

    for (auto& sizeGroup : bySize) {
        auto& indices = sizeGroup.second;
        if (indices.size() < 2) continue;

        std::unordered_map<std::string, std::vector<size_t>> byHash;
        for (size_t idx : indices) {
            std::string hash = sha256File(scan.files[idx].absolutePath, errorMsg);
            hashedSoFar++;
            if (onProgress) onProgress(hashedSoFar, totalToHash);

            if (hash.empty()) {
                errorMsg.clear();
                continue;
            }
            byHash[hash].push_back(idx);
        }

        for (auto& hashGroup : byHash) {
            if (hashGroup.second.size() > 1) {
                DuplicateGroup group;
                group.sha256 = hashGroup.first;
                group.fileIndices = hashGroup.second;
                outResult.contentDuplicates.push_back(std::move(group));
            }
        }
    }

    return true;
}
