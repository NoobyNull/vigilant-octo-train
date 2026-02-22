#include "storage_manager.h"

#include "../mesh/hash.h"
#include "../paths/app_paths.h"
#include "../utils/log.h"

namespace dw {

StorageManager::StorageManager(const Path& blobRoot)
    : m_blobRoot(blobRoot), m_tempDir(blobRoot / ".tmp") {}

Path StorageManager::blobPath(const std::string& hash,
                              const std::string& ext) const {
    if (hash.size() < 4) {
        return Path();
    }
    // hash[0..1]/hash[2..3]/hash.ext
    return m_blobRoot / hash.substr(0, 2) / hash.substr(2, 2) /
           (hash + "." + ext);
}

Path StorageManager::storeFile(const Path& source, const std::string& hash,
                               const std::string& ext, std::string& error) {
    try {
        // Dedup: if blob already exists, return path (no-op)
        Path finalPath = blobPath(hash, ext);
        if (finalPath.empty()) {
            error = "Invalid hash: must be at least 4 characters";
            return Path();
        }
        if (fs::exists(finalPath)) {
            return finalPath;
        }

        // Create temp dir if needed
        fs::create_directories(m_tempDir);

        // Generate temp file path
        Path tmpPath = m_tempDir / ("import_" + hash + "." + ext);

        // Copy source to temp
        fs::copy_file(source, tmpPath,
                      fs::copy_options::overwrite_existing);

        // Verify hash
        std::string computedHash = hash::computeFile(tmpPath);
        if (computedHash != hash) {
            fs::remove(tmpPath);
            error = "Hash verification failed: expected " + hash +
                    ", got " + computedHash;
            return Path();
        }

        // Create destination directories
        fs::create_directories(finalPath.parent_path());

        // Atomic rename
        fs::rename(tmpPath, finalPath);

        return finalPath;

    } catch (const fs::filesystem_error& e) {
        error = std::string("Filesystem error: ") + e.what();
        return Path();
    }
}

Path StorageManager::moveFile(const Path& source, const std::string& hash,
                              const std::string& ext, std::string& error) {
    // Store first (copy to blob store)
    Path result = storeFile(source, hash, ext, error);
    if (result.empty()) {
        return result;
    }

    // Try to remove source (best-effort)
    try {
        fs::remove(source);
    } catch (const fs::filesystem_error& e) {
        log::warningf("StorageManager",
                   "Could not remove source after move: %s", e.what());
        // Not a failure -- the blob is stored correctly
    }

    return result;
}

bool StorageManager::exists(const std::string& hash,
                            const std::string& ext) const {
    Path p = blobPath(hash, ext);
    if (p.empty()) {
        return false;
    }
    return fs::exists(p);
}

bool StorageManager::remove(const std::string& hash,
                            const std::string& ext) {
    try {
        Path p = blobPath(hash, ext);
        if (p.empty()) {
            return true;
        }
        // fs::remove returns true if file was removed, false if didn't exist
        // Either way, the blob is gone -- return true
        fs::remove(p);
        return true;
    } catch (const fs::filesystem_error& e) {
        log::errorf("StorageManager", "Failed to remove blob: %s",
                    e.what());
        return false;
    }
}

int StorageManager::cleanupOrphanedTempFiles() {
    int count = 0;
    try {
        if (!fs::exists(m_tempDir)) {
            return 0;
        }
        for (const auto& entry : fs::directory_iterator(m_tempDir)) {
            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (!ec) {
                ++count;
            }
        }
        if (count > 0) {
            log::infof("StorageManager",
                       "Cleaned up %d orphaned temp file(s)", count);
        }
    } catch (const fs::filesystem_error& e) {
        log::errorf("StorageManager",
                    "Error cleaning up orphaned temp files: %s",
                    e.what());
    }
    return count;
}

Path StorageManager::defaultBlobRoot() {
    return paths::getBlobStoreDir();
}

} // namespace dw
