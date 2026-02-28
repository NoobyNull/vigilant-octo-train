---
phase: 10-core-sender
plan: 02
subsystem: ui
tags: [imgui, grbl, cnc, jog, wcs, config]

requires:
  - phase: 10-core-sender
    provides: CncStatusPanel with m_cnc pointer and override controls from plan 01
provides:
  - 0.01mm precision jog step for zeroing
  - Per-step-group configurable feedrates (small/medium/large)
  - WCS G54-G59 quick-switch selector in status panel header
affects: [settings-app]

tech-stack:
  added: []
  patterns: [per-step-group feedrate config, WCS quick-switch in status panel]

key-files:
  created: []
  modified:
    - src/core/config/config.h
    - src/core/config/config.cpp
    - src/ui/panels/cnc_jog_panel.h
    - src/ui/panels/cnc_jog_panel.cpp
    - src/ui/panels/cnc_status_panel.h
    - src/ui/panels/cnc_status_panel.cpp

key-decisions:
  - "Three feedrate groups (small/medium/large) rather than per-step-size to keep config simple"
  - "WCS selector is independent from CncWcsPanel (convenience shortcut, not synchronized state)"

patterns-established:
  - "Config [cnc] section: jog_feed_small/medium/large for per-group feedrates"
  - "Status panel header: WCS combo right-aligned next to firmware version"

requirements-completed: [SND-06, SND-07, SND-08]

duration: 5min
completed: 2026-02-26
---

# Plan 10-02: Precision Jog, Per-Group Feedrates, WCS Quick-Switch Summary

**0.01mm precision jog step with configurable per-group feedrates and G54-G59 WCS quick-switch in status panel header**

## Performance

- **Duration:** 5 min
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- 0.01mm step size available for precision zeroing (5 total: 0.01, 0.1, 1, 10, 100mm)
- Per-step-group feedrates from Config: small 200, medium 1000, large 3000 mm/min defaults
- Active feedrate and group name displayed below step size selector
- WCS selector (G54-G59) in status panel header, right-aligned next to firmware version
- WCS switch sends corresponding G-code command immediately

## Task Commits

1. **Task 1: 0.01mm step + per-group feedrates** - `900f462` (feat)
2. **Task 2: WCS quick-switch selector** - `9a38912` (feat)

## Files Created/Modified
- `src/core/config/config.h` - Added getJogFeedSmall/Medium/Large getters/setters and members
- `src/core/config/config.cpp` - Added jog_feed_small/medium/large to [cnc] INI section
- `src/ui/panels/cnc_jog_panel.h` - 5 step sizes, removed hardcoded JOG_FEEDS array
- `src/ui/panels/cnc_jog_panel.cpp` - Per-group feedrate lookup from Config, feedrate display
- `src/ui/panels/cnc_status_panel.h` - Added WCS selector members and renderWcsSelector()
- `src/ui/panels/cnc_status_panel.cpp` - WCS combo implementation, called from renderStateIndicator

## Decisions Made
- Three feedrate groups (small/medium/large) rather than per-step-size to keep config manageable
- WCS selector is independent from CncWcsPanel (convenience shortcut, not state sync)

## Deviations from Plan
None - plan executed exactly as written

## Issues Encountered
None

## Next Phase Readiness
- Phase 10 Core Sender complete — all SND requirements implemented
- Ready for verification

---
*Phase: 10-core-sender*
*Completed: 2026-02-26*
