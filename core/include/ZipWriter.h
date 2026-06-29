#pragma once
#include "FileScanner.h"
#include "Renamer.h"
#include <vector>
#include <string>
#include <functional>

// progress(bytesWrittenSoFar, totalBytes)
using ZipProgressCallback = std::function<void(uint64_t, uint64_t)>;

class ZipWriter {
public:
    bool writeZip(
        const ScanResult& scan,
        const std::vector<FlattenedFile>& flattened,
        const std::filesystem::path& outputZipPath,
        std::string& errorMsg,
        const std::string& structureTextContent = "",
        const std::string& manifestJsonContent = "",
        ZipProgressCallback onProgress = nullptr
    );
};
