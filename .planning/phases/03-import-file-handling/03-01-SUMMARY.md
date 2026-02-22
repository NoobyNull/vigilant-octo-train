---
phase: 03-import-file-handling
plan: 01
subsystem: import-pipeline
tags: [filesystem-detection, import-dialog, nas-detection, cross-platform]

requires: []
provides:
  - FilesystemDetector utility with StorageLocation enum and detectFilesystem() function
  - ImportOptionsDialog with radio buttons for LeaveInPlace/CopyToLibrary/MoveToLibrary
  - Auto-detection of network filesystems with recommendation UI
affects: [03-02]

tech-stack:
  added: []
  patterns: [platform-ifdef-dispatch, parent-walk-for-nonexistent-paths, auto-recommend-from-detection]

key-files:
  created:
    - src/core/import/filesystem_detector.h
    - src/core/import/filesystem_detector.cpp
    - src/ui/dialogs/import_options_dialog.h
    - src/ui/dialogs/import_options_dialog.cpp
    - tests/test_filesystem_detector.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Single cpp file with #ifdef platform dispatch instead of separate per-platform files"
  - "Walk up parent directories to find existing ancestor when path doesn't exist yet"
  - "FUSE mounts classified as Network (covers rclone, sshfs, cloud FUSE drivers)"
  - "Default to MoveToLibrary for local, CopyToLibrary for network sources"

patterns-established:
  - "Platform-specific code via #ifdef in single source file for simple utilities"
  - "Filesystem detection with ancestor walking for paths that will be created"
  - "Dialog auto-recommendation based on detected storage location"

metrics:
  duration: "2m 37s"
  completed: "2026-02-22"
  tasks_completed: 2
  tasks_total: 2
  tests_added: 5
  files_created: 5
  files_modified: 2
---

# Phase 03 Plan 01: Filesystem Detector & Import Options Dialog Summary

Cross-platform filesystem detection utility with NAS/network auto-detection and ImGui import options dialog with smart defaults.

## Task Completion

| Task | Name | Commit | Key Files |
|------|------|--------|-----------|
| 1 | Filesystem detector with cross-platform detection and tests | 6a52a53 | filesystem_detector.h/.cpp, test_filesystem_detector.cpp |
| 2 | Import options dialog with radio buttons and NAS recommendation | c4a144f | import_options_dialog.h/.cpp |

## What Was Built

### FilesystemDetector (Task 1)
- `StorageLocation` enum: Local, Network, Unknown
- `FilesystemInfo` struct with location and fsTypeName
- `detectFilesystem()` free function in `namespace dw`
- Linux: `statfs(2)` with magic number matching for NFS (0x6969), SMB (0x517B), CIFS (0xFF534D42), SMB2 (0xFE534D42), FUSE (0x65735546)
- macOS: `statfs` with `f_fstypename` string matching for nfs, smbfs, afpfs, webdav
- Windows: `GetDriveTypeW()` checking for `DRIVE_REMOTE`
- Ancestor walking for non-existent paths (walks up until finding existing parent)
- 5 unit tests all passing

### ImportOptionsDialog (Task 2)
- Extends `Dialog` base class with ImGui modal popup
- `open(paths)` runs `detectFilesystem()` on first path's parent
- Three radio buttons matching `FileHandlingMode` enum values
- Network detection triggers yellow warning banner and auto-selects CopyToLibrary
- Extra red warning if user selects MoveToLibrary on network source
- ResultCallback delivers selected mode + paths on confirm
- Cancel button closes without callback

## Deviations from Plan

None - plan executed exactly as written.

## Verification Results

- Full build: PASSED (both digital_workshop and dw_tests)
- Filesystem detector tests: 5/5 PASSED
- File sizes within limits: header 27 lines, cpp 141 lines, dialog header 38 lines, dialog cpp 139 lines
- No new external dependencies (platform syscalls only)

## Self-Check: PASSED

- All 5 created files exist on disk
- Commit 6a52a53 (Task 1) verified in git log
- Commit c4a144f (Task 2) verified in git log
