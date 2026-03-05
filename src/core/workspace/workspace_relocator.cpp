#include "workspace_relocator.h"

#include "../config/config.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

int WorkspaceRelocator::countFiles(const Path& sourceRoot) {
    int count = 0;
    for (const char* category : kCategories) {
        Path categoryDir = sourceRoot / category;
        std::error_code ec;
        if (!fs::is_directory(categoryDir, ec))
            continue;
        for (auto it = fs::recursive_directory_iterator(categoryDir, ec);
             it != fs::recursive_directory_iterator();
             it.increment(ec)) {
            if (it->is_regular_file(ec))
                ++count;
        }
    }
    return count;
}

std::vector<std::string> WorkspaceRelocator::getOverriddenCategories() {
    std::vector<std::string> overridden;
    auto& cfg = Config::instance();

    // Check each category's raw config value (empty = derived from workspace root)
    // Access the raw member through the public getter and compare with the
    // effective workspace root to detect overrides
    Path wsRoot = cfg.getEffectiveWorkspaceRoot();
    if (cfg.getModelsDir() != wsRoot / "Models")
        overridden.emplace_back("Models");
    if (cfg.getProjectsDir() != wsRoot / "Projects")
        overridden.emplace_back("Projects");
    if (cfg.getMaterialsDir() != wsRoot / "Materials")
        overridden.emplace_back("Materials");
    if (cfg.getGCodeDir() != wsRoot / "GCode")
        overridden.emplace_back("GCode");
    if (cfg.getSupportDir() != wsRoot / "Support")
        overridden.emplace_back("Support");

    return overridden;
}

WorkspaceRelocator::Result WorkspaceRelocator::relocate(const Options& opts,
                                                         ProgressCallback onProgress,
                                                         CancelCheck isCancelled) {
    Result result;

    // Sanity: source == dest is a no-op
    std::error_code ec;
    if (fs::equivalent(opts.sourceRoot, opts.destRoot, ec)) {
        result.success = true;
        return result;
    }

    // Determine which categories to skip (individually overridden)
    auto overridden = getOverriddenCategories();

    for (const char* category : kCategories) {
        // Skip categories with individual overrides
        bool skip = false;
        for (const auto& ov : overridden) {
            if (ov == category) {
                skip = true;
                break;
            }
        }
        if (skip) {
            result.skippedCategories.emplace_back(category);
            continue;
        }

        Path srcDir = opts.sourceRoot / category;
        Path dstDir = opts.destRoot / category;

        // Skip if source category doesn't exist
        if (!fs::is_directory(srcDir, ec))
            continue;

        // Create destination category directory
        if (!file::createDirectories(dstDir)) {
            result.error = "Failed to create directory: " + dstDir.string();
            return result;
        }
        ++result.dirsCreated;

        // Walk source recursively
        for (auto it = fs::recursive_directory_iterator(srcDir, ec);
             it != fs::recursive_directory_iterator();
             it.increment(ec)) {

            if (isCancelled && isCancelled()) {
                result.error = "Cancelled by user";
                return result;
            }

            auto relPath = fs::relative(it->path(), srcDir, ec);
            if (ec) {
                result.error = "Failed to compute relative path: " + it->path().string();
                return result;
            }

            Path dstPath = dstDir / relPath;

            if (it->is_directory(ec)) {
                if (!file::createDirectories(dstPath)) {
                    result.error = "Failed to create directory: " + dstPath.string();
                    return result;
                }
                ++result.dirsCreated;
            } else if (it->is_regular_file(ec)) {
                // Skip if destination already exists
                if (file::exists(dstPath)) {
                    ++result.filesSkipped;
                    continue;
                }

                if (onProgress) {
                    onProgress(relPath.string());
                }

                // Ensure parent directory exists
                Path parentDir = dstPath.parent_path();
                if (!file::exists(parentDir)) {
                    if (!file::createDirectories(parentDir)) {
                        result.error = "Failed to create directory: " + parentDir.string();
                        return result;
                    }
                    ++result.dirsCreated;
                }

                // Copy the file
                if (!file::copy(it->path(), dstPath)) {
                    result.error = "Failed to copy: " + it->path().string();
                    return result;
                }

                // If move mode, delete the source file after successful copy
                if (opts.moveFiles) {
                    if (!file::remove(it->path())) {
                        log::warningf("Relocator",
                                   "Copied but failed to remove source: %s",
                                   it->path().string().c_str());
                        // Non-fatal — file is safely at destination
                    }
                }

                ++result.filesCopied;
            }
        }

        if (ec) {
            result.error = "Error iterating directory: " + srcDir.string() +
                           " (" + ec.message() + ")";
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace dw
