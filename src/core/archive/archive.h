#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dw {

// Archive entry metadata
struct ArchiveEntry {
    std::string path;
    uint64_t uncompressedSize = 0;
    uint64_t compressedSize = 0;
    bool isDirectory = false;
};

// Result of archive operations
struct ArchiveResult {
    bool success = false;
    std::string error;
    std::vector<std::string> files;

    static ArchiveResult ok(std::vector<std::string> files = {}) {
        return {true, "", std::move(files)};
    }

    static ArchiveResult fail(const std::string& error) {
        return {false, error, {}};
    }
};

// Archive format for project export
class ProjectArchive {
public:
    // Create a new archive for writing
    static ArchiveResult create(const std::string& archivePath,
                                 const std::string& projectDir);

    // Extract an archive
    static ArchiveResult extract(const std::string& archivePath,
                                  const std::string& outputDir);

    // List contents of an archive
    static std::vector<ArchiveEntry> list(const std::string& archivePath);

    // Check if a file is a valid project archive
    static bool isValidArchive(const std::string& archivePath);

    // Archive file extension
    static constexpr const char* Extension = ".dwp";
    static constexpr const char* MimeType = "application/x-digitalworkshop-project";
};

}  // namespace dw
