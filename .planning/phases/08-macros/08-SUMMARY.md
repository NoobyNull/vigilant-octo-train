---
phase: "08"
plan: "01-03"
subsystem: "CNC Macros"
tags: [cnc, macros, sqlite, ui-panel, grbl]
dependency-graph:
  requires: [phase-01-foundation, phase-03-manual-control]
  provides: [macro-storage, macro-execution, built-in-macros]
  affects: [ui-manager, application, cnc-controller]
tech-stack:
  added: [MacroManager, CncMacroPanel]
  patterns: [sqlite-crud, panel-base-class, cnc-callbacks]
key-files:
  created:
    - src/core/cnc/macro_manager.h
    - src/core/cnc/macro_manager.cpp
    - src/ui/panels/cnc_macro_panel.h
    - src/ui/panels/cnc_macro_panel.cpp
    - tests/test_macro_manager.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt
    - src/managers/ui_manager.h
    - src/managers/ui_manager.cpp
    - src/app/application.h
    - src/app/application.cpp
    - src/app/application_wiring.cpp
    - src/core/paths/app_paths.h
    - src/core/paths/app_paths.cpp
decisions:
  - Separate SQLite database for macros (macros.db) rather than sharing library.db
  - Built-in macros checked by count of built_in=1 rows for idempotent seeding
  - G-code comments (semicolon and parenthesis styles) stripped during parseLines
  - Macro panel uses inline edit area rather than popup dialog for simplicity
metrics:
  duration: "6 minutes"
  completed: "2026-02-25"
  tasks: 3
  files-created: 5
  files-modified: 9
  tests-added: 14
  tests-total: 715
---

# Phase 8: Macros Summary

SQLite-backed macro CRUD with 3 built-in macros (Homing, Probe Z, Return to Zero) and CncMacroPanel UI for run/edit/delete/reorder.

## Tasks Completed

### Plan 1: MacroManager Core + SQLite Storage + Tests
- **Commit:** c0a8be6
- Created `Macro` struct with id, name, gcode, shortcut, sortOrder, builtIn fields
- `MacroManager` class with full CRUD: getAll, getById, addMacro, updateMacro, deleteMacro, reorder
- `parseLines()` splits multi-line G-code, trims whitespace, skips empty/comment lines
- `ensureBuiltIns()` seeds 3 built-in macros idempotently:
  - Homing Cycle: `$H`
  - Probe Z (Touch Plate): `G91 / G38.2 Z-50 F100 / G90`
  - Return to Zero: `G90 / G53 G0 Z0 / G53 G0 X0 Y0` (Z first for safety)
- Built-in macros are non-deletable (throws on delete attempt) but fully editable
- 14 unit tests covering schema creation, CRUD, sort ordering, built-in guards, parseLines, idempotency

### Plan 2: CncMacroPanel UI
- **Commit:** 4a8d215
- `CncMacroPanel` extends `Panel` base class
- Macro list with Play button, name, optional shortcut hint, Edit/Delete/reorder controls
- State guards: run disabled during streaming, alarm, homing, or disconnect
- Built-in macros show settings icon and cannot be deleted
- Inline edit area with name, multiline G-code editor, shortcut field, save/cancel
- Execution sends parsed G-code lines sequentially via `CncController::sendCommand()`

### Plan 3: UIManager Wiring
- **Commit:** 61e812d
- Added `getMacroDatabasePath()` to app_paths (returns `<data_dir>/macros.db`)
- Application owns `MacroManager`, creates it during init, calls `ensureBuiltIns()`
- UIManager: CncMacroPanel added to panel creation, rendering, shutdown, View menu, workspace mode
- application_wiring: macro panel receives CncController and MacroManager pointers
- onConnectionChanged and onStatusUpdate callbacks wired for state guard tracking

## Deviations from Plan

None -- plan executed exactly as written.

## Verification

- All 715 tests pass (14 new MacroManager tests + 701 existing)
- Main executable compiles cleanly
- Test executable compiles cleanly
