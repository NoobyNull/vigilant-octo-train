---
phase: 02-import-pipeline
plan: 03
subsystem: import
tags: [gcode, loader, file-handler, mesh-conversion, toolpath]

# Dependency graph
requires:
  - phase: 02-01
    provides: "Database schema and import service interfaces"
  - phase: 02-02
    provides: "G-code parser and analyzer"
provides:
  - "GCodeLoader for .gcode/.nc/.ngc/.tap file import"
  - "FileHandler for copy/move/leave-in-place file operations"
  - "Toolpath-to-mesh conversion for viewport rendering"
  - "G-code metadata extraction (bounds, distance, time, feed rates, tools)"
affects: [02-04, 02-05, 02-06, import-ui]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "MeshLoader extension pattern for non-mesh file formats"
    - "Toolpath visualization via extruded quad geometry"
    - "Move type encoding in vertex texCoord for shader differentiation"
    - "Cross-filesystem file move with atomic rename fallback"

key-files:
  created:
    - src/core/loaders/gcode_loader.h
    - src/core/loaders/gcode_loader.cpp
    - src/core/import/file_handler.h
    - src/core/import/file_handler.cpp
  modified:
    - src/core/loaders/loader_factory.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Use extruded quads (0.5mm cutting, 0.2mm rapid) for toolpath visualization"
  - "Encode move type in vertex texCoord.x (1.0=rapid, 0.0=cutting) for shader use"
  - "Cross-filesystem move: atomic rename first, copy+verify+delete fallback"
  - "Default library directory: data dir / library"
  - "Unique filename generation capped at 10000 attempts"

patterns-established:
  - "Non-mesh formats can implement MeshLoader by converting to mesh geometry"
  - "FileHandler provides reusable file operations for import pipeline"
  - "Metadata extraction returns structured data separate from mesh"

# Metrics
duration: 5min 2s
completed: 2026-02-10
---

# Phase 02 Plan 03: G-code Loader and File Handler Summary

**G-code files (.gcode/.nc/.ngc/.tap) importable with toolpath-to-mesh conversion and three file handling modes (leave-in-place, copy, move) with cross-filesystem support**

## Performance

- **Duration:** 5 min 2 sec
- **Started:** 2026-02-10T03:36:12Z
- **Completed:** 2026-02-10T03:41:14Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- GCodeLoader registered in LoaderFactory for .gcode/.nc/.ngc/.tap extensions
- Toolpath segments converted to extruded quad geometry (degenerate triangles) for mesh rendering
- Metadata extraction: bounds, total distance, estimated time, unique feed rates, unique tool numbers
- FileHandler with three modes: leave-in-place, copy to library, move to library
- Cross-filesystem move support with atomic rename fallback to copy+verify+delete

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement GCodeLoader with toolpath-to-mesh conversion and metadata** - `79f06a5` (feat)
2. **Task 2: Implement FileHandler for copy/move/leave-in-place modes** - `491a621` (feat)

**Test dependencies update:** `7536d8a` (chore)

## Files Created/Modified
- `src/core/loaders/gcode_loader.h` - GCodeLoader class extending MeshLoader with metadata extraction
- `src/core/loaders/gcode_loader.cpp` - Toolpath-to-mesh conversion and metadata collection
- `src/core/loaders/loader_factory.cpp` - Registered .gcode/.nc/.ngc/.tap extensions
- `src/core/import/file_handler.h` - FileHandler class with three file handling modes
- `src/core/import/file_handler.cpp` - Copy/move/leave-in-place implementation with cross-filesystem support
- `src/CMakeLists.txt` - Added gcode_loader.cpp and file_handler.cpp to build
- `tests/CMakeLists.txt` - Added gcode_loader.cpp to test dependencies

## Decisions Made

**Toolpath visualization approach:**
- Convert G-code path segments to extruded quads (2 triangles per segment)
- Width: 0.5mm for cutting moves (G1), 0.2mm for rapid moves (G0)
- Rationale: Existing renderer expects triangle meshes; thin extrusions provide visual distinction

**Move type encoding:**
- Store rapid/cutting flag in vertex texCoord.x (1.0 = rapid, 0.0 = cutting)
- Rationale: Enables shader-based visual differentiation (e.g., different colors for rapid vs cutting)

**Cross-filesystem move strategy:**
- Try atomic rename first (works within same filesystem)
- Fallback to copy + size verification + delete source
- Rationale: Per research Pitfall 4, cross-filesystem moves require copy+delete, but atomic rename is faster when possible

**Unique filename generation:**
- Pattern: filename_1.ext, filename_2.ext, etc.
- Cap at 10000 attempts to prevent infinite loops
- Rationale: Prevents file overwrites while avoiding pathological cases

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- G-code files can now be imported through LoaderFactory like mesh files
- FileHandler ready for use in import pipeline (Plan 02-05)
- Metadata available for display in properties panel and library
- Toolpath geometry renders in viewport with existing mesh renderer
- Ready for background import queue integration (Plan 02-05)

## Self-Check: PASSED

All created files verified:
- src/core/loaders/gcode_loader.h ✓
- src/core/loaders/gcode_loader.cpp ✓
- src/core/import/file_handler.h ✓
- src/core/import/file_handler.cpp ✓

All commits verified:
- 79f06a5 (Task 1) ✓
- 491a621 (Task 2) ✓
- 7536d8a (Test dependencies) ✓

---
*Phase: 02-import-pipeline*
*Completed: 2026-02-10*
