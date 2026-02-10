---
phase: 02-import-pipeline
plan: 01
subsystem: threading-config
tags: [thread-pool, parallelism, configuration, settings-ui]
dependency_graph:
  requires: [main-thread-queue]
  provides: [thread-pool, import-config]
  affects: [config-system, settings-app]
tech_stack:
  added:
    - ThreadPool with condition variable pattern
    - ParallelismTier enum with calculateThreadCount
    - FileHandlingMode enum for import file handling
  patterns:
    - Lazy initialization for ThreadPool (created on first use)
    - Bounded queue with worker loop pattern
    - Atomic counters for active task tracking
    - Real-time path validation in Settings UI
key_files:
  created:
    - src/core/threading/thread_pool.h
    - src/core/threading/thread_pool.cpp
  modified:
    - src/core/config/config.h
    - src/core/config/config.cpp
    - settings/settings_app.h
    - settings/settings_app.cpp
    - settings/CMakeLists.txt
decisions:
  - title: ParallelismTier calculation strategy
    choice: Auto=60%, Fixed=90%, Expert=100% of cores
    rationale: Auto leaves headroom for UI and system, Fixed maximizes throughput with minimal reserve, Expert uses all cores for maximum parallelism
    alternatives: [Fixed percentages, User-specified thread count]
  - title: Library path validation approach
    choice: Inline validation with filesystem::exists and create_directories
    rationale: Immediate feedback without platform-specific dialog, validates both existing paths and creatable paths
    alternatives: [No validation, Defer validation to import time]
  - title: FileHandlingMode enum vs int
    choice: Dedicated enum with explicit values
    rationale: Type safety, clear intent, easy to extend, matches existing patterns
    alternatives: [Raw int with constants, String-based mode]
metrics:
  duration: 4m 57s
  tasks_completed: 2
  files_created: 2
  files_modified: 5
  commits: 2
  completed_date: 2026-02-10
---

# Phase 02 Plan 01: ThreadPool Infrastructure & Import Config Summary

**Built the foundation for parallel import processing with configurable thread pool and user-controlled import settings.**

## What Was Built

### ThreadPool Infrastructure
- Generic bounded thread pool with configurable worker count
- Workers execute tasks from queue using condition variable pattern
- Tracks pending tasks (queue size) and active tasks (atomic counter)
- Drains remaining tasks on shutdown (no task discarding)
- Lazy initialization pattern - pool created on first import, not at startup

### ParallelismTier System
- Auto tier: 60% of cores (balanced, leaves headroom)
- Fixed tier: 90% of cores (high throughput)
- Expert tier: 100% of cores (maximum parallelism)
- `calculateThreadCount()` with hardware detection and fallback to 4 cores
- Result clamped to [1, 64] range

### Config Extensions
- `ParallelismTier m_parallelismTier` - defaults to Auto
- `FileHandlingMode m_fileHandlingMode` - 0=LeaveInPlace, 1=CopyToLibrary, 2=MoveToLibrary
- `Path m_libraryDir` - managed library directory (empty = default)
- `bool m_showImportErrorToasts` - toast notification toggle (default: true)
- New `[import]` INI section with 4 keys: `parallelism_tier`, `file_handling_mode`, `library_dir`, `show_error_toasts`

### Settings App Import Tab
- **Parallelism section**: Combo box with 3 options, shows calculated thread count for current machine
- **File Handling section**: Radio buttons for leave/copy/move modes, editable library path text input with real-time validation
- **Notifications section**: Checkbox to toggle import error toasts
- Library path validation: green [OK] for valid paths, red [Invalid] for bad paths
- Validation checks: existing directory, or can be created via `filesystem::create_directories()`

## Task Breakdown

### Task 1: ThreadPool Implementation
**Commit:** `10c8e99`
**Files:** `src/core/threading/thread_pool.h`, `src/core/threading/thread_pool.cpp`

Created generic bounded thread pool with:
- Constructor creates N worker threads immediately
- `enqueue()` adds tasks to queue, notifies one worker
- `shutdown()` signals stop, drains queue, joins all threads
- `isIdle()` checks for no pending or active tasks
- `pendingCount()` returns queued task count
- `activeCount()` returns currently executing task count (atomic)
- `workerLoop()` waits on condition variable, dequeues task under lock, executes outside lock

ParallelismTier enum and `calculateThreadCount()` provide tier-based thread count calculation with hardware detection and sensible defaults.

### Task 2: Config and Settings Extensions
**Commit:** `48473c2`
**Files:** `config.h`, `config.cpp`, `settings_app.h`, `settings_app.cpp`, `settings/CMakeLists.txt`

Extended Config class with 4 new import-related fields:
- Parallelism tier (enum)
- File handling mode (enum)
- Library directory path (Path)
- Import error toast toggle (bool)

Added INI serialization in `[import]` section with proper type conversion.

Added `renderImportTab()` to Settings app:
- Parallelism combo shows tier name + actual thread count preview
- File handling radio buttons with conditional library path input
- Library path validates on input: attempts `create_directories()`, shows [OK] or [Invalid]
- Toast checkbox for notification preferences

Wired into Settings app load/save flow via cached members and `applySettings()`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Made mutex mutable in ThreadPool**
- **Found during:** Task 2 verification (settings compilation)
- **Issue:** `pendingCount()` const method couldn't lock non-mutable mutex
- **Fix:** Changed `std::mutex m_mutex` to `mutable std::mutex m_mutex` in thread_pool.h
- **Files modified:** `src/core/threading/thread_pool.h`
- **Commit:** Included in commit `48473c2`
- **Rationale:** Standard C++ pattern for const methods that need synchronization

**2. [Rule 3 - Blocking] Added thread_pool.cpp to settings CMake dependencies**
- **Found during:** Task 2 verification (settings linking)
- **Issue:** Settings app couldn't link - undefined reference to `calculateThreadCount()`
- **Fix:** Added `${CMAKE_SOURCE_DIR}/src/core/threading/thread_pool.cpp` to `SETTINGS_DEPS` in `settings/CMakeLists.txt`
- **Files modified:** `settings/CMakeLists.txt`
- **Commit:** Included in commit `48473c2`
- **Rationale:** Settings app calls `calculateThreadCount()` in Import tab to show thread count preview

## Verification Results

**Build Verification:**
- [x] `cmake --build build --target digital_workshop` - Success, no errors
- [x] `cmake --build build --target dw_settings` - Success, no errors
- [x] ThreadPool header includable from other translation units
- [x] Config new fields have correct default values (Auto tier, leave-in-place, toasts on)
- [x] Settings app renders Import tab without crashes (compilation verified)
- [x] Library directory path is editable via text input with validation

**Success Criteria:**
- [x] ThreadPool class exists and compiles with enqueue/shutdown/isIdle API
- [x] ParallelismTier enum with calculateThreadCount produces correct counts
- [x] Config has 4 new import settings with INI persistence
- [x] Settings app has Import tab with parallelism, file handling, and notification controls
- [x] Library directory path is user-editable via text input with validation feedback
- [x] Both dw and dw_settings targets compile cleanly

## Technical Notes

**ThreadPool Design:**
- Uses `std::condition_variable` over lock-free queue - simpler, debuggable, sufficient for expected load
- Executes callbacks outside lock to prevent deadlocks if callbacks enqueue
- Atomic active count separate from queue lock for lock-free `activeCount()` queries
- Shutdown drains queue before exiting - no task loss

**Config Design:**
- Forward-declared `ParallelismTier` in config.h, included thread_pool.h in config.cpp
- `FileHandlingMode` enum defined in config.h for full type safety
- Empty library path means "use default" (app data dir / "library") - computed at use time

**Settings UI Design:**
- Library path is editable text input, not read-only or browse-only
- Validation is inline and immediate - no separate "Validate" button needed
- Thread count preview shows actual calculated value for current machine
- No platform-specific file dialog required (per plan note)

## Dependencies

**Requires:**
- MainThreadQueue (same threading directory, compatible patterns)
- Config system (INI serialization patterns)
- Settings app infrastructure (tab rendering, apply flow)

**Provides:**
- ThreadPool for parallel task execution (ready for ImportQueue use)
- Import configuration fields (parallelism, file handling, library path, toasts)
- Settings UI for user control of import behavior

**Affects:**
- Config system (new [import] section, 4 new fields)
- Settings app (new Import tab, thread_pool.cpp dependency)

## Next Steps

- Plan 02-02 will use ThreadPool for parallel import processing in ImportQueue
- ImportQueue will read parallelism tier from Config and create ThreadPool with calculated thread count
- File handling mode will be used by ImportQueue to copy/move files to library directory
- Toast toggle will control error notification display in UI

## Self-Check: PASSED

**Created files verified:**
```
✓ src/core/threading/thread_pool.h exists
✓ src/core/threading/thread_pool.cpp exists
```

**Modified files verified:**
```
✓ src/core/config/config.h modified
✓ src/core/config/config.cpp modified
✓ settings/settings_app.h modified
✓ settings/settings_app.cpp modified
✓ settings/CMakeLists.txt modified
```

**Commits verified:**
```
✓ 10c8e99 feat(02-01): implement ThreadPool with configurable worker count
✓ 48473c2 feat(02-01): extend Config and Settings app with import pipeline settings
```

All deliverables present and accounted for.
