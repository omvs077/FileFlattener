#include "ProjectDetector.h"
#include <unordered_map>

namespace {

struct Signature {
    std::string marker;
    ProjectType type;
    std::string displayName;
};

// Ordered by priority (first match = primary type).
static const Signature kSignatures[] = {
    { "CMakeLists.txt",     ProjectType::CMakeCpp,        "C++ (CMake)"         },
    { "Makefile",           ProjectType::MakefileCpp,     "C++ (Makefile)"      },
    { "configure.ac",       ProjectType::MakefileCpp,     "C++ (Autoconf)"      },
    { "meson.build",        ProjectType::MakefileCpp,     "C++ (Meson)"         },
    { ".sln",               ProjectType::VisualStudioCpp, "C++ (Visual Studio)" },
    { "Cargo.toml",         ProjectType::Rust,            "Rust"                },
    { "go.mod",             ProjectType::Go,              "Go"                  },
    { "pom.xml",            ProjectType::Java,            "Java (Maven)"        },
    { "build.xml",          ProjectType::Java,            "Java (Ant)"          },
    { "build.gradle",       ProjectType::Android,         "Android/Java"        },
    { "build.gradle.kts",   ProjectType::Android,         "Android/Kotlin"      },
    { "pubspec.yaml",       ProjectType::Flutter,         "Flutter/Dart"        },
    { "Package.swift",      ProjectType::iOS,             "iOS/Swift"           },
    { "Podfile",            ProjectType::iOS,             "iOS (CocoaPods)"     },
    { "pyproject.toml",     ProjectType::Python,          "Python"              },
    { "requirements.txt",   ProjectType::Python,          "Python"              },
    { "Pipfile",            ProjectType::Python,          "Python (Pipenv)"     },
    { "setup.py",           ProjectType::Python,          "Python"              },
    { "tsconfig.json",      ProjectType::TypeScript,      "TypeScript"          },
    { "package.json",       ProjectType::JavaScript,      "JavaScript/Node.js"  },
    { "composer.json",      ProjectType::PHP,             "PHP"                 },
    { "Gemfile",            ProjectType::Ruby,            "Ruby"                },
    { "mix.exs",            ProjectType::Elixir,          "Elixir"              },
    { "global.json",        ProjectType::CSharp,          "C# (.NET)"           },
    { "Dockerfile",         ProjectType::Docker,          "Docker"              },
    { "docker-compose.yml", ProjectType::Docker,          "Docker Compose"      },
    { ".git",               ProjectType::Git,             "Git Repository"      },
    { ".gitignore",         ProjectType::Git,             "Git Repository"      },
};

} // namespace

ProjectDetectionResult ProjectDetector::detect(const std::filesystem::path& root) {
    ProjectDetectionResult result;
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root))
        return result;

    // Collect filenames/dirnames in root only (no recursion).
    std::unordered_map<std::string, bool> entries;
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(root, ec)) {
        entries[entry.path().filename().string()] = true;
        // Also check extension-based matches (e.g. .sln).
        entries[entry.path().extension().string()] = true;
    }

    for (const auto& sig : kSignatures) {
        if (entries.count(sig.marker)) {
            result.allTypes.push_back(sig.type);
            if (!result.isCodeProject) {
                result.primaryType   = sig.type;
                result.displayName   = sig.displayName;
                result.detectedMarker = sig.marker;
                result.isCodeProject = true;
            }
        }
    }

    return result;
}

std::string ProjectDetector::typeName(ProjectType t) {
    switch (t) {
        case ProjectType::CMakeCpp:        return "C++ (CMake)";
        case ProjectType::MakefileCpp:     return "C++ (Makefile)";
        case ProjectType::VisualStudioCpp: return "C++ (Visual Studio)";
        case ProjectType::Rust:            return "Rust";
        case ProjectType::Python:          return "Python";
        case ProjectType::JavaScript:      return "JavaScript";
        case ProjectType::TypeScript:      return "TypeScript";
        case ProjectType::Flutter:         return "Flutter/Dart";
        case ProjectType::Android:         return "Android";
        case ProjectType::iOS:             return "iOS/Swift";
        case ProjectType::Go:              return "Go";
        case ProjectType::Java:            return "Java";
        case ProjectType::CSharp:          return "C# (.NET)";
        case ProjectType::PHP:             return "PHP";
        case ProjectType::Ruby:            return "Ruby";
        case ProjectType::Docker:          return "Docker";
        case ProjectType::Elixir:          return "Elixir";
        case ProjectType::Git:             return "Git Repository";
        default:                           return "Unknown";
    }
}
