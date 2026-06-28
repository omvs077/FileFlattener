#include <iostream>
#include "FileScanner.h"
#include "Deduplicator.h"

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
    std::cout << "Total size: " << result.totalSizeBytes << " bytes\n\n";

    Deduplicator dedup;
    DedupResult dedupResult;
    if (!dedup.process(result, dedupResult, err)) {
        std::cerr << "Dedup failed: " << err << "\n";
        return 1;
    }

    std::cout << "Name collisions: " << dedupResult.nameCollisions.size() << "\n";
    for (auto& kv : dedupResult.nameCollisions) {
        std::cout << "  '" << kv.first << "' appears " << kv.second.size() << " times:\n";
        for (size_t idx : kv.second) {
            std::cout << "    - " << result.files[idx].relativePath.string() << "\n";
        }
    }

    std::cout << "\nContent duplicate groups: " << dedupResult.contentDuplicates.size() << "\n";
    for (auto& group : dedupResult.contentDuplicates) {
        std::cout << "  Hash " << group.sha256.substr(0, 12) << "...:\n";
        for (size_t idx : group.fileIndices) {
            std::cout << "    - " << result.files[idx].relativePath.string() << "\n";
        }
    }

    return 0;
}
