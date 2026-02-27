---
phase: 12-extended
plan: 04
subsystem: cnc
tags: [grbl, jog, M6, tool-change, M98, macros, keyboard]

requires:
  - phase: 08-macros
    provides: MacroManager and jog panel infrastructure
provides:
  - Tab key cycling through jog step groups
  - M6 tool change detection with streaming pause
  - M98 nested macro expansion with recursion guard
affects: [cnc-jog, cnc-controller, macro-execution]

tech-stack:
  added: []
  patterns: [M6 sender-side detection, M98 recursive expansion]

key-files:
  modified:
    - src/ui/panels/cnc_jog_panel.h
    - src/ui/panels/cnc_jog_panel.cpp
    - src/core/cnc/cnc_controller.h
    - src/core/cnc/cnc_controller.cpp
    - src/core/cnc/cnc_types.h
    - src/core/cnc/macro_manager.h
    - src/core/cnc/macro_manager.cpp

key-decisions:
  - "M6 detected before sending to GRBL (sender-level pause, not firmware)"
  - "M6 line skipped on acknowledgment since GRBL doesn't implement M6"
  - "M98 expansion uses getById() to look up macros by P parameter"
  - "Recursion guard at depth 16 with std::runtime_error on overflow"
  - "Tab key only active when no text input is focused (WantTextInput check)"

patterns-established:
  - "M6 detection pattern: uppercase, strip comments, check M6/M06 excluding M60+"
  - "M98 expansion pattern: recursive with depth guard, inline error comments for missing subs"

requirements-completed: [EXT-08, EXT-10, EXT-13]

duration: 10min
completed: 2026-02-27
---

# Plan 12-04: Extended Summary

**Jog step cycling via Tab key, M6 tool change detection, and M98 nested macro expansion**

## Performance

- **Duration:** ~10 min
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Tab key cycles jog step groups (Small 0.1mm -> Medium 1mm -> Large 10mm)
- M6 tool change halts streaming until operator acknowledges
- M98 Pxxxx resolves nested macros recursively with 16-level depth guard
- onToolChange callback added to CncCallbacks for UI notification

## Task Commits

1. **Task 1: Cycle-steps keyboard shortcut** - `2a5de28` (feat)
2. **Task 2: M6 detection and M98 expansion** - `2a5de28` (feat)

## Deviations from Plan
None - plan executed as specified.

---
*Phase: 12-extended*
*Completed: 2026-02-27*
