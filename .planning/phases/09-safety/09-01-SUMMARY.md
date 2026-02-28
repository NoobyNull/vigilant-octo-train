---
phase: 09-safety
plan: 01
subsystem: cnc-safety
tags: [imgui, config, safety, long-press, dead-man, grbl]

requires:
  - phase: 08-macros
    provides: CNC panel infrastructure and CncController integration
provides:
  - 8 safety Config keys with INI persistence under [safety] section
  - LongPressButton UI pattern for critical action confirmation
  - Dead-man watchdog for continuous jog safety
affects: [09-02, cnc-settings-panel, safety-settings-ui]

tech-stack:
  added: []
  patterns: [LongPressButton anonymous-namespace struct per panel, dead-man watchdog timer]

key-files:
  created: []
  modified:
    - src/core/config/config.h
    - src/core/config/config.cpp
    - src/ui/panels/cnc_jog_panel.h
    - src/ui/panels/cnc_jog_panel.cpp
    - src/ui/panels/cnc_safety_panel.cpp
    - src/ui/panels/gcode_panel.cpp

key-decisions:
  - "LongPressButton defined as anonymous-namespace struct in each .cpp that uses it (no shared header, avoids coupling)"
  - "Dead-man watchdog uses frame-based timer with keepalive reset pattern rather than OS timer"
  - "Abort long-press is off by default (existing confirmation dialog preferred for safety)"

patterns-established:
  - "LongPressButton pattern: anonymous-namespace struct with render() returning true on hold completion, fill overlay progress visualization"
  - "Safety config getters follow getSafety* naming convention with bool/int pairs for enable+threshold"

requirements-completed: [SAF-10, SAF-11, SAF-12, SAF-13, SAF-17]

duration: 8 min
completed: 2026-02-26
---

# Phase 9 Plan 01: Config Keys, Long-Press Buttons, Dead-Man Watchdog Summary

**8 safety Config keys with INI persistence, LongPressButton UI pattern for Home/Start/Abort critical buttons, dead-man watchdog for continuous jog auto-stop**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-26T23:30:00Z
- **Completed:** 2026-02-26T23:38:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- All 8 safety config keys (long-press, dead-man, door interlock, soft limits, pause-before-reset) with INI section persistence
- Home, Start, and Abort buttons use visible hold-to-activate pattern with progress fill overlay
- Dead-man watchdog catches stale key state during continuous jog, forcing auto-stop on timeout

## Task Commits

1. **Task 1: Safety Config keys** - `1f4341c` (feat)
2. **Task 2: Long-press buttons + dead-man watchdog** - `1accfb6` (feat)

## Files Created/Modified
- `src/core/config/config.h` - Added 8 safety getter/setter pairs and private members
- `src/core/config/config.cpp` - [safety] INI section for load/save
- `src/ui/panels/cnc_jog_panel.h` - Added m_jogWatchdogTimer member
- `src/ui/panels/cnc_jog_panel.cpp` - LongPressButton on Home, dead-man watchdog in handleKeyboardJog
- `src/ui/panels/cnc_safety_panel.cpp` - LongPressButton option for Abort button
- `src/ui/panels/gcode_panel.cpp` - LongPressButton on Start button

## Decisions Made
- LongPressButton as anonymous-namespace struct avoids header coupling while allowing per-panel customization
- Dead-man watchdog increments every frame and resets on positive key-down confirmation
- Abort long-press is disabled by default since the confirmation dialog is a more familiar UX pattern

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Config keys ready for Plan 09-02 to wire door interlock, soft limits, and safety settings UI
- LongPressButton pattern established for reuse

---
*Phase: 09-safety*
*Completed: 2026-02-26*
