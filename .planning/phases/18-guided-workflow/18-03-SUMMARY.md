---
phase: 18-guided-workflow
plan: "03"
subsystem: ui, carve
tags: [gcode, export, imgui, wizard, cnc, toolpath]

# Dependency graph
requires:
  - phase: 17-toolpath-generation
    provides: MultiPassToolpath, ToolpathConfig, Toolpath types
  - phase: 18-guided-workflow (plan 01)
    provides: DirectCarvePanel wizard framework, step navigation
provides:
  - G-code file export from MultiPassToolpath (gcode_export.h/cpp)
  - renderZeroConfirm with G10 L20 zero-setting buttons
  - renderCommit with full summary table and "Save as G-code"
  - renderRunning with progress bar, pause/resume, abort
affects: [19-streaming-integration]

# Tech tracking
tech-stack:
  added: []
  patterns: [standalone free-function export (testable without UI), ImGui table summary card]

key-files:
  created:
    - src/core/carve/gcode_export.h
    - src/core/carve/gcode_export.cpp
    - tests/test_gcode_export.cpp
  modified:
    - src/ui/panels/direct_carve_panel.h
    - src/ui/panels/direct_carve_panel.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "generateGcode() returns string for testability, exportGcode() wraps with file I/O"
  - "Feed rate F word emitted only on first G1 move (modal G-code convention)"
  - "FileDialog showSave with callback pattern matches existing panel convention"
  - "Abort sends Ctrl+X soft reset (0x18) for immediate machine stop"

patterns-established:
  - "Free function G-code export: stateless, testable, no UI dependency"
  - "RunState enum for completed/aborted state transitions with New Carve reset"

requirements-completed: [DC-25]

# Metrics
duration: 9min
completed: 2026-02-28
---

# Phase 18 Plan 03: Confirmation Summary, Commit Flow, Run Integration Summary

**Standalone G-code export with 10 tests, plus full renderZeroConfirm/renderCommit/renderRunning wizard steps with progress bar, pause/resume, abort, and "Save as G-code" file dialog**

## Performance

- **Duration:** 9 min
- **Started:** 2026-02-28T20:22:35Z
- **Completed:** 2026-02-28T20:31:19Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments
- Standalone gcode_export.h/cpp with exportGcode() and generateGcode() free functions
- 10 unit tests covering header, preamble, rapids vs feed, footer, file write, coordinate formatting
- renderZeroConfirm with G10 L20 P0 zero-setting buttons (Set Zero Here, Zero XY, Zero Z)
- renderCommit with ImGui table summary card showing all parameters + "Save as G-code" button
- renderRunning with progress bar, elapsed/remaining time, pause/resume toggle, abort confirmation dialog

## Task Commits

Each task was committed atomically:

1. **Task 1: G-code export implementation** - `f5f34df` (feat)
2. **Task 2: G-code export tests + CMake** - `55dade6` (test)
3. **Task 3: Panel wizard steps** - `96a5a39` (feat)

## Files Created/Modified
- `src/core/carve/gcode_export.h` - Export function declarations (exportGcode + generateGcode)
- `src/core/carve/gcode_export.cpp` - G-code generation with header, preamble, toolpath, footer
- `tests/test_gcode_export.cpp` - 10 unit tests for G-code export
- `src/ui/panels/direct_carve_panel.h` - Added RunState enum, running state members, FileDialog pointer
- `src/ui/panels/direct_carve_panel.cpp` - Full renderZeroConfirm, renderCommit, renderRunning implementations
- `src/CMakeLists.txt` - Added gcode_export.cpp to main build
- `tests/CMakeLists.txt` - Added test and dependency for gcode_export

## Decisions Made
- generateGcode() returns string for direct testability without filesystem; exportGcode() wraps with std::ofstream
- Feed rate (F word) emitted only on first G1 move, following modal G-code convention
- FileDialog::showSave with callback pattern used for "Save as G-code" (matches existing panel convention)
- Abort sends Ctrl+X (0x18) soft reset for immediate machine stop
- RunState enum with Active/Paused/Completed/Aborted states for clean renderRunning transitions

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Restored color constants removed by parallel linter**
- **Found during:** Task 3 (panel wizard steps)
- **Issue:** Parallel plan's linter removed kGreen, kRed, kYellow, kDimmed, kBright constants from anonymous namespace
- **Fix:** Restored constexpr ImVec4 color constants
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Committed in:** 96a5a39

**2. [Rule 1 - Bug] Fixed unused variable warning in renderStepIndicator**
- **Found during:** Task 3 build verification
- **Issue:** nextCenterX variable declared but unused (from parallel plan code)
- **Fix:** Removed unused variable
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Committed in:** 96a5a39

**3. [Rule 3 - Blocking] Added missing cnc_tool.h include to carve_job.h**
- **Found during:** Task 2 build
- **Issue:** carve_job.h referenced VtdbToolGeometry without including cnc_tool.h
- **Fix:** Added #include directive (already committed by parallel plan before our commit)
- **Files modified:** src/core/carve/carve_job.h
- **Committed in:** (absorbed by parallel plan commit)

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking)
**Impact on plan:** All fixes required for compilation. No scope creep.

## Issues Encountered
- Parallel plans 18-01 and 18-02 were actively modifying DirectCarvePanel files during execution, causing file-modified-since-read errors. Resolved by re-reading files before each edit.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- G-code export is fully standalone and tested, ready for streaming integration in Phase 19
- DirectCarvePanel has all 9 wizard steps implemented
- RunState machine ready for CncController streaming callbacks

## Self-Check: PASSED

All files verified present, all commit hashes found in git log.

---
*Phase: 18-guided-workflow*
*Completed: 2026-02-28*
