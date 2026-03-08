#pragma once

#include <string>
#include <vector>

#include "../types.h"

namespace dw {

class ModelRepository;
class GCodeRepository;

// A record with a missing file on disk
struct MissingFile {
    i64 id = 0;
    std::string name;
    Path storedPath;     // Path as stored in DB
    Path resolvedPath;   // Absolute path after resolution
    bool isGCode = false;
};

// A recovered file mapping
struct RecoveredFile {
    i64 id = 0;
    std::string name;
    Path oldPath;
    Path newPath;
    bool isGCode = false;
};

// Scans the database for records whose files no longer exist on disk,
// and attempts to relocate them using path-delta inference.
class PathRecovery {
  public:
    PathRecovery(ModelRepository& modelRepo, GCodeRepository& gcodeRepo);

    // Scan for all records whose files are missing from disk.
    std::vector<MissingFile> findMissing();

    // Given one relocated file (user pointed to its new location),
    // derive a path mapping and attempt to find all other missing files.
    // Returns the list of files that were found at predicted locations.
    std::vector<RecoveredFile> recoverFromRelocated(
        const MissingFile& relocated, const Path& newLocation,
        const std::vector<MissingFile>& allMissing);

    // Apply recovered paths to the database.
    // Returns the number of records updated.
    int applyRecoveries(const std::vector<RecoveredFile>& recoveries);

  private:
    ModelRepository& m_modelRepo;
    GCodeRepository& m_gcodeRepo;
};

} // namespace dw
