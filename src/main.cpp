#include <iostream>
#include "FileScanner.h"
#include "Deduplicator.h"
#include "Renamer.h"

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

    std::cout << "Scan complete. Total files: " << result.files.size() << "\n\n";

    Deduplicator dedup;
    DedupResult dedupResult;
    if (!dedup.process(result, dedupResult, err)) {
        std::cerr << "Dedup failed: " << err << "\n";
        return 1;
    }

    std::cout << "Name collisions: " << dedupResult.nameCollisions.size() << "\n";
    std::cout << "Content duplicate groups: " << dedupResult.contentDuplicates.size() << "\n\n";

    Renamer renamer;
    auto flattened = renamer.resolve(result, dedupResult);

    std::cout << "Flattened file list (" << flattened.size() << " files):\n";
    for (const auto& ff : flattened) {
        std::cout << "  " << result.files[ff.originalIndex].relativePath.string()
                   << "  ->  " << ff.flattenedName << "\n";
    }

    return 0;
}
