#pragma once

#include <string>

#include "../types.h"

namespace dw {

// Forward declaration - actual definition in config.h
enum class FileHandlingMode : int;

// Handles post-import file operations (copy/move/leave-in-place)
class FileHandler {
  public:
    // Handle an imported file according to mode.
    // Returns the final file path (original if leave-in-place, new path if copy/move).
    // On error, returns empty path and sets error string.
    static Path handleImportedFile(const Path& source, FileHandlingMode mode,
                                    const Path& libraryRoot, std::string& error);

    // Ensure library directory exists (creates if needed)
    static bool ensureLibraryDir(const Path& libraryRoot);

    // Get default library directory (data dir / "library")
    static Path defaultLibraryDir();

  private:
    // Generate unique destination path (avoids overwrite)
    static Path uniqueDestination(const Path& dir, const Path& filename);
};

} // namespace dw
