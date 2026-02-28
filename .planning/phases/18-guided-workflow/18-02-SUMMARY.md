---
phase: 18-guided-workflow
plan: "02"
subsystem: ui
tags: [imgui, carve, heightmap, toolpath, cnc, gcode]

requires:
  - phase: 15-surface-analysis
    provides: CurvatureResult, IslandResult, analyzeCurvature, detectIslands
  - phase: 17-toolpath-generation
    provides: ToolpathGenerator, Toolpath, MultiPassToolpath
  - phase: 18-01
    provides: DirectCarvePanel wizard framework, step navigation
provides:
  - renderModelFit with stock dims, scale/depth/position, live fit, heightmap generation
  - renderPreview with toolpath overlay, statistics, zoom controls
  - renderOutlineTest with G90 perimeter trace at safe Z
  - CarveJob analyzeHeightmap and generateToolpath methods
affects: [19-gcode-streaming, direct-carve-pipeline]

tech-stack:
  added: []
  patterns: [ImDrawList toolpath overlay, worldToScreen coordinate mapping]

key-files:
  created: []
  modified:
    - src/core/carve/carve_job.h
    - src/core/carve/carve_job.cpp
    - src/ui/panels/direct_carve_panel.h
    - src/ui/panels/direct_carve_panel.cpp

key-decisions:
  - "CarveJob analysis and toolpath generation run synchronously on main thread (fast enough for interactive use)"
  - "Toolpath preview uses ImDrawList polylines over a dummy area (no GL texture for heightmap image yet)"
  - "Outline test sends 6 G-code commands: G90 retract, then 5 G0 moves tracing toolpath bounding rectangle"
  - "Scale slider range derived from stock/model ratio (not hardcoded)"

patterns-established:
  - "worldToScreen lambda for mapping toolpath XY to ImGui screen coordinates"
  - "CarveJob as single owner of analysis results + toolpath (analyze then generate pattern)"

requirements-completed: [DC-22, DC-23, DC-24]

duration: 14min
completed: 2026-02-28
---

# Phase 18 Plan 02: Model Fitting UI, Preview Rendering, Outline Test Summary

**Model fit step with live stock/machine bounds validation, toolpath preview with color-coded ImDrawList overlay, and G90 perimeter outline test**

## Performance

- **Duration:** 14 min
- **Started:** 2026-02-28T20:22:45Z
- **Completed:** 2026-02-28T20:36:33Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- CarveJob extended with analyzeHeightmap() and generateToolpath() for complete analysis-to-toolpath pipeline
- Model fit step uses DragFloat/SliderFloat with dynamic ranges derived from stock/model dimensions
- Toolpath preview renders finishing (blue), clearing (red), and rapids (green) as polylines via ImDrawList
- Outline test sends G90 absolute positioning commands tracing the toolpath bounding rectangle at safe Z

## Task Commits

Each task was committed atomically:

1. **Task 1: CarveJob analysis and toolpath methods** - `b0527ee` (feat)
2. **Task 2: Panel renderModelFit, renderPreview, renderOutlineTest** - `42441bc` (feat)

## Files Created/Modified
- `src/core/carve/carve_job.h` - Added analyzeHeightmap, generateToolpath, toolpath accessors, analysis state
- `src/core/carve/carve_job.cpp` - Implementation of analysis pipeline and toolpath generation
- `src/ui/panels/direct_carve_panel.h` - Added preview/overlay/outline state members, tool geometry members
- `src/ui/panels/direct_carve_panel.cpp` - Full renderModelFit, renderPreview, renderOutlineTest implementations

## Decisions Made
- CarveJob analysis runs synchronously on main thread -- curvature analysis and island detection are fast enough for interactive use
- Scale slider max range computed as 2x (stock width / model X extent) -- avoids hardcoded upper bound
- Outline test uses 6 G-code commands: G90 Z retract, then 4 corner G0 moves + return to start
- Toolpath preview area sized to heightmap aspect ratio with user-adjustable zoom slider

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added missing cnc_tool.h include to carve_job.h**
- **Found during:** Task 1
- **Issue:** VtdbToolGeometry type not found -- missing include for cnc_tool.h
- **Fix:** Added `#include "../cnc/cnc_tool.h"` to carve_job.h
- **Files modified:** src/core/carve/carve_job.h
- **Verification:** Build succeeds
- **Committed in:** b0527ee

**2. [Rule 1 - Bug] Fixed VtdbToolGeometry namespace qualification**
- **Found during:** Task 2
- **Issue:** Used `carve::VtdbToolGeometry` but VtdbToolGeometry is in `dw::` namespace
- **Fix:** Removed `carve::` prefix
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Verification:** Build succeeds, 804/804 tests pass
- **Committed in:** 42441bc

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug)
**Impact on plan:** Both auto-fixes necessary for compilation. No scope creep.

## Issues Encountered
- Parallel execution with plan 18-01 created a linter race condition where the linter kept modifying direct_carve_panel.cpp between read/write operations. Resolved by using bash heredoc to write the file atomically.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- DirectCarvePanel has all wizard steps implemented through commit confirmation
- Phase 19 (G-code streaming) can wire the Running step to CncController
- CarveJob now provides complete pipeline: heightmap -> analysis -> toolpath

## Self-Check: PASSED

All files found, all commits verified, 804/804 tests passing.

---
*Phase: 18-guided-workflow*
*Completed: 2026-02-28*
