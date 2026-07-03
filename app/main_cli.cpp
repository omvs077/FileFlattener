#include <iostream>
#include "FileScanner.h"
#include "Deduplicator.h"
#include "Renamer.h"
#include "ZipWriter.h"
#include "GraphModel.h"
#include "ProjectDetector.h"
#include "CodeLexer.h"

namespace {
void printGraphSummary(const ScanResult& result) {
    GraphModel model = GraphBuilder::build(result);
    int maxDepth = 0;
    int folderCount = 0;
    int fileCount = 0;
    for (const auto& n : model.nodes) {
        if (n.depth > maxDepth) maxDepth = n.depth;
        if (n.isDirectory) ++folderCount; else ++fileCount;
    }
    std::cout << "\n[GraphModel] nodes: " << model.nodes.size()
              << " (folders: " << folderCount << ", files: " << fileCount << ")\n";
    std::cout << "[GraphModel] max depth: " << maxDepth << "\n";
    std::cout << "[GraphModel] root size (bytes): " << model.nodes[0].sizeBytes << "\n";
}
}

int main(int argc, char* argv[]) {
    std::filesystem::path root = (argc > 1) ? argv[1] : "D:/My projects/FileFlattener/testdata";
    std::filesystem::path outputZip = (argc > 2) ? argv[2] : "D:/My projects/FileFlattener/output.zip";
    bool graphOnly = (argc > 3) && std::string(argv[3]) == "--graph";
    bool codeOnly  = (argc > 3) && std::string(argv[3]) == "--code";

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

    ProjectDetectionResult detection = ProjectDetector::detect(root);
    if (detection.isCodeProject) {
        std::cout << "[ProjectDetector] Type: " << detection.displayName << "\n";
        std::cout << "[ProjectDetector] Marker: " << detection.detectedMarker << "\n";
    } else {
        std::cout << "[ProjectDetector] Not a recognized code project.\n";
    }

    if (codeOnly) {
        std::vector<std::filesystem::path> sources;
        for (const auto& f : result.files) sources.push_back(f.absolutePath);
        CodeGraph cg = CodeLexer::analyze(root, sources);
        int files=0, classes=0, methods=0;
        for (const auto& n : cg.nodes) {
            if (n.type==CodeNodeType::File) ++files;
            else if (n.type==CodeNodeType::Class||n.type==CodeNodeType::Struct) ++classes;
            else ++methods;
        }
        int includes=0, inherits=0;
        for (const auto& e : cg.edges) {
            if (e.type==CodeEdgeType::Includes) ++includes;
            else if (e.type==CodeEdgeType::Inherits) ++inherits;
        }
        std::cout << "\n[CodeLexer] file nodes: " << files << "\n";
        std::cout << "[CodeLexer] classes/structs: " << classes << "\n";
        std::cout << "[CodeLexer] methods/functions: " << methods << "\n";
        std::cout << "[CodeLexer] #include edges: " << includes << "\n";
        std::cout << "[CodeLexer] inheritance edges: " << inherits << "\n";
        return 0;
    }
    if (graphOnly) {
        printGraphSummary(result);
        return 0;
    }

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

    printGraphSummary(result);
    return 0;
}
