---
phase: 04-tool-integration
plan: 01
subsystem: ui
tags: [imgui, cnc, feeds-speeds, tool-database, material-manager]

requires:
  - phase: 01-fix-foundation
    provides: Thread-safe CncController for CNC workspace context
provides:
  - CncToolPanel class with tool/material selection and auto-calculation display
  - ImGui panel following established CNC panel patterns
affects: [04-02-integration, 05-job-streaming]

tech-stack:
  added: []
  patterns: [auto-recalculate on selection change, change tracking via prev IDs]

key-files:
  created:
    - src/ui/panels/cnc_tool_panel.h
    - src/ui/panels/cnc_tool_panel.cpp
  modified:
    - src/CMakeLists.txt

key-decisions:
  - "Standalone CncToolPanel rather than integrating into CncStatusPanel for single responsibility"
  - "Flat combo selector for tools rather than recursive tree (simpler for CNC context)"
  - "Auto-recalculate on selection change with prev-ID tracking to avoid per-frame recalc"
  - "Uses MaterialManager (Janka hardness) for calculator, not Vectric VtdbMaterial"

patterns-established:
  - "Selection change tracking: store prev IDs, compare each frame, recalculate only on change"
  - "Material combo from MaterialManager for Janka-based calculation"

requirements-completed: [TAC-01, TAC-02, TAC-03]

duration: 5min
completed: 2026-02-24
---

# Phase 4 Plan 01: CncToolPanel Summary

**CncToolPanel with tool geometry combo, material dropdown, machine info, and auto-calculated feeds/speeds display**

## Performance

- **Duration:** 5 min
- **Started:** 2026-02-24
- **Completed:** 2026-02-24
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- CncToolPanel class with tool selector combo populated from ToolDatabase tree entries
- Material selector combo populated from MaterialManager with Janka hardness display
- Auto-recalculation when tool or material selection changes
- Clean reference display with RPM, feed rate, plunge rate, stepdown, stepover, chip load, power

## Task Commits

1. **Task 1: Create CncToolPanel header and implementation** - `b5d9bce` (feat)
2. **Task 2: Add CncToolPanel to CMake build** - `b5d9bce` (feat, combined commit)

## Files Created/Modified
- `src/ui/panels/cnc_tool_panel.h` - CncToolPanel class declaration with setters and state
- `src/ui/panels/cnc_tool_panel.cpp` - Full implementation: tool/material combos, machine info, auto-calc, results
- `src/CMakeLists.txt` - Added cnc_tool_panel.cpp to build

## Decisions Made
- Standalone panel rather than embedding in CncStatusPanel (single responsibility, dockable)
- Flat combo for tool selection rather than recursive tree (CNC context is selection-focused, not CRUD)
- Auto-recalculate on selection change using prev-ID tracking pattern
- Green-colored parameter values for readability against dark theme
- Machine selector only shown if multiple machines exist, defaults to first available

## Deviations from Plan
None - plan executed as specified.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- CncToolPanel class ready for UIManager integration (Plan 04-02)
- Panel needs wiring to ToolDatabase and MaterialManager via application_wiring.cpp

---
*Phase: 04-tool-integration*
*Completed: 2026-02-24*
