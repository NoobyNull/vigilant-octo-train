#pragma once

#include <string>

#include "../types.h"

namespace dw {

/// Content-addressable blob storage manager.
/// Files are stored at hash-derived paths: blobRoot/ab/cd/abcdef...1234.ext
/// Writes use atomic temp+verify+rename pattern for crash safety.
class StorageManager {
  public:
    explicit StorageManager(const Path& blobRoot);

    /// Pure path computation -- no I/O.
    /// Returns blobRoot/hash[0..1]/hash[2..3]/hash.ext
    /// Returns empty path if hash length < 4.
    Path blobPath(const std::string& hash, const std::string& ext) const;

    /// Copy source into blob store via temp+verify+rename.
    /// Idempotent: no-op if blob already exists (dedup).
    /// Returns final blob path on success, empty path + sets error on failure.
    Path storeFile(const Path& source,
                   const std::string& hash,
                   const std::string& ext,
                   std::string& error);

    /// Move source into blob store. Falls back to copy+delete if
    /// cross-filesystem.
    Path moveFile(const Path& source,
                  const std::string& hash,
                  const std::string& ext,
                  std::string& error);

    /// Check if blob exists at hash path.
    bool exists(const std::string& hash, const std::string& ext) const;

    /// Remove blob file. Returns true if removed or didn't exist.
    bool remove(const std::string& hash, const std::string& ext);

    /// Clean up orphaned temp files from prior crashes.
    /// Call on startup. Returns count of files cleaned.
    int cleanupOrphanedTempFiles();

    /// Default root: paths::getBlobStoreDir()
    static Path defaultBlobRoot();

  private:
    Path m_blobRoot;
    Path m_tempDir;
};

} // namespace dw
