#include "FileScanner.h"
#include <system_error>
#include <array>
namespace fs = std::filesystem;
bool FileScanner::isBlockedSystemPath(const fs::path& canonical) {
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
bool FileScanner::scan(const fs::path& root, ScanResult& outResult, std::string& errorMsg, const FilterEngine* filter) {
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
    while (it != end) {
        try {
            if (ec) {
                ec.clear();
                it.increment(ec);
                continue;
            }
            const fs::directory_entry& entry = *it;
            if (it.depth() > kMaxDepth) {
                errorMsg = "Directory depth exceeds limit (" + std::to_string(kMaxDepth) + ").";
                return false;
            }
            std::error_code localEc;
            if (fs::is_symlink(entry.path(), localEc)) {
                outResult.skippedSymlinks++;
                it.disable_recursion_pending();
                it.increment(ec);
                continue;
            }
            if (entry.is_directory(localEc)) {
                if (filter && filter->shouldExcludeDirectory(entry.path().filename())) {
                    outResult.filteredOutCount++;
                    it.disable_recursion_pending();
                    it.increment(ec);
                    continue;
                }
                outResult.totalDirectories++;
                it.increment(ec);
                continue;
            }
            if (!entry.is_regular_file(localEc)) {
                it.increment(ec);
                continue;
            }
            fs::path relPath = fs::relative(entry.path(), canonicalRoot, localEc);
            if (filter && filter->shouldExcludeFile(relPath)) {
                outResult.filteredOutCount++;
                it.increment(ec);
                continue;
            }
            uint64_t size = entry.file_size(localEc);
            if (localEc) {
                it.increment(ec);
                continue;
            }
            ScannedFile sf;
            sf.absolutePath = entry.path();
            sf.relativePath = relPath;
            sf.sizeBytes = size;
            outResult.files.push_back(std::move(sf));
            outResult.totalSizeBytes += size;
            if (outResult.totalSizeBytes > kMaxTotalBytes) {
                errorMsg = "Total size exceeds limit (20 GB).";
                return false;
            }
            it.increment(ec);
        } catch (const fs::filesystem_error&) {
            // Entry could not be accessed (bad path, permissions, encoding issue, etc.) — skip it.
            outResult.skippedSymlinks++; // reuse counter as a general "skipped entries" tally
            std::error_code skipEc;
            it.increment(skipEc);
            ec.clear();
        } catch (...) {
            std::error_code skipEc;
            it.increment(skipEc);
            ec.clear();
        }
    }
    return true;
}
