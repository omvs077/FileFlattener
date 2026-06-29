#pragma once
#include "FileScanner.h"
#include "Renamer.h"
#include <vector>
#include <string>

class ZipWriter {
public:
    bool writeZip(
        const ScanResult& scan,
        const std::vector<FlattenedFile>& flattened,
        const std::filesystem::path& outputZipPath,
        std::string& errorMsg,
        const std::string& structureTextContent = "",
        const std::string& manifestJsonContent = ""
    );
};
