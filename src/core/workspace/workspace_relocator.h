#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../types.h"

namespace dw {

// Relocates workspace files from one root directory to another.
// Standalone utility — no UI dependencies.
class WorkspaceRelocator {
  public:
    struct Options {
        Path sourceRoot;
        Path destRoot;
        bool moveFiles = true; // true = move (copy + delete), false = copy only
    };

    struct Result {
        bool success = false;
        int filesCopied = 0;
        int filesSkipped = 0; // already existed at destination
        int dirsCreated = 0;
        std::string error;
        std::vector<std::string> skippedCategories; // categories with individual overrides
    };

    using ProgressCallback = std::function<void(const std::string& currentFile)>;
    using CancelCheck = std::function<bool()>;

    // Category subdirectories that get relocated
    static constexpr const char* kCategories[] = {
        "Models", "Projects", "Materials", "GCode", "Support"};

    // Count total files across all category subdirs (for progress bar)
    static int countFiles(const Path& sourceRoot);

    // Execute the relocation. Thread-safe for use on a background thread.
    static Result relocate(const Options& opts,
                           ProgressCallback onProgress,
                           CancelCheck isCancelled);

    // Check which categories have individual overrides in Config
    // (these should be skipped during relocation)
    static std::vector<std::string> getOverriddenCategories();
};

} // namespace dw
