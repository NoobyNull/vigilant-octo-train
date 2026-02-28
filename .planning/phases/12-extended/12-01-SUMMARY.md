---
phase: 12-extended
plan: 01
subsystem: cnc
tags: [grbl, alarm, error, firmware, logging]

requires:
  - phase: 08-macros
    provides: CNC settings panel and console panel infrastructure
provides:
  - Comprehensive GRBL alarm code reference (10 codes with detailed descriptions)
  - Complete GRBL error code reference (all 37 codes)
  - Firmware $I query and display in settings panel
  - Log-to-file configuration toggle with INI persistence
affects: [cnc-status, cnc-settings, cnc-console]

tech-stack:
  added: []
  patterns: [AlarmEntry/ErrorEntry reference structs with static tables]

key-files:
  modified:
    - src/core/cnc/cnc_types.h
    - src/ui/panels/cnc_status_panel.cpp
    - src/ui/panels/cnc_console_panel.cpp
    - src/ui/panels/cnc_settings_panel.h
    - src/ui/panels/cnc_settings_panel.cpp
    - src/core/config/config.h
    - src/core/config/config.cpp
    - tests/test_cnc_controller.cpp

key-decisions:
  - "Alarm descriptions expanded to full sentences with operator guidance (e.g., 're-home required')"
  - "Error descriptions cover all 37 GRBL error codes for complete reference"
  - "Firmware info captured by intercepting $I response lines until 'ok'"

patterns-established:
  - "AlarmEntry/ErrorEntry structs with static array + count pattern for reference tables"

requirements-completed: [EXT-01, EXT-02, EXT-03, EXT-05]

duration: 15min
completed: 2026-02-27
---

# Plan 12-01: Extended Summary

**GRBL alarm/error reference tables with detailed descriptions, firmware info display, and log-to-file configuration**

## Performance

- **Duration:** ~15 min
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments
- Expanded alarm descriptions to 10 codes with detailed operator guidance
- Added all 37 GRBL error code descriptions
- Firmware $I query displays version info in settings panel toolbar
- Log-to-file toggle with INI persistence in Config

## Task Commits

1. **Task 1: Alarm/error reference tables** - `639cfd1` (feat)
2. **Task 2: Firmware info, log toggle** - `639cfd1` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. Test expectations updated**
- **Found during:** Task 1
- **Issue:** Tests expected old alarm/error description strings
- **Fix:** Updated test expectations in test_cnc_controller.cpp
- **Committed in:** 639cfd1

---
*Phase: 12-extended*
*Completed: 2026-02-27*
