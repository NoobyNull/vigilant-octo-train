---
phase: 05-project-export
plan: 02
subsystem: export-ui
tags: [project-export, ui-wiring, file-menu, progress-dialog, drag-drop]

requires:
  - ProjectExportManager (05-01)
  - ProgressDialog for thread-safe progress
  - MainThreadQueue for worker-to-UI communication
  - FileDialog for save/open dialogs
provides:
  - Export .dwproj button in ProjectPanel
  - Import .dwproj via File menu
  - Drag-and-drop .dwproj import
  - Background threaded export/import with ProgressDialog
affects:
  - src/app/application.h
  - src/app/application.cpp
  - src/managers/file_io_manager.h
  - src/managers/file_io_manager.cpp
  - src/managers/ui_manager.h
  - src/managers/ui_manager.cpp
  - src/ui/panels/project_panel.h
  - src/ui/panels/project_panel.cpp

tech-stack:
  added: []
  patterns: [detached-thread-with-mainthread-queue, callback-based-ui-wiring]

key-files:
  created: []
  modified:
    - src/app/application.h
    - src/app/application.cpp
    - src/managers/file_io_manager.h
    - src/managers/file_io_manager.cpp
    - src/managers/ui_manager.h
    - src/managers/ui_manager.cpp
    - src/ui/panels/project_panel.h
    - src/ui/panels/project_panel.cpp

decisions:
  - "Used detached threads for export/import (matching existing thumbnail regeneration pattern)"
  - "Export button placed on separate line below Save/Close for visual clarity"
  - "Used FileDialog::projectFilters() for consistent *.dwproj filtering"

metrics:
  duration: "4m 0s"
  completed: "2026-02-22"
  tasks: 2
  tests: 0
---

# Phase 05 Plan 02: Export/Import UI Wiring Summary

**One-liner:** Export .dwproj button in ProjectPanel, File menu import entry, and drag-and-drop import with threaded ProgressDialog feedback.

## What Was Built

### Application Ownership (src/app/application.h/.cpp)

- Application constructs and owns `ProjectExportManager` after Database init
- Passes `ProjectExportManager*` to FileIOManager constructor
- Sets ProgressDialog and MainThreadQueue on FileIOManager for async operations
- Wires ExportProjectCallback from ProjectPanel to FileIOManager
- Wires Import .dwproj callback from UIManager to FileIOManager
- Proper shutdown ordering (ProjectExportManager destroyed before Database)

### FileIOManager Export/Import (src/managers/file_io_manager.h/.cpp)

**exportProjectArchive():**
- Validates current project exists and has models
- Shows save dialog with .dwproj filter
- Spawns detached thread running ProjectExportManager::exportProject()
- ProgressDialog shows per-model progress
- Toast on success/failure posted via MainThreadQueue

**importProjectArchive():**
- Shows open dialog with .dwproj filter
- Spawns detached thread running ProjectExportManager::importProject()
- ProgressDialog shows import progress
- Toast on success/failure, hides start page on success

**onFilesDropped() update:**
- Detects .dwproj extension on dropped files
- Routes directly to import flow (bypasses file dialog)

### ProjectPanel Export Button (src/ui/panels/project_panel.h/.cpp)

- "Export .dwproj" button below Save/Close buttons
- Disabled (grayed out) when project has no models
- Tooltip "Add models to project before exporting" when disabled
- ExportProjectCallback wired through Application

### File Menu Import Entry (src/managers/ui_manager.h/.cpp)

- "Import .dwproj..." menu item after Export Model
- Triggers FileIOManager::importProjectArchive() via callback

## Deviations from Plan

None - plan executed exactly as written.

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 | 80ccdf7 | feat(05-02): wire ProjectExportManager into Application and FileIOManager |
| 2 | 20b74ea | feat(05-02): add Export button to ProjectPanel and Import .dwproj to File menu |
