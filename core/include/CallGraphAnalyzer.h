#pragma once
#include "CodeLexer.h"
#include <filesystem>
#include <vector>

// Result of a call-graph analysis pass. Wraps the augmented CodeGraph
// (base nodes/edges plus new Calls edges) along with safety-cap flags
// so the UI layer can warn the user if results were truncated.
struct CallGraphResult {
    CodeGraph graph;
    bool edgeCapped = false;     // true if kMaxCallEdges was hit (truncated)
    bool fileSizeSkipped = false; // true if any file exceeded kMaxFileBytes and was skipped
    int callEdgeCount = 0;
};

// Headless, no Qt. Performs a second regex pass over C++ source files
// already analyzed by CodeLexer, detecting call-sites inside method/function
// bodies via brace-counting, and resolving them against the known symbol
// table (CodeGraph nodes) using unqualified/qualified name matching.
class CallGraphAnalyzer {
public:
    // Hard cap on Calls edges added, to bound render complexity on large projects.
    static constexpr int kMaxCallEdges = 1500;

    // Skip any single file larger than this to avoid pathological regex cost.
    static constexpr size_t kMaxFileBytes = 2 * 1024 * 1024; // 2 MB

    // root: project root (for relative path resolution, consistent with CodeLexer::analyze)
    // sourceFiles: same file list passed to CodeLexer::analyze
    // baseGraph: the CodeGraph already produced by CodeLexer::analyze (nodes/edges reused, not recomputed)
    static CallGraphResult analyze(const std::filesystem::path& root,
                                    const std::vector<std::filesystem::path>& sourceFiles,
                                    const CodeGraph& baseGraph);
};
