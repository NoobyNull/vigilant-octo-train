---
phase: 02-content-addressable-storage
plan: 01
subsystem: storage
tags: [cas, blob-store, atomic-write, dedup, filesystem]

requires:
  - phase: v1.0
    provides: hash::computeFile(), app_paths infrastructure
provides:
  - StorageManager class with atomic temp+verify+rename writes
  - getBlobStoreDir() and getTempStoreDir() path functions
  - Content deduplication by hash
  - Orphan temp file cleanup
affects: [02-02, 02-03]

tech-stack:
  added: []
  patterns: [content-addressable-storage, atomic-write, temp-verify-rename]

key-files:
  created:
    - src/core/storage/storage_manager.h
    - src/core/storage/storage_manager.cpp
    - tests/test_storage_manager.cpp
  modified:
    - src/core/paths/app_paths.h
    - src/core/paths/app_paths.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Blob paths use 2-byte hash prefix sharding: hash[0..1]/hash[2..3]/hash.ext"
  - "Temp dir co-located with blob root at .tmp/ for same-filesystem atomic rename"
  - "moveFile falls back to copy+delete with warning on cross-filesystem"
  - "Hash validation requires minimum 4 characters"

patterns-established:
  - "Atomic write: copy to temp, verify hash, rename to final path"
  - "Dedup: skip store if blob already exists at hash path"

metrics:
  duration: "3m 5s"
  tasks_completed: 2
  tasks_total: 2
  tests_added: 9
  tests_passing: 9
  completed: "2026-02-22"
---

# Phase 02 Plan 01: StorageManager and CAS Blob Paths Summary

StorageManager with atomic temp+verify+rename writes, 2-byte hash prefix blob paths, dedup, and orphan cleanup

## Tasks Completed

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Add blob store paths and create StorageManager | ac72203 | storage_manager.h/.cpp, app_paths.h/.cpp |
| 2 | Add to CMake and write unit tests | 9159bb5 | test_storage_manager.cpp, CMakeLists.txt |

## Implementation Details

### StorageManager API

- `blobPath(hash, ext)` -- Pure path computation: `blobRoot/ab/cd/abcdef...1234.stl`
- `storeFile(source, hash, ext, error)` -- Atomic copy via temp+verify+rename, dedup on existing
- `moveFile(source, hash, ext, error)` -- Store + remove source (best-effort remove)
- `exists(hash, ext)` / `remove(hash, ext)` -- Blob existence and removal
- `cleanupOrphanedTempFiles()` -- Startup cleanup of crash leftovers in .tmp/
- `defaultBlobRoot()` -- Returns `paths::getBlobStoreDir()`

### Path Functions Added to app_paths

- `getBlobStoreDir()` -- `getDataDir() / "blobs"`
- `getTempStoreDir()` -- `getBlobStoreDir() / ".tmp"`
- Both added to `ensureDirectoriesExist()`

## Test Coverage

9 tests covering:
1. BlobPathComputation -- correct prefix sharding
2. BlobPathShortHash -- rejection of invalid short hashes
3. StoreFileBasic -- atomic store with temp cleanup
4. StoreFileDedup -- idempotent second store
5. StoreFileHashMismatch -- verification failure cleanup
6. MoveFileBasic -- store + source removal
7. ExistsCheck -- before/after store
8. RemoveBlob -- deletion and verification
9. CleanupOrphanedTempFiles -- crash recovery simulation

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed log function name**
- **Found during:** Task 1 build verification
- **Issue:** Used `log::warnf()` which doesn't exist; correct name is `log::warningf()`
- **Fix:** Changed to `log::warningf()`
- **Files modified:** src/core/storage/storage_manager.cpp
- **Commit:** ac72203

## Self-Check: PASSED

- All 3 created files exist on disk
- Both commits (ac72203, 9159bb5) verified in git log
- All 9 tests passing
