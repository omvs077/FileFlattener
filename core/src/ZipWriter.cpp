#include "ZipWriter.h"
#include <minizip/zip.h>
#include <fstream>
#include <vector>
namespace fs = std::filesystem;
static bool writeMemoryEntry(zipFile zf, const std::string& entryName, const std::string& content, std::string& errorMsg) {
    int openResult = zipOpenNewFileInZip(
        zf, entryName.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr,
        Z_DEFLATED, Z_DEFAULT_COMPRESSION
    );
    if (openResult != ZIP_OK) {
        errorMsg = "Failed to open entry in ZIP for: " + entryName;
        return false;
    }
    if (!content.empty()) {
        int writeResult = zipWriteInFileInZip(zf, content.data(), static_cast<unsigned int>(content.size()));
        if (writeResult != ZIP_OK) {
            errorMsg = "Failed writing data into ZIP for: " + entryName;
            zipCloseFileInZip(zf);
            return false;
        }
    }
    zipCloseFileInZip(zf);
    return true;
}
bool ZipWriter::writeZip(
    const ScanResult& scan,
    const std::vector<FlattenedFile>& flattened,
    const fs::path& outputZipPath,
    std::string& errorMsg,
    const std::string& structureTextContent,
    const std::string& manifestJsonContent,
    ZipProgressCallback onProgress)
{
    zipFile zf = zipOpen(outputZipPath.string().c_str(), APPEND_STATUS_CREATE);
    if (!zf) {
        errorMsg = "Failed to create ZIP file at: " + outputZipPath.string();
        return false;
    }
    if (!structureTextContent.empty()) {
        if (!writeMemoryEntry(zf, "structure.txt", structureTextContent, errorMsg)) {
            zipClose(zf, nullptr);
            return false;
        }
    }
    if (!manifestJsonContent.empty()) {
        if (!writeMemoryEntry(zf, "manifest.json", manifestJsonContent, errorMsg)) {
            zipClose(zf, nullptr);
            return false;
        }
    }
    constexpr size_t kChunkSize = 64 * 1024;
    std::vector<char> buffer(kChunkSize);
    uint64_t totalBytes = scan.totalSizeBytes;
    uint64_t bytesWritten = 0;
    for (const auto& ff : flattened) {
        const ScannedFile& src = scan.files[ff.originalIndex];
        int openResult = zipOpenNewFileInZip(
            zf, ff.flattenedName.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr,
            Z_DEFLATED, Z_DEFAULT_COMPRESSION
        );
        if (openResult != ZIP_OK) {
            errorMsg = "Failed to open entry in ZIP for: " + ff.flattenedName;
            zipClose(zf, nullptr);
            return false;
        }
        std::ifstream in(src.absolutePath, std::ios::binary);
        if (!in) {
            errorMsg = "Failed to open source file for reading: " + src.absolutePath.string();
            zipCloseFileInZip(zf);
            zipClose(zf, nullptr);
            return false;
        }
        while (in.read(buffer.data(), kChunkSize) || in.gcount() > 0) {
            std::streamsize bytesRead = in.gcount();
            if (bytesRead > 0) {
                int writeResult = zipWriteInFileInZip(zf, buffer.data(), static_cast<unsigned int>(bytesRead));
                if (writeResult != ZIP_OK) {
                    errorMsg = "Failed writing data into ZIP for: " + ff.flattenedName;
                    zipCloseFileInZip(zf);
                    zipClose(zf, nullptr);
                    return false;
                }
                bytesWritten += static_cast<uint64_t>(bytesRead);
                if (onProgress) onProgress(bytesWritten, totalBytes, ff.flattenedName);
            }
            if (in.eof()) break;
        }
        zipCloseFileInZip(zf);
    }
    zipClose(zf, nullptr);
    return true;
}
