---
phase: 09-safety
plan: 02
subsystem: cnc-safety
tags: [imgui, safety, door-interlock, soft-limits, preflight, grbl]

requires:
  - phase: 09-safety
    provides: Safety config keys and LongPressButton pattern from Plan 01
provides:
  - Door interlock blocking when GRBL reports door pin active
  - Soft limit pre-check comparing G-code bounds vs machine travel
  - Pause-before-reset stop behavior reducing tool marks
  - Safety settings UI tab with per-feature toggles
affects: [cnc-safety-panel, gcode-panel, cnc-settings-panel]

tech-stack:
  added: []
  patterns: [pause-before-reset timer, soft limit bounds comparison, safety settings tab]

key-files:
  created: []
  modified:
    - src/ui/panels/cnc_safety_panel.h
    - src/ui/panels/cnc_safety_panel.cpp
    - src/ui/panels/gcode_panel.cpp
    - src/core/cnc/preflight_check.h
    - src/core/cnc/preflight_check.cpp
    - src/ui/panels/cnc_settings_panel.h
    - src/ui/panels/cnc_settings_panel.cpp

key-decisions:
  - "Soft limit check is Warning severity, not Error -- operator can choose to proceed"
  - "Pause-before-reset uses 200ms timer to allow deceleration before soft reset"
  - "Door interlock checked via direct status pin reading in gcode_panel to avoid circular panel dependencies"

patterns-established:
  - "runPreflightChecks accepts optional nullable params for backwards-compatible extension"
  - "Safety settings tab renders all toggles with immediate Config save on change"

requirements-completed: [SAF-14, SAF-15, SAF-16, SAF-17]

duration: 7 min
completed: 2026-02-26
---

# Phase 9 Plan 02: Door Interlock, Soft Limits, Pause-Before-Reset, Safety UI Summary

**Door interlock blocking, soft limit pre-check for G-code bounds vs machine travel, pause-before-reset graceful stop, and safety settings tab with all 8 feature toggles**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-26T23:40:00Z
- **Completed:** 2026-02-26T23:47:00Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Door interlock shows warning banner and blocks Start/Resume when GRBL reports Pn:D active
- Soft limit pre-check warns operator when job bounding box exceeds machine travel limits
- Abort and Stop send feed hold before soft reset for smoother deceleration
- Safety tab in firmware settings panel exposes all 8 feature toggles with immediate persistence

## Task Commits

1. **Task 1: Door interlock and pause-before-reset** - `6a4a95d` (feat)
2. **Task 2: Soft limit pre-check and safety settings tab** - `f51cdf3` (feat)

## Files Created/Modified
- `src/ui/panels/cnc_safety_panel.h` - Added isDoorInterlockActive(), m_doorActive, abort timer members
- `src/ui/panels/cnc_safety_panel.cpp` - Door warning banner, pause-before-reset, resume disabled during interlock
- `src/ui/panels/gcode_panel.cpp` - Door interlock on Start, pause-before-reset on Stop, soft limit in buildSendProgram
- `src/core/cnc/preflight_check.h` - Extended signature with optional bounds/profile params
- `src/core/cnc/preflight_check.cpp` - Soft limit bounds comparison logic
- `src/ui/panels/cnc_settings_panel.h` - Added renderSafetyTab() declaration
- `src/ui/panels/cnc_settings_panel.cpp` - Safety tab with checkboxes and sliders for all 8 safety features

## Decisions Made
- Soft limit check uses Warning severity so the operator can choose to proceed with oversized jobs
- Door interlock check in gcode_panel reads MachineStatus pins directly rather than depending on CncSafetyPanel pointer
- Pause-before-reset delay of 200ms gives GRBL time to decelerate before the soft reset byte

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 9 Safety complete -- all safety guards implemented and configurable
- Ready for Phase 10 Core Sender

---
*Phase: 09-safety*
*Completed: 2026-02-26*
