---
phase: 05-job-streaming
plan: 02
subsystem: cnc-ui
tags: [cnc, feed-deviation, wiring, ui-manager]

requires: [cnc_job_panel.h, cnc_tool_panel.h, ui_manager.h, application_wiring.cpp]
provides: [feed-deviation-warning, cnc-job-panel-integration]
affects: [ui_manager.h, ui_manager.cpp, application_wiring.cpp]

tech-stack:
  patterns: [UIManager panel pattern, CncCallbacks wiring, override-aware comparison]

key-files:
  created: []
  modified:
    - src/ui/panels/cnc_job_panel.h
    - src/ui/panels/cnc_job_panel.cpp
    - src/managers/ui_manager.h
    - src/managers/ui_manager.cpp
    - src/app/application_wiring.cpp

key-decisions:
  - Feed deviation accounts for feed override percentage (50% override = 50% of recommended is normal)
  - Deviation only checked during MachineState::Run with feedRate > 0 (avoids false positives during decel)
  - Recommended feed rate pushed from CncToolPanel to CncJobPanel via status update callback (5Hz)
  - Streaming state detected from progress data (totalLines > 0 && ackedLines < totalLines)

requirements-completed: [TAC-04]
duration: 5 min
completed: 2026-02-24
---

# Phase 05 Plan 02: Feed Deviation Warning + UIManager Integration Summary

Feed deviation warning implemented with 20% threshold, override-aware comparison, and red visual highlight. CncJobPanel fully wired into UIManager (CNC workspace mode) and CncController callbacks (status, progress, connection, recommended feed rate).

## Tasks
- Task 1: Implemented feed deviation detection and warning display in CncJobPanel
- Task 2: Wired CncJobPanel into UIManager and application callbacks

## Deviations from Plan
None - plan executed exactly as written.

## Next
Phase complete, ready for verification.
