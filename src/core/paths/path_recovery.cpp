#include "path_recovery.h"

#include <filesystem>

#include "../database/gcode_repository.h"
#include "../database/model_repository.h"
#include "../utils/log.h"
#include "path_resolver.h"

namespace dw {
namespace fs = std::filesystem;

PathRecovery::PathRecovery(ModelRepository& modelRepo, GCodeRepository& gcodeRepo)
    : m_modelRepo(modelRepo), m_gcodeRepo(gcodeRepo) {}

std::vector<MissingFile> PathRecovery::findMissing() {
    std::vector<MissingFile> missing;

    auto models = m_modelRepo.findAll();
    for (const auto& model : models) {
        Path resolved = PathResolver::resolve(model.filePath, PathCategory::Models);
        if (!resolved.empty() && !fs::exists(resolved)) {
            MissingFile mf;
            mf.id = model.id;
            mf.name = model.name;
            mf.storedPath = model.filePath;
            mf.resolvedPath = resolved;
            mf.isGCode = false;
            missing.push_back(std::move(mf));
        }
    }

    auto gcodes = m_gcodeRepo.findAll();
    for (const auto& gc : gcodes) {
        Path resolved = PathResolver::resolve(gc.filePath, PathCategory::GCode);
        if (!resolved.empty() && !fs::exists(resolved)) {
            MissingFile mf;
            mf.id = gc.id;
            mf.name = gc.name;
            mf.storedPath = gc.filePath;
            mf.resolvedPath = resolved;
            mf.isGCode = true;
            missing.push_back(std::move(mf));
        }
    }

    return missing;
}

namespace {

// Split a path into its components.
std::vector<std::string> pathComponents(const Path& p) {
    std::vector<std::string> parts;
    for (auto it = p.begin(); it != p.end(); ++it)
        parts.push_back(it->string());
    return parts;
}

// Derive old→new prefix mapping from a known relocation.
// Returns {oldPrefix, newPrefix} by finding the common suffix from both paths.
std::pair<Path, Path> derivePathMapping(const Path& oldResolved, const Path& newResolved) {
    auto oldParts = pathComponents(oldResolved);
    auto newParts = pathComponents(newResolved);

    int oi = static_cast<int>(oldParts.size()) - 1;
    int ni = static_cast<int>(newParts.size()) - 1;
    while (oi >= 0 && ni >= 0 &&
           oldParts[static_cast<size_t>(oi)] == newParts[static_cast<size_t>(ni)]) {
        --oi;
        --ni;
    }

    Path oldPrefix;
    for (int i = 0; i <= oi; ++i)
        oldPrefix /= oldParts[static_cast<size_t>(i)];

    Path newPrefix;
    for (int i = 0; i <= ni; ++i)
        newPrefix /= newParts[static_cast<size_t>(i)];

    return {oldPrefix, newPrefix};
}

// Try to find a missing file by substituting the old prefix with the new prefix.
// Returns empty path if not found.
Path tryPrefixRecovery(const MissingFile& mf, const Path& oldPrefix, const Path& newPrefix) {
    std::string oldPathStr = mf.resolvedPath.string();
    std::string oldPrefixStr = oldPrefix.string();

    if (oldPrefixStr.empty())
        return {};

    if (oldPathStr.substr(0, oldPrefixStr.size()) != oldPrefixStr)
        return {};

    Path candidate = newPrefix / Path(oldPathStr.substr(oldPrefixStr.size()));
    if (fs::exists(candidate))
        return candidate;

    return {};
}

// Try to find a missing file by searching recursively from a directory.
Path tryFilenameSearch(const MissingFile& mf, const Path& searchDir) {
    std::string targetFilename = mf.resolvedPath.filename().string();
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                 searchDir, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && entry.path().filename().string() == targetFilename)
                return entry.path();
        }
    } catch (const fs::filesystem_error&) {
        // Permission denied or broken symlink — skip
    }
    return {};
}

} // anonymous namespace

std::vector<RecoveredFile> PathRecovery::recoverFromRelocated(
    const MissingFile& relocated, const Path& newLocation,
    const std::vector<MissingFile>& allMissing) {

    std::vector<RecoveredFile> recovered;
    Path newResolved = fs::canonical(newLocation);

    auto [oldPrefix, newPrefix] = derivePathMapping(relocated.resolvedPath, newResolved);
    log::infof("Recovery", "Path mapping: %s -> %s",
               oldPrefix.string().c_str(), newPrefix.string().c_str());

    Path searchDir = newResolved.parent_path();

    for (const auto& mf : allMissing) {
        if (mf.id == relocated.id && mf.isGCode == relocated.isGCode)
            continue;

        Path found = tryPrefixRecovery(mf, oldPrefix, newPrefix);
        if (found.empty())
            found = tryFilenameSearch(mf, searchDir);

        if (!found.empty()) {
            RecoveredFile rf;
            rf.id = mf.id;
            rf.name = mf.name;
            rf.oldPath = mf.resolvedPath;
            rf.newPath = found;
            rf.isGCode = mf.isGCode;
            recovered.push_back(std::move(rf));
        }
    }

    // Add the user-relocated file itself
    RecoveredFile rf;
    rf.id = relocated.id;
    rf.name = relocated.name;
    rf.oldPath = relocated.resolvedPath;
    rf.newPath = newResolved;
    rf.isGCode = relocated.isGCode;
    recovered.insert(recovered.begin(), std::move(rf));

    return recovered;
}

int PathRecovery::applyRecoveries(const std::vector<RecoveredFile>& recoveries) {
    int updated = 0;
    for (const auto& rf : recoveries) {
        if (rf.isGCode) {
            auto record = m_gcodeRepo.findById(rf.id);
            if (record) {
                record->filePath = PathResolver::makeStorable(rf.newPath, PathCategory::GCode);
                if (m_gcodeRepo.update(*record))
                    ++updated;
            }
        } else {
            auto record = m_modelRepo.findById(rf.id);
            if (record) {
                record->filePath = PathResolver::makeStorable(rf.newPath, PathCategory::Models);
                if (m_modelRepo.update(*record))
                    ++updated;
            }
        }
    }

    log::infof("Recovery", "Applied %d path recoveries", updated);
    return updated;
}

} // namespace dw
