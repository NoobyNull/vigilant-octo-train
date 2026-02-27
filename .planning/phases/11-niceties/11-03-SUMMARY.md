---
phase: 11-niceties
plan: 03
subsystem: ui
tags: [imgui, cnc, config, input-binding, gcode, toast]

requires:
  - phase: 11-niceties
    provides: Plans 01 and 02 complete
provides:
  - Recent G-code files list (last 10) with Config persistence
  - Feed/spindle override keyboard shortcuts via InputBinding system
  - Job completion toast notification with elapsed time
  - Status bar flash effect on job completion
affects: []

tech-stack:
  added: []
  patterns: [Config recent-files pattern extended to G-code, BindAction keyboard shortcuts]

key-files:
  created: []
  modified:
    - src/core/config/config.h
    - src/core/config/config.cpp
    - src/core/config/input_binding.h
    - src/core/config/input_binding.cpp
    - src/ui/panels/gcode_panel.h
    - src/ui/panels/gcode_panel.cpp

key-decisions:
  - "Recent G-code files stored in separate [recent_gcode] INI section"
  - "Override shortcuts default to Ctrl+=/- and Ctrl+Shift+=/- for feed/spindle"
  - "Job completion notification and flash are independently toggleable via Config"

patterns-established:
  - "Recent files: addRecentGCodeFile follows same pattern as addRecentProject"
  - "Override shortcuts: checkOverride lambda for clean modifier+key matching"

requirements-completed: [NIC-05, NIC-06, NIC-08]

duration: 10min
completed: 2026-02-27
---

# Plan 11-03: Recent G-code Files, Override Shortcuts, Job Completion Notification Summary

**Recent G-code files dropdown, feed/spindle override keyboard shortcuts, and job completion toast with status bar flash**

## Performance

- **Duration:** 10 min
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Recent G-code files (last 10) stored in Config, with dropdown popup in toolbar for quick re-loading
- Four new BindAction entries for feed/spindle override +/-10% with default keyboard shortcuts
- Job completion toast shows "Job Complete" with elapsed time (mm:ss format)
- Pulsing green flash bar for 3 seconds on job completion
- Both notification features independently configurable via Config

## Task Commits

1. **Task 1: Recent G-code files and override shortcuts** - `bfb1967` (feat)
2. **Task 2: Job completion notification** - `bfb1967` (feat, same commit)

## Files Created/Modified
- `src/core/config/config.h` - Added recent G-code file methods, job notification config
- `src/core/config/config.cpp` - Load/save for recent_gcode section, addRecentGCodeFile impl
- `src/core/config/input_binding.h` - Four new BindAction entries for override shortcuts
- `src/core/config/input_binding.cpp` - Default bindings and display names for override actions
- `src/ui/panels/gcode_panel.h` - Added renderRecentFiles, job flash timer
- `src/ui/panels/gcode_panel.cpp` - Recent files popup, override shortcut handling, enhanced onGrblProgress

## Decisions Made
- Override shortcuts check modifiers explicitly for strict matching (no accidental triggers)
- Recent files stored in [recent_gcode] section separate from [recent] projects
- Flash bar uses sin-based alpha pulsing at ~1Hz frequency

## Deviations from Plan
None - plan executed as written.

## Issues Encountered
None.

## Next Phase Readiness
- All niceties features complete
- Ready for phase verification

---
*Phase: 11-niceties*
*Completed: 2026-02-27*
