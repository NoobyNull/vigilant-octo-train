---
phase: 17-toolpath-generation
plan: 01
subsystem: carve
tags: [toolpath, raster-scan, heightmap, cnc, gcode, stepover, milling-direction]

requires:
  - phase: 14-heightmap
    provides: Heightmap class with atMm bilinear interpolation
  - phase: 15-surface-analysis
    provides: IslandResult and Island types for clearing regions
provides:
  - Raster scan toolpath generator (X/Y/XThenY/YThenX axis modes)
  - Stepover presets (UltraFine 1% through Roughing 40%)
  - Milling direction control (Climb/Conventional/Alternating)
  - Tool offset compensation for V-bit, ball nose, and end mill geometries
  - Travel limit validation
  - Surgical island clearing with depth-pass raster scanning
  - Time and distance estimation for toolpaths
affects: [17-02-gcode-emitter, 17-03-streaming, 18-wizard-ui]

tech-stack:
  added: []
  patterns: [raster-scan-generation, tool-offset-compensation, island-clearing]

key-files:
  created:
    - src/core/carve/toolpath_types.h
    - src/core/carve/toolpath_generator.h
    - src/core/carve/toolpath_generator.cpp
    - tests/test_toolpath_generator.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "V-bit cone flares upward from tip -- offset only when neighbor is above cone body"
  - "Boundary-aware tool offset skips out-of-bounds neighbors to avoid defaultZ artifacts"
  - "Island clearing uses per-island depth passes with ramp entry/exit"
  - "40% stepover and full-diameter stepdown defaults for clearing roughing"

patterns-established:
  - "Tool-specific offset dispatch via VtdbToolType enum"
  - "Raster scan with axis selection and direction alternation"
  - "Island-scoped clearing with mask lookup and lead-in/out ramps"

requirements-completed: [DC-14, DC-15, DC-16]

duration: 9min
completed: 2026-02-28
---

# Phase 17 Plan 01: Raster Scan Generation Summary

**Raster scan toolpath generator with axis selection, stepover presets, milling direction, tool offset compensation, and island clearing**

## Performance

- **Duration:** 9 min
- **Started:** 2026-02-28T20:06:49Z
- **Completed:** 2026-02-28T20:15:58Z
- **Tasks:** 5 (types, header, implementation, CMake, tests)
- **Files modified:** 6

## Accomplishments
- Complete raster scan toolpath generation from heightmap with configurable axis, stepover, and milling direction
- Tool offset compensation for V-bit, ball nose, and end mill geometries with boundary-safe neighbor lookup
- Surgical island clearing with depth-pass raster scanning, mask-based cell membership, and ramp entry/exit
- 14 tests covering all scan modes, stepover spacing, direction alternation, safe Z retract, time estimation, and clearing passes

## Task Commits

Each task was committed atomically:

1. **Task 1: Toolpath types and config structs** - `da10105` (feat)
2. **Task 2: Warnings field and toolpath header** - `c662baf` (feat)
3. **Task 3: Island clearing header extension** - `7753b83` (feat)
4. **Task 4: Tool offset compensation and validation** - `ea8adce` (feat)
5. **Task 5: Island clearing implementation** - `db1ee94` (feat)
6. **Task 6: Clearing pass tests** - `5523ab9` (test)
7. **Task 7: Tool offset tests and bounds fix** - `8ecf661` (test)

## Files Created/Modified
- `src/core/carve/toolpath_types.h` - ScanAxis, MillDirection, StepoverPreset enums; ToolpathConfig, ToolpathPoint, Toolpath, MultiPassToolpath structs
- `src/core/carve/toolpath_generator.h` - ToolpathGenerator class with finishing, clearing, validation, and tool offset methods
- `src/core/carve/toolpath_generator.cpp` - Full implementation (~375 lines): scan lines, tool offsets, island clearing, metrics
- `tests/test_toolpath_generator.cpp` - 14 tests: 8 finishing + 6 clearing/offset
- `src/CMakeLists.txt` - Added toolpath_generator.cpp to build
- `tests/CMakeLists.txt` - Added test file and source dependency

## Decisions Made
- V-bit offset uses upward-flaring cone model: only raises when neighbor surface exceeds cone body height at that XY distance
- Boundary-aware offset computation: skips out-of-bounds heightmap neighbors to prevent defaultZ (0.0) from causing false collision detection on negative-Z surfaces
- Island clearing uses per-island bounding box scan with mask-based cell membership, not full-surface scan
- Clearing depth passes use linear interpolation from island maxZ to minZ with configurable stepdown

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] V-bit offset formula direction**
- **Found during:** Test verification
- **Issue:** V-bit cone formula used downward direction (centerZ - dist/tan) causing false offsets on flat surfaces
- **Fix:** Corrected to upward-flaring model (centerZ + dist/tan) matching physical V-bit geometry
- **Files modified:** src/core/carve/toolpath_generator.cpp
- **Verification:** FlatSurfaceXScan test passes with Z=-5 flat surface

**2. [Rule 1 - Bug] Boundary-aware tool offset lookups**
- **Found during:** Test verification
- **Issue:** Tool offset neighbor lookups sampled outside heightmap bounds, getting defaultZ=0 which caused large false offsets on surfaces with negative Z
- **Fix:** Added bounds clamping to skip out-of-bounds neighbors in vBitOffset, ballNoseOffset, endMillOffset
- **Files modified:** src/core/carve/toolpath_generator.cpp
- **Verification:** All 14 tests pass, 794 total tests pass

**3. [Rule 2 - Enhancement] Extended API beyond plan scope**
- **Found during:** File creation
- **Issue:** Linter extended header with tool offset compensation (VtdbToolGeometry integration), validateLimits, and clearIslandRegion
- **Fix:** Implemented all extended methods to match the expanded header
- **Files modified:** src/core/carve/toolpath_generator.h, toolpath_generator.cpp
- **Verification:** Full test suite passes

---

**Total deviations:** 3 auto-fixed (2 bugs, 1 enhancement)
**Impact on plan:** Bug fixes essential for correctness. Enhancement provides tool-aware offset compensation needed by downstream phases.

## Issues Encountered
None beyond the auto-fixed deviations above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Toolpath generator complete with finishing and clearing passes
- Ready for G-code emitter (17-02) to convert ToolpathPoint sequences to G-code commands
- MultiPassToolpath struct ready for combined clearing+finishing workflow

---
*Phase: 17-toolpath-generation*
*Completed: 2026-02-28*
