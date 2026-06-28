#include "ZipWriter.h"
#include <minizip/zip.h>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

bool ZipWriter::writeZip(
    const ScanResult& scan,
    const std::vector<FlattenedFile>& flattened,
    const fs::path& outputZipPath,
    std::string& errorMsg)
{
    zipFile zf = zipOpen(outputZipPath.string().c_str(), APPEND_STATUS_CREATE);
    if (!zf) {
        errorMsg = "Failed to create ZIP file at: " + outputZipPath.string();
        return false;
    }

    constexpr size_t kChunkSize = 64 * 1024; // 64 KB streaming buffer
    std::vector<char> buffer(kChunkSize);

    for (const auto& ff : flattened) {
        const ScannedFile& src = scan.files[ff.originalIndex];

        int openResult = zipOpenNewFileInZip(
            zf,
            ff.flattenedName.c_str(),
            nullptr,            // zip_fileinfo (default timestamps)
            nullptr, 0,         // extrafield_local
            nullptr, 0,         // extrafield_global
            nullptr,            // comment
            Z_DEFLATED,
            Z_DEFAULT_COMPRESSION
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
            }
            if (in.eof()) break;
        }

        zipCloseFileInZip(zf);
    }

    zipClose(zf, nullptr);
    return true;
}
