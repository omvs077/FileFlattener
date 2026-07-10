#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

enum class CodeNodeType {
    File,
    Class,
    Struct,
    Method,
    Function
};

struct CodeNode {
    int id = -1;
    CodeNodeType type = CodeNodeType::File;
    std::string name;
    std::string filePath;
    int line = 0;
};

enum class CodeEdgeType {
    Includes,
    Contains,
    Inherits,
    Calls
};

struct CodeEdge {
    int fromId = -1;
    int toId   = -1;
    CodeEdgeType type = CodeEdgeType::Includes;
};

struct CodeGraph {
    std::vector<CodeNode> nodes;
    std::vector<CodeEdge> edges;
};

class CodeLexer {
public:
    static CodeGraph analyze(const std::filesystem::path& root,
                             const std::vector<std::filesystem::path>& sourceFiles);
private:
    static void analyzeCpp(const std::filesystem::path& file, int fileNodeId,
                           CodeGraph& graph,
                           std::unordered_map<std::string,int>& fileIndex);
    static void analyzePython(const std::filesystem::path& file, int fileNodeId,
                              CodeGraph& graph);
    static void analyzeJs(const std::filesystem::path& file, int fileNodeId,
                          CodeGraph& graph);
};
