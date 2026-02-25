---
phase: 06-job-safety
plan: 04
subsystem: ui
tags: [imgui, resume, modal-scanner, preflight, streaming, safety]

requires:
  - phase: 06-01
    provides: GCodeModalScanner for preamble generation
  - phase: 06-03
    provides: CncSafetyPanel base with button controls
provides:
  - Resume-from-line dialog with preamble preview
  - Pre-flight gating on all streaming operations
  - Program data pipeline from GCodePanel to CncSafetyPanel
affects: []

tech-stack:
  added: []
  patterns: [preamble preview with child window, pre-flight gating pattern]

key-files:
  created: []
  modified:
    - src/ui/panels/cnc_safety_panel.h
    - src/ui/panels/cnc_safety_panel.cpp
    - src/ui/panels/gcode_panel.h
    - src/ui/panels/gcode_panel.cpp
    - src/app/application_wiring.cpp

key-decisions:
  - "Pre-flight passes false for tool/material selection (conservative default)"
  - "Program data pushed lazily on first progress update via getRawLines()"
  - "Resume combines preamble + remaining lines into single startStream call"

patterns-established:
  - "Pre-flight gating: check before startStream, errors block, warnings inform"
  - "Resume flow: input line -> generate preamble -> review -> pre-flight -> stream"

requirements-completed: [SAF-04, SAF-05, SAF-07]

duration: 8min
completed: 2026-02-24
---

# Plan 06-04: Resume Dialog Summary

**Resume-from-line dialog with modal state preamble preview and pre-flight validation gating all streaming**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Resume-from-line dialog: line input, preamble generation, read-only preview
- Pre-flight checks gate Resume button (errors block, warnings show)
- Combined streaming: preamble lines + remaining program from resume point
- Pre-flight checks added to GCodePanel's normal Send flow
- Safety warnings about arbitrary resume risks and arc limitations
- GCodePanel::getRawLines() exposes program for safety panel

## Task Commits

1. **Task 1: Resume dialog in CncSafetyPanel** - `b2c9fd7` (feat)
2. **Task 2: Pre-flight in GCodePanel + program wiring** - `b2c9fd7` (feat, same commit)

## Files Created/Modified
- `src/ui/panels/cnc_safety_panel.h` - Resume state members and renderResumeDialog
- `src/ui/panels/cnc_safety_panel.cpp` - Full resume dialog with preamble preview
- `src/ui/panels/gcode_panel.h` - getRawLines() accessor
- `src/ui/panels/gcode_panel.cpp` - Pre-flight checks in buildSendProgram + getRawLines
- `src/app/application_wiring.cpp` - Program data push to safety panel

## Decisions Made
- Pass false for tool/material selection in pre-flight (conservative -- always shows warnings)
- Program data pushed lazily in onProgressUpdate (avoids separate callback)
- Resume dialog resets state on Cancel (clean slate for next open)

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None

## Next Phase Readiness
- Phase 6 Job Safety complete -- all SAF requirements addressed
- End-to-end flow: load G-code -> pre-flight -> stream OR resume from line with preamble

---
*Phase: 06-job-safety*
*Completed: 2026-02-24*
