---
phase: 17-toolpath-generation
plan: 02
subsystem: carve
tags: [cnc, toolpath, tool-offset, v-bit, ball-nose, end-mill, travel-limits]

# Dependency graph
requires:
  - phase: 14-heightmap-generation
    provides: Heightmap API with atMm() bilinear interpolation
  - phase: 17-01
    provides: ToolpathGenerator scan-line generation, toolpath_types.h
provides:
  - Tool offset compensation (V-bit, ball nose, end mill)
  - Machine travel limit validation with user-facing warnings
  - warnings field on Toolpath struct
affects: [18-gcode-streaming, 19-carve-wizard]

# Tech tracking
tech-stack:
  added: []
  patterns: [drop-cutter algorithm for ball nose, cone-gouge check for V-bit, dispatcher pattern for tool-type offset]

key-files:
  created: []
  modified:
    - src/core/carve/toolpath_types.h
    - src/core/carve/toolpath_generator.h
    - src/core/carve/toolpath_generator.cpp
    - tests/test_toolpath_generator.cpp

key-decisions:
  - "8-neighbor gouge check for V-bit offset (cone geometry intersection)"
  - "Drop-cutter algorithm for ball nose Z compensation"
  - "Max-Z-within-radius for end mill flat bottom offset"
  - "First-violation-per-axis for travel limit warnings (avoid warning spam)"
  - "Bounds checking on neighbor sampling to prevent out-of-range heightmap reads"

patterns-established:
  - "Tool-type dispatcher: toolOffset() delegates to vBitOffset/ballNoseOffset/endMillOffset"
  - "Travel validation as pure check returning string warnings (no side effects)"

requirements-completed: [DC-17, DC-19]

# Metrics
duration: 8min
completed: 2026-02-28
---

# Phase 17 Plan 02: Tool Offset Compensation Summary

**V-bit cone-gouge, ball-nose drop-cutter, and end-mill flat-bottom offset compensation with machine travel limit validation**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-28T20:07:06Z
- **Completed:** 2026-02-28T20:15:20Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments
- Tool offset compensation for 3 tool types: V-bit (cone gouge check), ball nose (drop-cutter), end mill (max Z within radius)
- Machine travel limit validation returning per-axis warning strings
- 6 new tests covering all offset types and travel limit edge cases

## Task Commits

Each task was committed atomically:

1. **Task 1: Add warnings field to Toolpath** - `da10105` (feat)
2. **Task 2: Tool offset compensation and travel limits** - `ea8adce` (feat)
3. **Task 3: Tests and bounds fixes** - `8ecf661` (test)

## Files Created/Modified
- `src/core/carve/toolpath_types.h` - Added warnings vector to Toolpath struct
- `src/core/carve/toolpath_generator.h` - Added tool offset methods, validateLimits, VtdbToolGeometry parameter
- `src/core/carve/toolpath_generator.cpp` - Implemented vBitOffset, ballNoseOffset, endMillOffset, validateLimits
- `tests/test_toolpath_generator.cpp` - 6 new tests: VBitFlatSurface, BallNoseCompensation, EndMillCompensation, NoGouging, WithinBounds, ExceedsBounds

## Decisions Made
- V-bit offset uses 8-neighbor cone intersection check at heightmap resolution distance
- Ball nose uses drop-cutter: max(surfaceZ + sqrt(R^2 - dx^2 - dy^2)) within tip radius
- End mill uses simple max-Z-within-radius (flat bottom)
- Travel validation reports first violation per axis to avoid warning spam
- Added bounds checking on all neighbor sampling to prevent edge-of-heightmap issues

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed V-bit cone direction**
- **Found during:** Task 3 (test verification)
- **Issue:** V-bit cone Z was computed as centerZ - dist/tanHalf (flaring downward) instead of centerZ + dist/tanHalf (flaring upward from tip)
- **Fix:** Corrected cone body formula to flare upward
- **Files modified:** src/core/carve/toolpath_generator.cpp
- **Committed in:** 8ecf661

**2. [Rule 1 - Bug] Added bounds checking on neighbor sampling**
- **Found during:** Task 3 (test verification)
- **Issue:** Offset helpers could sample outside heightmap bounds at edges
- **Fix:** Added boundsMin/boundsMax checks before atMm() calls in all three offset helpers
- **Files modified:** src/core/carve/toolpath_generator.cpp
- **Committed in:** 8ecf661

---

**Total deviations:** 2 auto-fixed (2 bugs)
**Impact on plan:** Both fixes necessary for correctness. No scope creep.

## Issues Encountered
- Parallel execution with 17-01 and 17-03: files created by those agents required coordination. Resolved by building on the existing headers and merging changes.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Tool offset compensation ready for integration into carve wizard
- Travel limit validation ready for machine profile integration
- generateFinishing() now accepts VtdbToolGeometry for full tool awareness

## Self-Check: PASSED

All 4 files verified present. All 3 commit hashes verified in git log.

---
*Phase: 17-toolpath-generation*
*Completed: 2026-02-28*
