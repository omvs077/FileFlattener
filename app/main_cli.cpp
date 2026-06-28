#include <iostream>
#include "FileScanner.h"
#include "Deduplicator.h"
#include "Renamer.h"
#include "ZipWriter.h"

int main(int argc, char* argv[]) {
    std::filesystem::path root = (argc > 1) ? argv[1] : "D:/My projects/FileFlattener/testdata";
    std::filesystem::path outputZip = (argc > 2) ? argv[2] : "D:/My projects/FileFlattener/output.zip";

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
    std::cout << "Scan complete. Total files: " << result.files.size() << "\n";

    Deduplicator dedup;
    DedupResult dedupResult;
    if (!dedup.process(result, dedupResult, err)) {
        std::cerr << "Dedup failed: " << err << "\n";
        return 1;
    }
    std::cout << "Name collisions: " << dedupResult.nameCollisions.size() << "\n";
    std::cout << "Content duplicate groups: " << dedupResult.contentDuplicates.size() << "\n";

    Renamer renamer;
    auto flattened = renamer.resolve(result, dedupResult);
    std::cout << "Flattened " << flattened.size() << " files.\n";

    ZipWriter zipWriter;
    if (!zipWriter.writeZip(result, flattened, outputZip, err)) {
        std::cerr << "ZIP write failed: " << err << "\n";
        return 1;
    }

    std::cout << "\nZIP created successfully at: " << outputZip.string() << "\n";
    return 0;
}

