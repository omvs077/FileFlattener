#pragma once
#include "FileScanner.h"
#include "Deduplicator.h"
#include <vector>
#include <string>

struct FlattenedFile {
    size_t originalIndex;       // index into ScanResult.files
    std::string flattenedName;  // final name to use inside the ZIP
};

class Renamer {
public:
    // Produces a flattened name for every file. Files involved in a name
    // collision get a path-based prefix (e.g. "folderA__notes.txt").
    // Files with no collision keep their original filename.
    std::vector<FlattenedFile> resolve(const ScanResult& scan, const DedupResult& dedup);

private:
    // Builds "folderA__folderB__filename.ext" using all parent dir names
    // joined with "__", to guarantee uniqueness even on deeper collisions.
    std::string buildPrefixedName(const ScannedFile& file);
};
