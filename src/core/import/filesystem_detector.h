#pragma once

// Digital Workshop - Filesystem Detection Utility
// Detects whether a file path resides on a local or network filesystem.

#include <filesystem>
#include <string>

#include "../types.h"

namespace dw {

// Classification of storage location
enum class StorageLocation { Local, Network, Unknown };

// Result of filesystem detection
struct FilesystemInfo {
    StorageLocation location = StorageLocation::Unknown;
    std::string fsTypeName;
};

// Detect the filesystem type for a given path.
// Walks up parent directories if the path doesn't exist yet.
// Returns Unknown on unsupported platforms or if detection fails.
FilesystemInfo detectFilesystem(const Path& path);

} // namespace dw
