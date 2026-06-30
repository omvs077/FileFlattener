#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "FileScanner.h"

struct GraphNode {
    int id = -1;
    int parentId = -1;
    std::string name;           // last path segment
    std::string relativePath;   // full relative path, '/' separated
    bool isDirectory = false;
    uint64_t sizeBytes = 0;     // file: own size; folder: sum of descendants
    int depth = 0;
    std::vector<int> childIds;
};

struct GraphModel {
    std::vector<GraphNode> nodes; // index in vector == GraphNode::id
};

class GraphBuilder {
public:
    // Builds a folder/file tree graph from a completed scan.
    // Root folder itself is node 0 (relativePath empty, name = "").
    static GraphModel build(const ScanResult& scanResult);
};
