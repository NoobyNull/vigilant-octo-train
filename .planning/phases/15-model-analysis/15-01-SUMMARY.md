---
phase: 15-model-analysis
plan: 01
subsystem: carve
tags: [curvature, heightmap, surface-analysis, discrete-laplacian, tool-sizing]

requires:
  - phase: 14-heightmap-generation
    provides: Heightmap class with grid accessors and resolution
provides:
  - CurvatureResult struct with min/avg concave radius
  - analyzeCurvature() free function for heightmap curvature analysis
  - computeLocalRadius() helper for per-cell radius of curvature
affects: [15-02, 16-tool-recommendation]

tech-stack:
  added: []
  patterns: [discrete-laplacian-curvature, noise-threshold-filtering, neighbor-validation]

key-files:
  created:
    - src/core/carve/surface_analysis.h
    - src/core/carve/surface_analysis.cpp
    - tests/test_surface_analysis.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Noise threshold derived from grid resolution: 0.001 / res^2 to filter numerical noise while preserving gentle curvature"
  - "2+ concave neighbor requirement to reject isolated noise spikes"
  - "Free functions (not class) for stateless curvature analysis"

patterns-established:
  - "Curvature analysis via discrete central differences on heightmap grid"
  - "Noise filtering: resolution-derived threshold + neighbor validation"

requirements-completed: [DC-05]

duration: 6min
completed: 2026-02-28
---

# Phase 15 Plan 01: Minimum Feature Radius from Heightmap Curvature Summary

**Discrete Laplacian curvature analysis on heightmap grid with noise-filtered minimum concave radius detection for CNC tool sizing**

## Performance

- **Duration:** 6 min
- **Started:** 2026-02-28T19:37:15Z
- **Completed:** 2026-02-28T19:43:22Z
- **Tasks:** 1 (single implementation task with tests)
- **Files modified:** 5

## Accomplishments
- Implemented surface curvature analysis computing minimum concave feature radius from heightmap
- Noise filtering via resolution-derived threshold and 2+ concave neighbor validation
- 5 tests covering flat, parabolic bowl, V-groove, large radius, and location accuracy
- All 749 tests passing (5 new CurvatureAnalysis tests added)

## Task Commits

Each task was committed atomically:

1. **Surface analysis implementation + tests** - `1eef6bd` (feat)

## Files Created/Modified
- `src/core/carve/surface_analysis.h` - CurvatureResult struct, analyzeCurvature/computeLocalRadius declarations
- `src/core/carve/surface_analysis.cpp` - Discrete Laplacian curvature with noise filtering
- `tests/test_surface_analysis.cpp` - 5 tests: FlatSurface, SphericalBowl, VGroove, LargeRadius, MinRadiusLocation
- `src/CMakeLists.txt` - Added surface_analysis.cpp to main build
- `tests/CMakeLists.txt` - Added test_surface_analysis.cpp to test build

## Decisions Made
- Noise threshold set to `0.001 / res^2` rather than the plan's suggested `2.0 / res^2`, which was too aggressive and rejected all practical curvature. The lower threshold correctly filters float precision noise while detecting gentle curves.
- Used parabolic bowl (constant curvature) for the sphere test instead of a hemisphere cap, avoiding rim-discontinuity artifacts that would produce misleadingly small minimum radii.
- Free functions in dw::carve namespace (no class needed for stateless analysis)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed noise threshold coefficient**
- **Found during:** Implementation + test validation
- **Issue:** Plan-specified threshold `2.0 / res^2` was too aggressive -- with 0.5mm resolution, threshold=8.0 rejected all curvature with radius > 0.125mm, making the analysis useless for practical tool sizing
- **Fix:** Changed coefficient to `0.001 / res^2` which correctly filters numerical noise while preserving real surface curvature
- **Files modified:** src/core/carve/surface_analysis.cpp
- **Verification:** SphericalBowl test now correctly detects curvature with R=5mm
- **Committed in:** 1eef6bd

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Essential correction -- the plan's threshold formula would have made curvature detection non-functional for any practical use case.

## Issues Encountered
None beyond the threshold correction noted above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- CurvatureResult available for Phase 15-02 (island detection) and Phase 16 (tool recommendation)
- analyzeCurvature() takes a const Heightmap& reference, ready for integration into CarveJob pipeline

---
*Phase: 15-model-analysis*
*Completed: 2026-02-28*
