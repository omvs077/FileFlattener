#include "CallGraphAnalyzer.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <system_error>

namespace {

bool isCppFile(const std::filesystem::path& p) {
    std::string e = p.extension().string();
    return e==".cpp"||e==".cc"||e==".cxx"||e==".c"||e==".h"||e==".hpp"||e==".hxx";
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Same reMethod pattern as CodeLexer::analyzeCpp — must stay identical so
// match order/count aligns with the Method nodes already created in baseGraph.
const std::regex& methodRegex() {
    static const std::regex re(
        R"(\b(\w[\w\*\&:<>]*)\s+(?:(\w+)::)?(\w+)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?[{])");
    return re;
}

const std::unordered_set<std::string>& skipKeywords() {
    static const std::unordered_set<std::string> kw = {
        "if","for","while","switch","catch","return","sizeof","static_cast",
        "dynamic_cast","reinterpret_cast","const_cast","typeid","decltype",
        "new","delete","throw","using","namespace","class","struct","enum",
        "typedef","template","explicit","noexcept","alignof","alignas","sizeof..."
    };
    return kw;
}

} // namespace

CallGraphResult CallGraphAnalyzer::analyze(const std::filesystem::path& root,
                                            const std::vector<std::filesystem::path>& sourceFiles,
                                            const CodeGraph& baseGraph) {
    (void)root;
    CallGraphResult result;
    result.graph = baseGraph; // copy: we augment with Calls edges

    // Build symbol tables from existing Method/Function nodes.
    std::unordered_map<std::string, int> qualifiedMap;      // "Class::method" or "func" -> id
    std::unordered_map<std::string, std::vector<int>> unqualifiedMap; // "method" -> ids
    // methodNodeIdsByFile: filePath -> ordered list of node ids (source order)
    std::unordered_map<std::string, std::vector<int>> methodNodeIdsByFile;
    // classOfMethod: node id -> owning class name (empty if free function)
    std::unordered_map<int, std::string> classOfMethod;

    for (const auto& n : baseGraph.nodes) {
        if (n.type != CodeNodeType::Method && n.type != CodeNodeType::Function) continue;
        qualifiedMap[n.name] = n.id;
        auto pos = n.name.find("::");
        std::string unqualified = (pos == std::string::npos) ? n.name : n.name.substr(pos + 2);
        std::string className   = (pos == std::string::npos) ? std::string() : n.name.substr(0, pos);
        unqualifiedMap[unqualified].push_back(n.id);
        classOfMethod[n.id] = className;
        methodNodeIdsByFile[n.filePath].push_back(n.id);
    }

    bool capped = false;
    int edgeCount = 0;

    for (const auto& path : sourceFiles) {
        if (capped) break;
        if (!isCppFile(path)) continue;

        std::error_code ec;
        auto fsize = std::filesystem::file_size(path, ec);
        if (ec || fsize > kMaxFileBytes) {
            result.fileSizeSkipped = true;
            continue;
        }

        std::string filePathStr = path.string();
        auto fit = methodNodeIdsByFile.find(filePathStr);
        if (fit == methodNodeIdsByFile.end() || fit->second.empty()) continue;
        const std::vector<int>& orderedIds = fit->second;

        std::string src = readFile(path);
        if (src.empty()) continue;

        try {
            // Re-derive method match positions; must align 1:1 with orderedIds.
            std::vector<std::pair<size_t,size_t>> methodSpans; // (bodyStart, node index in orderedIds)
            std::vector<size_t> braceStart;
            {
                auto it = std::sregex_iterator(src.begin(), src.end(), methodRegex());
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    size_t matchEnd = static_cast<size_t>(it->position(0) + it->length(0));
                    braceStart.push_back(matchEnd - 1); // index of the '{'
                }
            }
            if (braceStart.size() != orderedIds.size()) {
                // Alignment mismatch (file changed on disk, or regex divergence) — skip file safely.
                continue;
            }

            for (size_t i = 0; i < braceStart.size() && !capped; ++i) {
                int methodNodeId = orderedIds[i];
                size_t bodyStartIdx = braceStart[i] + 1;
                int depth = 1;
                size_t j = bodyStartIdx;
                while (j < src.size() && depth > 0) {
                    if (src[j] == '{') depth++;
                    else if (src[j] == '}') depth--;
                    j++;
                }
                if (depth != 0) continue; // unbalanced braces, skip this method body
                size_t bodyEndIdx = j - 1; // index of matching '}'
                if (bodyEndIdx <= bodyStartIdx) continue;

                std::string body = src.substr(bodyStartIdx, bodyEndIdx - bodyStartIdx);
                const std::string& selfClass = classOfMethod[methodNodeId];

                static const std::regex reCall(R"((?:(\w+)::)?(\w+)\s*\()");
                auto cit = std::sregex_iterator(body.begin(), body.end(), reCall);
                auto cend = std::sregex_iterator();
                std::unordered_set<int> addedTargets; // dedupe multiple calls to same target
                for (; cit != cend && !capped; ++cit) {
                    std::string qualifier = (*cit)[1].str();
                    std::string name      = (*cit)[2].str();
                    if (name.empty() || skipKeywords().count(name)) continue;

                    int targetId = -1;
                    if (!qualifier.empty()) {
                        auto qf = qualifiedMap.find(qualifier + "::" + name);
                        if (qf != qualifiedMap.end()) targetId = qf->second;
                    } else {
                        if (!selfClass.empty()) {
                            auto sf = qualifiedMap.find(selfClass + "::" + name);
                            if (sf != qualifiedMap.end()) targetId = sf->second;
                        }
                        if (targetId == -1) {
                            auto uf = unqualifiedMap.find(name);
                            if (uf != unqualifiedMap.end() && uf->second.size() == 1) {
                                targetId = uf->second[0];
                            }
                            // size > 1 => ambiguous, skip (no edge)
                        }
                    }

                    if (targetId == -1 || targetId == methodNodeId) continue;
                    if (!addedTargets.insert(targetId).second) continue; // already linked

                    result.graph.edges.push_back({methodNodeId, targetId, CodeEdgeType::Calls});
                    edgeCount++;
                    if (edgeCount >= kMaxCallEdges) { capped = true; break; }
                }
            }
        } catch (const std::exception&) {
            // Malformed file — skip, consistent with CodeLexer's fail-safe pattern.
            continue;
        }
    }

    result.edgeCapped = capped;
    result.callEdgeCount = edgeCount;
    return result;
}
