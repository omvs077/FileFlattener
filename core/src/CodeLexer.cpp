#include "CodeLexer.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <unordered_map>

namespace {

bool isCppFile(const std::filesystem::path& p) {
    std::string e = p.extension().string();
    return e==".cpp"||e==".cc"||e==".cxx"||e==".c"||e==".h"||e==".hpp"||e==".hxx";
}
bool isPyFile(const std::filesystem::path& p) {
    return p.extension().string()==".py";
}
bool isJsFile(const std::filesystem::path& p) {
    std::string e = p.extension().string();
    return e==".js"||e==".ts"||e==".jsx"||e==".tsx";
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

void CodeLexer::analyzeCpp(const std::filesystem::path& file, int fileNodeId,
                            CodeGraph& graph,
                            std::unordered_map<std::string,int>& fileIndex) {
    std::string src = readFile(file);
    if (src.empty()) return;

    // #include "foo.h" or <foo.h>
    static const std::regex reInclude(R"(#\s*include\s*[\"<]([^\">\n]+)[\">])");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reInclude);
         it != std::sregex_iterator(); ++it) {
        std::string inc = (*it)[1].str();
        auto found = fileIndex.find(inc);
        if (found == fileIndex.end()) {
            // Also try just the filename part.
            std::filesystem::path incPath(inc);
            found = fileIndex.find(incPath.filename().string());
        }
        if (found != fileIndex.end()) {
            graph.edges.push_back({fileNodeId, found->second, CodeEdgeType::Includes});
        }
    }

    // class Foo / class Foo : public Bar
    static const std::regex reClass(
        R"(\b(class|struct)\s+(\w+)\s*(?::\s*(?:public|protected|private)\s+(\w+))?)");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reClass);
         it != std::sregex_iterator(); ++it) {
        std::string keyword = (*it)[1].str();
        std::string className = (*it)[2].str();
        if (className == "override" || className == "final") continue;

        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = (keyword == "struct") ? CodeNodeType::Struct : CodeNodeType::Class;
        node.name     = className;
        node.filePath = file.string();
        graph.nodes.push_back(node);

        // Contains edge: file → class
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});

        // Inheritance edge
        if ((*it)[3].matched) {
            std::string base = (*it)[3].str();
            for (auto& n : graph.nodes) {
                if (n.name == base &&
                   (n.type == CodeNodeType::Class || n.type == CodeNodeType::Struct)) {
                    graph.edges.push_back({node.id, n.id, CodeEdgeType::Inherits});
                    break;
                }
            }
        }
    }

    // Method/function signatures: ReturnType ClassName::methodName( or just name(
    static const std::regex reMethod(
        R"(\b(\w[\w\*\&:<>]*)\s+(?:(\w+)::)?(\w+)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?[{])");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reMethod);
         it != std::sregex_iterator(); ++it) {
        std::string className = (*it)[2].str();
        std::string methodName = (*it)[3].str();
        if (methodName == "if"||methodName == "while"||methodName == "for"||
            methodName == "switch"||methodName == "catch") continue;

        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::Method;
        node.name     = (className.empty() ? "" : className + "::") + methodName;
        node.filePath = file.string();
        graph.nodes.push_back(node);
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});
    }
}

void CodeLexer::analyzePython(const std::filesystem::path& file, int fileNodeId,
                               CodeGraph& graph) {
    std::string src = readFile(file);
    if (src.empty()) return;

    // import foo / from foo import bar
    static const std::regex reImport(R"(^\s*(?:import|from)\s+([\w\.]+))");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reImport);
         it != std::sregex_iterator(); ++it) {
        // Record as unresolved include node for now (no cross-file resolution for Python).
        (void)it;
    }

    // class Foo: / class Foo(Base):
    static const std::regex reClass(R"(^\s*class\s+(\w+)\s*(?:\((\w+)\))?\s*:)", std::regex::multiline);
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reClass);
         it != std::sregex_iterator(); ++it) {
        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::Class;
        node.name     = (*it)[1].str();
        node.filePath = file.string();
        graph.nodes.push_back(node);
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});

        if ((*it)[2].matched) {
            std::string base = (*it)[2].str();
            for (auto& n : graph.nodes) {
                if (n.name == base && n.type == CodeNodeType::Class) {
                    graph.edges.push_back({node.id, n.id, CodeEdgeType::Inherits});
                    break;
                }
            }
        }
    }

    // def method_name(
    static const std::regex reFunc(R"(^\s*def\s+(\w+)\s*\()", std::regex::multiline);
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reFunc);
         it != std::sregex_iterator(); ++it) {
        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::Function;
        node.name     = (*it)[1].str();
        node.filePath = file.string();
        graph.nodes.push_back(node);
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});
    }
}

void CodeLexer::analyzeJs(const std::filesystem::path& file, int fileNodeId,
                           CodeGraph& graph) {
    std::string src = readFile(file);
    if (src.empty()) return;

    // import ... from 'foo' / require('foo')
    static const std::regex reImport(R"((?:import\s+.*from\s+['"]([^'"]+)['"]|require\s*\(\s*['"]([^'"]+)['"]\s*\)))");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reImport);
         it != std::sregex_iterator(); ++it) {
        (void)it; // cross-file resolution in future milestone
    }

    // class Foo / class Foo extends Bar
    static const std::regex reClass(R"(\bclass\s+(\w+)(?:\s+extends\s+(\w+))?)");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reClass);
         it != std::sregex_iterator(); ++it) {
        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::Class;
        node.name     = (*it)[1].str();
        node.filePath = file.string();
        graph.nodes.push_back(node);
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});

        if ((*it)[2].matched) {
            std::string base = (*it)[2].str();
            for (auto& n : graph.nodes) {
                if (n.name == base && n.type == CodeNodeType::Class) {
                    graph.edges.push_back({node.id, n.id, CodeEdgeType::Inherits});
                    break;
                }
            }
        }
    }

    // function foo( / const foo = ( / const foo = function(
    static const std::regex reFunc(R"(\b(?:function\s+(\w+)|(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\()\s*\()");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), reFunc);
         it != std::sregex_iterator(); ++it) {
        std::string name = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
        if (name.empty()) continue;
        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::Function;
        node.name     = name;
        node.filePath = file.string();
        graph.nodes.push_back(node);
        graph.edges.push_back({fileNodeId, node.id, CodeEdgeType::Contains});
    }
}

CodeGraph CodeLexer::analyze(const std::filesystem::path& root,
                              const std::vector<std::filesystem::path>& sourceFiles) {
    CodeGraph graph;
    std::unordered_map<std::string,int> fileIndex; // filename → node id

    // First pass: create a file node for every source file.
    for (const auto& path : sourceFiles) {
        if (!isCppFile(path) && !isPyFile(path) && !isJsFile(path)) continue;
        CodeNode node;
        node.id       = static_cast<int>(graph.nodes.size());
        node.type     = CodeNodeType::File;
        node.name     = path.filename().string();
        node.filePath = path.string();
        fileIndex[node.name] = node.id;
        // Also index by relative path for cross-project includes.
        auto rel = std::filesystem::relative(path, root).string();
        std::replace(rel.begin(), rel.end(), '\\', '/');
        fileIndex[rel] = node.id;
        graph.nodes.push_back(node);
    }

    // Second pass: analyze each file.
    for (const auto& path : sourceFiles) {
        std::string name = path.filename().string();
        auto it = fileIndex.find(name);
        if (it == fileIndex.end()) continue;
        int fileNodeId = it->second;
        try {
            if (isCppFile(path))       analyzeCpp(path, fileNodeId, graph, fileIndex);
            else if (isPyFile(path))   analyzePython(path, fileNodeId, graph);
            else if (isJsFile(path))   analyzeJs(path, fileNodeId, graph);
        } catch (const std::exception&) { /* skip malformed file */ }
    }
    return graph;
}
