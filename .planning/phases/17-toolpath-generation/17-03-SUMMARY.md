---
phase: 17-toolpath-generation
plan: "03"
subsystem: carve
tags: [toolpath, clearing, island, raster, cnc, multi-pass]

# Dependency graph
requires:
  - phase: 15-island-detection
    provides: IslandResult with island mask and bounding boxes
  - phase: 17-toolpath-generation (plans 01-02)
    provides: ToolpathGenerator class, Toolpath types, scan-line generation
provides:
  - Surgical island clearing pass generation (generateClearing)
  - Multi-depth clearIslandRegion with ramp lead-in/out
  - MultiPassToolpath struct combining clearing + finishing
  - leadInMm config for boundary ramp distance
affects: [18-gcode-streaming, 19-carve-wizard]

# Tech tracking
tech-stack:
  added: []
  patterns: [island-mask-membership scan, multi-depth-pass raster clearing, ramp lead-in/out at boundaries]

key-files:
  created: []
  modified:
    - src/core/carve/toolpath_types.h
    - src/core/carve/toolpath_generator.h
    - src/core/carve/toolpath_generator.cpp
    - tests/test_toolpath_generator.cpp

key-decisions:
  - "40% stepover for roughing clearing passes (standard roughing practice)"
  - "Stepdown equals tool diameter for depth passes (conservative safe default)"
  - "0.2mm margin above island floor to avoid over-cutting"
  - "Lead-in/out ramp distance from config (not hardcoded) per coding standards"

patterns-established:
  - "Island-mask membership check: skip non-island cells during raster scan"
  - "Multi-depth clearing: ceil(depth/stepdown) passes from surface to target"

requirements-completed: [DC-18]

# Metrics
duration: 7min
completed: 2026-02-28
---

# Phase 17 Plan 03: Surgical Island Clearing Pass Generation Summary

**Island-aware clearing that raster-scans only within island bounding boxes, with multi-depth passes and ramp lead-in/out at boundaries**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-28T20:07:25Z
- **Completed:** 2026-02-28T20:14:48Z
- **Tasks:** 4
- **Files modified:** 4

## Accomplishments
- Replaced placeholder generateClearing() with surgical island-only clearing
- clearIslandRegion() performs per-island multi-depth raster scanning with mask membership checks
- Ramp lead-in/out at island boundaries instead of plunge/retract (configurable distance)
- MultiPassToolpath struct groups clearing + finishing for combined execution
- 5 clearing pass tests all passing (SingleIsland, DeepIslandMultiPass, NoIslandsNoClearingPath, RampEntry, MultipleIslands)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add MultiPassToolpath and leadInMm config** - `c662baf` (feat)
2. **Task 2: Add clearIslandRegion to generator header** - `7753b83` (feat)
3. **Task 3: Implement generateClearing and clearIslandRegion** - `db1ee94` (feat)
4. **Task 4: Add clearing pass tests** - `5523ab9` (test)

## Files Created/Modified
- `src/core/carve/toolpath_types.h` - Added MultiPassToolpath struct and leadInMm config field
- `src/core/carve/toolpath_generator.h` - Added clearIslandRegion private method declaration
- `src/core/carve/toolpath_generator.cpp` - Replaced clearing stub with island-aware implementation
- `tests/test_toolpath_generator.cpp` - Added 5 ClearingPass tests and helper utilities

## Decisions Made
- 40% stepover for roughing clearing passes (standard CNC roughing practice)
- Stepdown equals tool diameter for multi-depth passes (conservative default)
- 0.2mm margin above island floor prevents over-cutting
- Lead-in/out distance sourced from ToolpathConfig.leadInMm (not hardcoded)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed test compilation: removed duplicate helper functions**
- **Found during:** Task 4 (test creation)
- **Issue:** Parallel plan 17-02 already added makeBallNose/makeEndMill helpers; my additions created duplicates causing -Werror build failure
- **Fix:** Removed duplicate helper definitions
- **Files modified:** tests/test_toolpath_generator.cpp
- **Verification:** Build succeeds, all 794 tests pass
- **Committed in:** 5523ab9 (Task 4 commit, amended)

**2. [Rule 1 - Bug] Fixed SingleIsland test margin to account for lead-in/out ramp**
- **Found during:** Task 4 (test verification)
- **Issue:** Ramp lead-out points extend beyond island bounding box + tool radius; test margin of 2.5mm was too small
- **Fix:** Increased margin to 4.5mm (tool radius 2mm + leadIn 2mm + tolerance 0.5mm)
- **Files modified:** tests/test_toolpath_generator.cpp
- **Verification:** ClearingPass.SingleIsland passes
- **Committed in:** 5523ab9 (Task 4 commit, amended)

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug)
**Impact on plan:** Both fixes necessary for correct compilation and test behavior. No scope creep.

## Issues Encountered
None beyond the auto-fixed deviations.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All three 17-xx plans complete: raster scan generation, tool offset compensation, island clearing
- 20 toolpath tests passing (9 finishing + 5 clearing + 6 offset/limits)
- Ready for Phase 18 (G-code streaming) which will convert Toolpath/MultiPassToolpath to G-code commands

---
*Phase: 17-toolpath-generation*
*Completed: 2026-02-28*
