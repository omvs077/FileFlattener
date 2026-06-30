#include "Renamer.h"
#include <unordered_set>
namespace fs = std::filesystem;
static std::string sanitizeComponent(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '/' || c == '\\' || c == ':') {
            out += '_';
        } else {
            out += c;
        }
    }
    // Collapse a component that is exactly ".." or "." into a safe literal,
    // since these have special meaning in path contexts even without separators.
    if (out == "..") out = "__";
    else if (out == ".") out = "_";
    return out;
}
std::string Renamer::buildPrefixedName(const ScannedFile& file) {
    fs::path parent = file.relativePath.parent_path();
    std::string filename = sanitizeComponent(file.relativePath.filename().string());
    if (parent.empty()) {
        return filename;
    }
    std::string prefix;
    for (const auto& part : parent) {
        if (!prefix.empty()) prefix += "__";
        prefix += sanitizeComponent(part.string());
    }
    return prefix + "__" + filename;
}
std::vector<FlattenedFile> Renamer::resolve(const ScanResult& scan, const DedupResult& dedup) {
    std::vector<FlattenedFile> result;
    result.reserve(scan.files.size());
    std::unordered_set<size_t> collidingIndices;
    for (const auto& kv : dedup.nameCollisions) {
        for (size_t idx : kv.second) {
            collidingIndices.insert(idx);
        }
    }
    std::unordered_set<std::string> usedNames;
    for (size_t i = 0; i < scan.files.size(); ++i) {
        const ScannedFile& f = scan.files[i];
        std::string finalName;
        if (collidingIndices.count(i)) {
            finalName = buildPrefixedName(f);
        } else {
            finalName = sanitizeComponent(f.relativePath.filename().string());
        }
        if (usedNames.count(finalName)) {
            std::string base = finalName;
            int counter = 1;
            do {
                finalName = base + "_(" + std::to_string(counter) + ")";
                counter++;
            } while (usedNames.count(finalName));
        }
        usedNames.insert(finalName);
        result.push_back({ i, finalName });
    }
    return result;
}
