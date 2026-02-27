---
phase: 12-extended
plan: 03
subsystem: cnc
tags: [grbl, probe, z-probe, tls, 3d-probing, G38]

requires:
  - phase: 09-safety
    provides: Safety panel and preflight check infrastructure
provides:
  - Probe pin LED indicator in status panel
  - Z-probe workflow with two-stage G38.2 and plate thickness compensation
  - Tool length setter with reference Z capture and G43.1 offset
  - 3D probing workflows (edge X/Y, corner, center)
affects: [cnc-safety, cnc-status, probing]

tech-stack:
  added: []
  patterns: [tabbed modal probe dialog, G38.2 two-stage probe sequence]

key-files:
  modified:
    - src/ui/panels/cnc_status_panel.h
    - src/ui/panels/cnc_status_panel.cpp
    - src/ui/panels/cnc_safety_panel.h
    - src/ui/panels/cnc_safety_panel.cpp

key-decisions:
  - "EXT-06 (out-of-bounds pre-check) already implemented in Phase 9 SAF-15"
  - "Two-stage probe: fast probe then retract and slower accuracy probe"
  - "Probe workflows require Idle state and connected -- all controls disabled otherwise"
  - "TLS uses simplified offset calculation (current Z - reference Z)"

patterns-established:
  - "Probe dialog tabs pattern: modal popup with TabBar, per-tab parameter inputs and command preview"
  - "LED indicator pattern: DrawCircleFilled with green/gray colors"

requirements-completed: [EXT-06, EXT-07, EXT-11, EXT-15, EXT-16]

duration: 12min
completed: 2026-02-27
---

# Plan 12-03: Extended Summary

**Probe workflows with Z-probe, tool length setter, and 3D probing dialogs plus probe LED indicator**

## Performance

- **Duration:** ~12 min
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Probe pin LED indicator (green active, gray inactive) in status panel
- Z-probe dialog with configurable speed, plate thickness, retract, and command preview
- Tool length setter with reference Z capture and G43.1 compensation
- 3D probing with Edge X, Edge Y, Corner, and Center modes

## Task Commits

1. **Task 1: Probe LED indicator** - `90c01cb` (feat)
2. **Task 2: Probe workflow dialogs** - `90c01cb` (feat)

## Deviations from Plan
- EXT-06 was already implemented -- verified existing implementation in preflight_check.cpp

---
*Phase: 12-extended*
*Completed: 2026-02-27*
