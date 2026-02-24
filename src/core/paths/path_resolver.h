#pragma once

#include "../types.h"

namespace dw {

class Config;

// Category of user-visible content directory
enum class PathCategory {
    Models,
    Projects,
    Materials,
    GCode,
    Support, // CAS blob store and internal support files
};

// Single choke point for all path read/write operations.
// Stored DB paths are either relative (to their category root) or absolute (external files).
namespace PathResolver {

// Get the current configured root directory for a category.
Path categoryRoot(PathCategory cat);

// Resolve a stored DB path to an absolute filesystem path.
// If storedPath is absolute → return as-is.
// If relative → prepend the category root.
Path resolve(const Path& storedPath, PathCategory cat);

// Convert an absolute filesystem path to a storable DB path.
// If absolutePath is under the category root → return relative.
// Otherwise → return the absolute path unchanged.
Path makeStorable(const Path& absolutePath, PathCategory cat);

} // namespace PathResolver
} // namespace dw
