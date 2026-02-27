---
phase: 10-core-sender
plan: 01
subsystem: ui
tags: [imgui, grbl, cnc, overrides, coolant, alarm, config]

requires:
  - phase: 08-cnc-controller-suite
    provides: CncController with override/command methods, CncStatusPanel, CncCallbacks wiring
provides:
  - Interactive spindle/feed override sliders in status panel
  - Rapid override buttons (25/50/100%)
  - Coolant toggle buttons (M7/M8/M9)
  - Alarm banner with code, description, and inline $X unlock
  - Configurable status poll interval (50-200ms) via Config INI
affects: [10-core-sender, settings-app]

tech-stack:
  added: []
  patterns: [per-section Config INI keys, CncController dynamic poll rate]

key-files:
  created: []
  modified:
    - src/core/config/config.h
    - src/core/config/config.cpp
    - src/core/cnc/cnc_controller.h
    - src/core/cnc/cnc_controller.cpp
    - src/ui/panels/cnc_status_panel.h
    - src/ui/panels/cnc_status_panel.cpp
    - src/app/application_wiring.cpp

key-decisions:
  - "Override section is always visible (not conditionally hidden at 100%) since it is now interactive"
  - "Coolant state tracked by command buttons (GRBL standard status does not report coolant)"
  - "Status poll interval stored as int member, no mutex needed (single writer, single reader, int is atomic)"

patterns-established:
  - "Config [cnc] section: INI section for CNC-specific settings"
  - "setCncController() wiring pattern for status panel to send commands"

requirements-completed: [SND-01, SND-02, SND-03, SND-04, SND-05]

duration: 8min
completed: 2026-02-26
---

# Plan 10-01: Override Controls, Coolant, Alarm, Poll Config Summary

**Interactive spindle/feed/rapid override controls, coolant M7/M8/M9 toggles, alarm banner with $X unlock, and configurable status polling interval**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Spindle override slider (10-200%) sends GRBL real-time spindle override commands
- Feed override slider (10-200%) with reset button
- Rapid override buttons (25/50/100%) with active state highlighting
- Coolant toggle buttons (Flood M8, Mist M7, Off M9) disabled during alarm
- Alarm banner shows alarm code, human-readable description, and inline $X unlock button
- Status polling interval configurable via Config (50-200ms, default 200ms)
- CncController uses dynamic m_statusPollMs instead of hardcoded constant

## Task Commits

1. **Task 1: Config key + CncController dynamic poll** - `d8f3d75` (feat)
2. **Task 2: Override controls, coolant, alarm display** - `417c93a` (feat)

## Files Created/Modified
- `src/core/config/config.h` - Added getStatusPollIntervalMs/setStatusPollIntervalMs, m_statusPollIntervalMs member
- `src/core/config/config.cpp` - Added [cnc] INI section for status_poll_interval_ms load/save
- `src/core/cnc/cnc_controller.h` - Replaced static STATUS_POLL_MS with m_statusPollMs member, added setStatusPollMs()
- `src/core/cnc/cnc_controller.cpp` - Uses m_statusPollMs in IO thread, reads Config on connect
- `src/ui/panels/cnc_status_panel.h` - Added CncController* dependency, override/coolant/alarm methods
- `src/ui/panels/cnc_status_panel.cpp` - Full override controls, coolant buttons, alarm banner implementation
- `src/app/application_wiring.cpp` - Wired setCncController and onAlarm for status panel

## Decisions Made
- Override section always visible (interactive controls, not just display)
- Coolant tracked by buttons sent, not GRBL status (standard GRBL doesn't report coolant state)
- Added application_wiring.cpp to files modified (not in original plan) to properly wire setCncController and onAlarm

## Deviations from Plan
- Added application_wiring.cpp modification as the plan noted this was needed but did not include it in files_modified

## Issues Encountered
None

## Next Phase Readiness
- Plan 10-02 can build on top: CncStatusPanel now has m_cnc pointer for WCS selector
- Override section provides template for additional interactive controls

---
*Phase: 10-core-sender*
*Completed: 2026-02-26*
