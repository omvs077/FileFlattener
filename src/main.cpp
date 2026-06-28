#include <iostream>
#include "FileScanner.h"

int main(int argc, char* argv[]) {
    std::filesystem::path root = (argc > 1) ? argv[1] : "D:/My projects/FileFlattener/src";

    FileScanner scanner;
    std::string err;

    if (!scanner.validateRoot(root, err)) {
        std::cerr << "Validation failed: " << err << "\n";
        return 1;
    }

    ScanResult result;
    if (!scanner.scan(root, result, err)) {
        std::cerr << "Scan failed: " << err << "\n";
        return 1;
    }

    std::cout << "Scan complete.\n";
    std::cout << "Total files: " << result.files.size() << "\n";
    std::cout << "Total directories: " << result.totalDirectories << "\n";
    std::cout << "Total size: " << result.totalSizeBytes << " bytes\n";
    std::cout << "Skipped symlinks: " << result.skippedSymlinks << "\n\n";

    std::cout << "Sample files:\n";
    size_t shown = 0;
    for (const auto& f : result.files) {
        std::cout << "  " << f.relativePath.string() << " (" << f.sizeBytes << " bytes)\n";
        if (++shown >= 10) break;
    }

    return 0;
}