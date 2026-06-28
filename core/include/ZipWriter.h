#pragma once
#include "FileScanner.h"
#include "Renamer.h"
#include <vector>
#include <string>

class ZipWriter {
public:
    // Streams each file (per the flattened name mapping) into a single ZIP archive.
    bool writeZip(
        const ScanResult& scan,
        const std::vector<FlattenedFile>& flattened,
        const std::filesystem::path& outputZipPath,
        std::string& errorMsg
    );
};
