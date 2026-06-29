#pragma once
#include "FileScanner.h"
#include "Renamer.h"
#include <string>

class StructureExporter {
public:
    // ASCII directory tree of the ORIGINAL folder structure (pre-flattening).
    static std::string buildStructureText(const ScanResult& scan, const std::string& projectName);

    // manifest.json mapping flattened names back to original relative paths,
    // plus size/extension metadata for every file.
    static std::string buildManifestJson(
        const ScanResult& scan,
        const std::vector<FlattenedFile>& flattened,
        const std::string& projectName
    );
};
