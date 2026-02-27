---
phase: 11-niceties
plan: 01
subsystem: ui
tags: [imgui, cnc, dro, jog, g-code]

requires:
  - phase: 10-core-sender
    provides: CncStatusPanel and CncJogPanel with basic DRO and jog controls
provides:
  - Interactive DRO with double-click-to-zero via G10 L20 P0
  - Move-To modal dialog for explicit coordinate positioning
  - Four diagonal XY jog buttons with combined $J commands
affects: []

tech-stack:
  added: []
  patterns: [ImGui Selectable for click-interactive DRO values]

key-files:
  created: []
  modified:
    - src/ui/panels/cnc_status_panel.h
    - src/ui/panels/cnc_status_panel.cpp
    - src/ui/panels/cnc_jog_panel.h
    - src/ui/panels/cnc_jog_panel.cpp

key-decisions:
  - "Used ImGui::Selectable with AllowDoubleClick for DRO click-to-zero interaction"
  - "Diagonal jog buttons use slash/backslash characters as visual direction indicators"

patterns-established:
  - "DRO interaction: Selectable overlaid on axis value for click detection"

requirements-completed: [NIC-01, NIC-02, NIC-03]

duration: 8min
completed: 2026-02-27
---

# Plan 11-01: DRO Click-to-Zero, Move-To Dialog, Diagonal Jog Summary

**Interactive DRO with double-click-to-zero, Move-To dialog for explicit positioning, and diagonal XY jog buttons**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- DRO axis values are now interactive: double-click zeros the axis via G10 L20 P0
- Move-To dialog pre-fills current work position, supports G0 rapid and G1 feed moves
- Four diagonal jog buttons in XY grid corners send combined $J commands with per-step feedrates

## Task Commits

1. **Task 1: DRO click-to-zero and Move-To dialog** - `880cd3b` (feat)
2. **Task 2: Diagonal XY jog buttons** - `880cd3b` (feat, same commit)

## Files Created/Modified
- `src/ui/panels/cnc_status_panel.h` - Added Move-To dialog state members
- `src/ui/panels/cnc_status_panel.cpp` - Interactive DRO selectables, Move-To popup modal
- `src/ui/panels/cnc_jog_panel.h` - Added jogDiagonal method
- `src/ui/panels/cnc_jog_panel.cpp` - Diagonal buttons in 3x3 grid, jogDiagonal implementation

## Decisions Made
- Used ImGui::Selectable with AllowDoubleClick for DRO values (preserves existing large font layout)
- Diagonal buttons use backslash/slash characters for visual clarity with tooltips for direction

## Deviations from Plan
None - plan executed as written.

## Issues Encountered
None.

## Next Phase Readiness
- All CNC UI interaction enhancements complete
- Ready for verification

---
*Phase: 11-niceties*
*Completed: 2026-02-27*
