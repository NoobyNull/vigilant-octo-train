---
phase: 02-content-addressable-storage
plan: 02
subsystem: import-pipeline
tags: [cas, import, blob-store, application-lifecycle, orphan-cleanup]

requires:
  - phase: 02-01
    provides: StorageManager class, blobPath(), storeFile(), moveFile(), cleanupOrphanedTempFiles()
provides:
  - ImportQueue CAS integration via StorageManager injection
  - Application-level StorageManager ownership and startup orphan cleanup
  - Atomic blob writes during import with DB rollback on failure
affects: [02-03]

tech-stack:
  added: []
  patterns: [dependency-injection, db-rollback-on-storage-failure, pre-insert-path-resolution]

key-files:
  created: []
  modified:
    - src/core/import/import_queue.h
    - src/core/import/import_queue.cpp
    - src/app/application.h
    - src/app/application.cpp

key-decisions:
  - "Set filePath to blob path BEFORE DB insert to eliminate crash window where record points to non-existent path"
  - "Roll back DB record (delete) if blob store write fails after insert"
  - "Preserve FileHandler fallback when StorageManager is null for backward compatibility"
  - "StorageManager constructed after ensureDirectoriesExist() and before ImportQueue in Application::init()"

patterns-established:
  - "Pre-insert path resolution: compute final storage path before DB write"
  - "Storage failure rollback: delete DB record if filesystem write fails"
  - "Optional dependency injection: null StorageManager falls back to legacy behavior"

metrics:
  duration: "2m 32s"
  tasks_completed: 2
  tasks_total: 2
  tests_added: 0
  tests_passing: 534
  completed: "2026-02-22"
---

# Phase 02 Plan 02: Wire StorageManager into Import Pipeline Summary

CAS blob storage wired into ImportQueue stages 5/5.5 and Application lifecycle with atomic writes, DB rollback, and startup orphan cleanup

## Tasks Completed

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Wire StorageManager into ImportQueue Stage 5/5.5 | 382e8cb | import_queue.h, import_queue.cpp |
| 2 | Wire StorageManager into Application lifecycle | 556cf34 | application.h, application.cpp |

## Implementation Details

### ImportQueue Changes

- Constructor accepts optional `StorageManager*` (backward compatible, defaults to nullptr)
- **Stage 5 (Inserting):** `record.filePath` set to `m_storageManager->blobPath()` BEFORE DB insert when StorageManager present and mode is not LeaveInPlace. Eliminates crash window where DB record points to non-existent file.
- **Stage 5.5 (File Handling):** Uses `m_storageManager->storeFile()` (CopyToLibrary) or `m_storageManager->moveFile()` (MoveToLibrary) for atomic CAS writes. On failure, rolls back DB record via `modelRepo.remove()` / `gcodeRepo.remove()`.
- **Fallback:** When `m_storageManager` is null, falls back to legacy `FileHandler::handleImportedFile()` for full backward compatibility.
- **LeaveInPlace:** Unchanged -- no blob store interaction, filePath stays as source path.
- **GCode support:** Same CAS pattern applied to GCode import path.

### Application Changes

- `Application` owns `std::unique_ptr<StorageManager> m_storageManager`
- Constructed after `paths::ensureDirectoriesExist()` with `StorageManager::defaultBlobRoot()`
- `cleanupOrphanedTempFiles()` called on startup (STOR-03)
- Passed to ImportQueue constructor via `.get()`
- Added to shutdown sequence in correct reverse order

## Deviations from Plan

None -- plan executed exactly as written.

## Self-Check: PASSED
