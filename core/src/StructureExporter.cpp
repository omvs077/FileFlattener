#include "StructureExporter.h"
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

struct TreeNode {
    std::map<std::string, TreeNode> children;
    std::vector<std::string> files;
};

static void insertPath(TreeNode& root, const fs::path& relPath) {
    TreeNode* current = &root;
    fs::path parent = relPath.parent_path();

    for (const auto& part : parent) {
        current = &current->children[part.string()];
    }
    current->files.push_back(relPath.filename().string());
}

static void renderTree(const TreeNode& node, std::ostringstream& out, const std::string& prefix) {
    std::vector<std::string> sortedFiles = node.files;
    std::sort(sortedFiles.begin(), sortedFiles.end());

    size_t totalItems = sortedFiles.size() + node.children.size();
    size_t idx = 0;

    for (const auto& fname : sortedFiles) {
        bool isLast = (idx == totalItems - 1);
        out << prefix << (isLast ? "+-- " : "|-- ") << fname << "\n";
        idx++;
    }

    for (const auto& kv : node.children) {
        bool isLast = (idx == totalItems - 1);
        out << prefix << (isLast ? "+-- " : "|-- ") << kv.first << "/\n";
        renderTree(kv.second, out, prefix + (isLast ? "    " : "|   "));
        idx++;
    }
}

std::string StructureExporter::buildStructureText(const ScanResult& scan, const std::string& projectName) {
    TreeNode root;
    for (const auto& f : scan.files) {
        insertPath(root, f.relativePath);
    }

    std::ostringstream out;
    out << projectName << "/\n";
    renderTree(root, out, "");
    return out.str();
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

std::string StructureExporter::buildManifestJson(
    const ScanResult& scan,
    const std::vector<FlattenedFile>& flattened,
    const std::string& projectName)
{
    std::ostringstream out;
    out << "{\n";
    out << "  \"project_name\": \"" << jsonEscape(projectName) << "\",\n";
    out << "  \"total_files\": " << flattened.size() << ",\n";
    out << "  \"total_size_bytes\": " << scan.totalSizeBytes << ",\n";
    out << "  \"files\": [\n";

    for (size_t i = 0; i < flattened.size(); ++i) {
        const auto& ff = flattened[i];
        const auto& src = scan.files[ff.originalIndex];

        out << "    {\n";
        out << "      \"name\": \"" << jsonEscape(ff.flattenedName) << "\",\n";
        out << "      \"original_relative_path\": \"" << jsonEscape(src.relativePath.generic_string()) << "\",\n";
        out << "      \"extension\": \"" << jsonEscape(src.relativePath.extension().string()) << "\",\n";
        out << "      \"size_bytes\": " << src.sizeBytes << "\n";
        out << "    }" << (i + 1 < flattened.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    return out.str();
}
