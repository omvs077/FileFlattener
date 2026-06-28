#include "FileScanner.h"
#include <system_error>
#include <array>

namespace fs = std::filesystem;

bool FileScanner::isBlockedSystemPath(const fs::path& canonical) {
    // Compare against common system roots (Windows-focused for now)
    std::array<std::string, 5> blocked = {
        "C:\\", "C:\\Windows", "C:\\Program Files", "C:\\Program Files (x86)", "C:\\ProgramData"
    };
    std::string c = canonical.string();
    for (auto& b : blocked) {
        if (c == b) return true;
    }
    return false;
}

bool FileScanner::validateRoot(const fs::path& root, std::string& errorMsg) {
    std::error_code ec;

    if (!fs::exists(root, ec) || ec) {
        errorMsg = "Path does not exist.";
        return false;
    }

    fs::path canonical = fs::canonical(root, ec);
    if (ec) {
        errorMsg = "Failed to resolve canonical path: " + ec.message();
        return false;
    }

    if (!fs::is_directory(canonical, ec) || ec) {
        errorMsg = "Path is not a directory.";
        return false;
    }

    if (isBlockedSystemPath(canonical)) {
        errorMsg = "Refusing to operate on a protected system directory: " + canonical.string();
        return false;
    }

    return true;
}

bool FileScanner::scan(const fs::path& root, ScanResult& outResult, std::string& errorMsg) {
    std::error_code ec;
    fs::path canonicalRoot = fs::canonical(root, ec);
    if (ec) {
        errorMsg = "Failed to canonicalize root for scan: " + ec.message();
        return false;
    }

    outResult = ScanResult{};

    fs::recursive_directory_iterator it(
        canonicalRoot,
        fs::directory_options::skip_permission_denied,
        ec
    );
    fs::recursive_directory_iterator end;

    if (ec) {
        errorMsg = "Failed to start directory iteration: " + ec.message();
        return false;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            // Skip this entry, keep going
            ec.clear();
            continue;
        }

        const fs::directory_entry& entry = *it;

        // Depth check
        if (it.depth() > kMaxDepth) {
            errorMsg = "Directory depth exceeds limit (" + std::to_string(kMaxDepth) + ").";
            return false;
        }

        // Symlink check — drop and don't recurse
        if (fs::is_symlink(entry.path(), ec)) {
            outResult.skippedSymlinks++;
            it.disable_recursion_pending();
            continue;
        }

        if (entry.is_directory(ec)) {
            outResult.totalDirectories++;
            continue;
        }

        if (!entry.is_regular_file(ec)) {
            continue; // skip devices, sockets, etc.
        }

        uint64_t size = entry.file_size(ec);
        if (ec) {
            ec.clear();
            continue;
        }

        ScannedFile sf;
        sf.absolutePath = entry.path();
        sf.relativePath = fs::relative(entry.path(), canonicalRoot, ec);
        sf.sizeBytes = size;

        outResult.files.push_back(std::move(sf));
        outResult.totalSizeBytes += size;

        // Hard limit checks
        if (outResult.files.size() > kMaxFiles) {
            errorMsg = "File count exceeds limit (" + std::to_string(kMaxFiles) + ").";
            return false;
        }
        if (outResult.totalSizeBytes > kMaxTotalBytes) {
            errorMsg = "Total size exceeds limit (500 MB).";
            return false;
        }
    }

    return true;
}