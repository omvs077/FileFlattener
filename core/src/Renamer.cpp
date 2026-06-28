#include "Renamer.h"
#include <unordered_set>

namespace fs = std::filesystem;

std::string Renamer::buildPrefixedName(const ScannedFile& file) {
    fs::path parent = file.relativePath.parent_path();
    std::string filename = file.relativePath.filename().string();

    if (parent.empty()) {
        return filename;
    }

    std::string prefix;
    for (const auto& part : parent) {
        if (!prefix.empty()) prefix += "__";
        prefix += part.string();
    }

    return prefix + "__" + filename;
}

std::vector<FlattenedFile> Renamer::resolve(const ScanResult& scan, const DedupResult& dedup) {
    std::vector<FlattenedFile> result;
    result.reserve(scan.files.size());

    // Build a fast lookup of which file indices are involved in a name collision.
    std::unordered_set<size_t> collidingIndices;
    for (const auto& kv : dedup.nameCollisions) {
        for (size_t idx : kv.second) {
            collidingIndices.insert(idx);
        }
    }

    // Track final names to catch any residual collisions after prefixing
    // (e.g. two files at identical relative paths in edge cases).
    std::unordered_set<std::string> usedNames;

    for (size_t i = 0; i < scan.files.size(); ++i) {
        const ScannedFile& f = scan.files[i];
        std::string finalName;

        if (collidingIndices.count(i)) {
            finalName = buildPrefixedName(f);
        } else {
            finalName = f.relativePath.filename().string();
        }

        // Safety net: if even the prefixed name collides (rare), append a counter.
        if (usedNames.count(finalName)) {
            std::string base = finalName;
            int counter = 1;
            do {
                finalName = base + "_(" + std::to_string(counter) + ")";
                counter++;
            } while (usedNames.count(finalName));
        }

        usedNames.insert(finalName);
        result.push_back({ i, finalName });
    }

    return result;
}
