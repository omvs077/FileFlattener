#pragma once
#include <filesystem>
#include <string>
#include <vector>

enum class ProjectType {
    Unknown,
    Git,
    CMakeCpp,
    MakefileCpp,
    VisualStudioCpp,
    Rust,
    Python,
    JavaScript,
    TypeScript,
    Flutter,
    Android,
    iOS,
    Go,
    Java,
    CSharp,
    PHP,
    Ruby,
    Docker,
    Elixir
};

struct ProjectDetectionResult {
    ProjectType primaryType = ProjectType::Unknown;
    std::vector<ProjectType> allTypes;   // all matched types
    std::string displayName;             // e.g. "C++ (CMake)", "Python"
    std::string detectedMarker;          // e.g. "CMakeLists.txt"
    bool isCodeProject = false;
};

class ProjectDetector {
public:
    // Checks only the immediate root directory (no recursion) for signature files.
    static ProjectDetectionResult detect(const std::filesystem::path& root);

    // Human-readable label for a ProjectType.
    static std::string typeName(ProjectType t);
};
