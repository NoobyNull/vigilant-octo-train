---
phase: 06-job-safety
plan: 02
subsystem: cnc
tags: [grbl, pins, preflight, safety, parsing]

requires:
  - phase: 05
    provides: CncController with parseStatusReport
provides:
  - Pn: field parsing in GRBL status reports
  - PIN_* bitmask constants for 8 input pins
  - PreflightCheck utility (4 errors, 2 warnings)
  - 5 new Pn: parsing tests
affects: [06-03, 06-04, safety-panel, streaming]

tech-stack:
  added: []
  patterns: [bitmask pin constants, free function preflight check]

key-files:
  created:
    - src/core/cnc/preflight_check.h
    - src/core/cnc/preflight_check.cpp
  modified:
    - src/core/cnc/cnc_types.h
    - src/core/cnc/cnc_controller.cpp
    - tests/test_cnc_controller.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Pin constants use constexpr u32 bitmask (1 << N) pattern"
  - "PreflightCheck is a free function, not a class (stateless)"
  - "Pre-flight errors block streaming, warnings are informational"

patterns-established:
  - "Pin bitmask: cnc::PIN_X_LIMIT through cnc::PIN_START"
  - "Pre-flight severity: Error blocks, Warning informs"

requirements-completed: [SAF-06, SAF-07]

duration: 6min
completed: 2026-02-24
---

# Plan 06-02: Pn: Parsing + Pre-flight Summary

**GRBL input pin parsing (Pn: field) and pre-flight validation utility with 4 error and 2 warning checks**

## Performance

- **Duration:** 6 min
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- PIN_X_LIMIT through PIN_START constants for all 8 GRBL v1.1 input pins
- Pn: field parsed from status reports into MachineStatus.inputPins bitmask
- runPreflightChecks validates connection, alarm, error state, streaming (errors) and tool/material (warnings)
- 5 new tests for Pn: parsing and pin constants

## Task Commits

1. **Task 1: Pin constants and Pn: parsing** - `3a8f0a7` (feat)
2. **Task 2: Pre-flight check utility** - `3a8f0a7` (feat, same commit)

## Files Created/Modified
- `src/core/cnc/cnc_types.h` - PIN_* bitmask constants
- `src/core/cnc/cnc_controller.cpp` - Pn: field parsing in parseStatusReport
- `src/core/cnc/preflight_check.h` - PreflightIssue struct and runPreflightChecks
- `src/core/cnc/preflight_check.cpp` - Pre-flight check implementation
- `tests/test_cnc_controller.cpp` - 5 new Pn: parsing tests

## Decisions Made
- Free function for pre-flight checks (no state needed, easy to call from multiple panels)
- inputPins defaults to 0 in MachineStatus struct (GRBL omits Pn: when no pins active)

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None

## Next Phase Readiness
- Pin constants ready for safety panel sensor display (Plan 03)
- Pre-flight checks ready for streaming gates (Plans 03 and 04)

---
*Phase: 06-job-safety*
*Completed: 2026-02-24*
