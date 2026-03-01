---
phase: 19-streaming-integration
plan: 01
subsystem: cnc
tags: [gcode, streaming, carve, cnc, grbl, toolpath]

# Dependency graph
requires:
  - phase: 18-guided-workflow
    provides: G-code export functions, MultiPassToolpath, ToolpathConfig types
  - phase: 13-tcp-ip-byte-stream-transport
    provides: CncController with character-counting streaming protocol
provides:
  - CarveStreamer class for on-the-fly G-code generation from toolpath data
  - CarveJob streaming orchestration (startStreaming/streamer)
  - Line-by-line G-code generation without full file in memory
affects: [19-02-streaming-integration, direct-carve-panel, cnc-job-panel]

# Tech tracking
tech-stack:
  added: []
  patterns: [line-source streaming adapter, modal feed rate optimization, atomic state flags]

key-files:
  created:
    - src/core/carve/carve_streamer.h
    - src/core/carve/carve_streamer.cpp
    - tests/test_carve_streamer.cpp
  modified:
    - src/core/carve/carve_job.h
    - src/core/carve/carve_job.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Phase-based state machine (Preamble/Clearing/Finishing/Postamble/Complete) for nextLine() dispatch"
  - "Modal feed rate: F word emitted only when rate changes (consistent with gcode_export.cpp convention)"
  - "Postamble split into 3 individual lines (retract/M5/M30) for character-counting buffer compatibility"
  - "fmt() helper duplicated from gcode_export.cpp for consistent coordinate formatting"

patterns-established:
  - "Line-source adapter: nextLine() returns one G-code line per call for streaming protocols"
  - "Atomic state flags for cross-thread streaming control (running/paused/aborted)"

requirements-completed: [DC-26, DC-29]

# Metrics
duration: 2min
completed: 2026-02-28
---

# Phase 19 Plan 01: On-the-Fly G-code Generation and Streaming Adapter Summary

**CarveStreamer generates G-code line-by-line from toolpath data for CncController character-counting streaming**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-28T20:35:08Z
- **Completed:** 2026-02-28T20:37:28Z
- **Tasks:** 1 (single atomic implementation)
- **Files modified:** 7

## Accomplishments
- CarveStreamer class with phase-based state machine for clearing + finishing passes
- Modal feed rate optimization (F word only on change, matching gcode_export.cpp)
- Pause/resume/abort with CncController integration (feedHold, cycleStart, softReset)
- CarveJob gains startStreaming() orchestration for direct carve pipeline
- 14 comprehensive tests (all passing), full suite 818/818

## Task Commits

Each task was committed atomically:

1. **Task 1: CarveStreamer implementation + tests + CMake** - `96d8412` (feat)

## Files Created/Modified
- `src/core/carve/carve_streamer.h` - CarveStreamer class header with phase enum, atomic state, format helpers
- `src/core/carve/carve_streamer.cpp` - Implementation: start, nextLine, pause/resume/abort, progress
- `tests/test_carve_streamer.cpp` - 14 tests: preamble, rapid/linear format, feed optimization, pass ordering, postamble, progress, abort, pause, empty toolpath
- `src/core/carve/carve_job.h` - Added startStreaming(), streamer(), CarveStreamer member
- `src/core/carve/carve_job.cpp` - Implemented startStreaming() and streamer() accessor
- `src/CMakeLists.txt` - Added carve_streamer.cpp to DW_SOURCES
- `tests/CMakeLists.txt` - Added test_carve_streamer.cpp and carve_streamer.cpp dep

## Decisions Made
- Phase-based state machine (Preamble/Clearing/Finishing/Postamble/Complete) instead of single index counter -- cleaner dispatch, handles pass transitions naturally
- Postamble emitted as 3 separate lines rather than one multi-line string -- each line fits in GRBL's 128-byte RX buffer independently
- Feed rate tracked with m_lastFeedRate for modal optimization -- only emits F when rate changes, matching existing gcode_export.cpp convention
- fmt() helper duplicated from gcode_export.cpp rather than extracting to shared utility -- keeps both modules self-contained within 800-line limit

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- CarveStreamer ready for integration with DirectCarvePanel streaming workflow (19-02)
- CncController.startStream() can be fed lines from CarveStreamer.nextLine()
- All 818 tests passing

---
*Phase: 19-streaming-integration*
*Completed: 2026-02-28*
