#include "GraphModel.h"
#include <unordered_map>
#include <algorithm>

namespace {

std::string normalizeSeparators(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::vector<std::string> splitPathSegments(const std::string& path) {
    std::vector<std::string> segments;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) segments.push_back(current);
    return segments;
}

} // namespace

GraphModel GraphBuilder::build(const ScanResult& scanResult) {
    GraphModel model;

    // Root node (id 0)
    GraphNode root;
    root.id = 0;
    root.parentId = -1;
    root.name = "";
    root.relativePath = "";
    root.isDirectory = true;
    root.depth = 0;
    model.nodes.push_back(root);

    // folderPath ("a/b") -> node id, for dedup
    std::unordered_map<std::string, int> folderIndex;
    folderIndex[""] = 0;

    auto ensureFolder = [&](const std::vector<std::string>& segments, int upToIndex) -> int {
        // upToIndex is inclusive count of segments forming this folder's path
        std::string folderPath;
        for (int i = 0; i < upToIndex; ++i) {
            if (i > 0) folderPath += "/";
            folderPath += segments[i];
        }
        auto it = folderIndex.find(folderPath);
        if (it != folderIndex.end()) return it->second;

        // Need to create it; parent is segments[0..upToIndex-2]
        std::string parentPath;
        for (int i = 0; i < upToIndex - 1; ++i) {
            if (i > 0) parentPath += "/";
            parentPath += segments[i];
        }
        int parentId = folderIndex.count(parentPath) ? folderIndex[parentPath] : 0;

        GraphNode node;
        node.id = static_cast<int>(model.nodes.size());
        node.parentId = parentId;
        node.name = segments[upToIndex - 1];
        node.relativePath = folderPath;
        node.isDirectory = true;
        node.depth = upToIndex;
        model.nodes.push_back(node);
        model.nodes[parentId].childIds.push_back(node.id);
        folderIndex[folderPath] = node.id;
        return node.id;
    };

    for (const auto& file : scanResult.files) {
        std::string relPath = normalizeSeparators(file.relativePath.string());
        std::vector<std::string> segments = splitPathSegments(relPath);
        if (segments.empty()) continue;

        // Ensure all folder ancestors exist (all segments except the last, which is the file)
        int parentId = 0;
        for (size_t i = 1; i < segments.size(); ++i) {
            parentId = ensureFolder(segments, static_cast<int>(i));
        }

        GraphNode fileNode;
        fileNode.id = static_cast<int>(model.nodes.size());
        fileNode.parentId = parentId;
        fileNode.name = segments.back();
        fileNode.relativePath = relPath;
        fileNode.isDirectory = false;
        fileNode.sizeBytes = file.sizeBytes;
        fileNode.depth = static_cast<int>(segments.size());
        model.nodes.push_back(fileNode);
        model.nodes[parentId].childIds.push_back(fileNode.id);
    }

    // Roll up folder sizes bottom-up (nodes were appended in creation order,
    // so iterating in reverse guarantees children are summed before parents).
    for (auto it = model.nodes.rbegin(); it != model.nodes.rend(); ++it) {
        if (it->parentId >= 0) {
            model.nodes[it->parentId].sizeBytes += it->isDirectory ? 0 : it->sizeBytes;
        }
    }
    // Fix: folder sizes need full descendant sums, not just direct file children.
    // Recompute properly via post-order accumulation using depth ordering.
    for (auto& n : model.nodes) n.sizeBytes = n.isDirectory ? 0 : n.sizeBytes;
    std::vector<int> order(model.nodes.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return model.nodes[a].depth > model.nodes[b].depth;
    });
    for (int idx : order) {
        const GraphNode& n = model.nodes[idx];
        if (n.parentId >= 0) {
            model.nodes[n.parentId].sizeBytes += n.sizeBytes;
        }
    }

    return model;
}
