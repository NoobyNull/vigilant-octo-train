---
phase: 05-job-streaming
plan: 01
subsystem: cnc-ui
tags: [cnc, job-progress, streaming, time-estimation]

requires: [cnc_types.h, cnc_controller.h, panel.h]
provides: [cnc_job_panel.h, cnc_job_panel.cpp, cnc_tool_panel getters]
affects: [CMakeLists.txt]

tech-stack:
  patterns: [Panel inheritance, snprintf formatting, ImGui progress bar]

key-files:
  created:
    - src/ui/panels/cnc_job_panel.h
    - src/ui/panels/cnc_job_panel.cpp
  modified:
    - src/ui/panels/cnc_tool_panel.h
    - src/CMakeLists.txt

key-decisions:
  - New CncJobPanel class rather than extending CncStatusPanel (separation of concerns)
  - Time formatting uses adaptive HH:MM:SS / MM:SS format
  - ETA calculation uses simple line-rate (naturally adjusts with feed rate changes)
  - ETA hidden until 5 lines / 3 seconds to avoid wild estimates

requirements-completed: [TAC-05, TAC-06, TAC-07, TAC-08]
duration: 5 min
completed: 2026-02-24
---

# Phase 05 Plan 01: CncJobPanel with Progress Display Summary

CncJobPanel created with elapsed time (HH:MM:SS), line-rate-based remaining time estimate, current/total line counts, and progress bar with percentage. CncToolPanel gains getRecommendedFeedRate/hasCalcResult/getCalcResult getters for downstream feed deviation comparison.

## Tasks
- Task 1: Created CncJobPanel header and implementation (2 files)
- Task 2: Added CalcResult getters to CncToolPanel

## Deviations from Plan
None - plan executed exactly as written.

## Next
Ready for 05-02 (feed deviation + UIManager wiring).
