---
phase: 31-core-toolpath-rendering
plan: 01
subsystem: render
tags: [opengl, glsl, gcode, toolpath, shader, viewport]

# Dependency graph
requires: []
provides:
  - "Height-line shader compiled by Renderer and accessible via heightLineShader()"
  - "ViewportPanel G-code line rendering pipeline: setGCodeProgram -> buildGCodeGeometry -> renderGCodeLines"
  - "Height-based depth coloring for G-code cutting paths (blue gradient)"
  - "Flat-colored rapids/plunges/retracts matching GCodePanel palette"
affects: [31-02, 32-viewport-filters, 34-simulation-overlay]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "G-code Y/Z swap during geometry building (Z-up G-code to Y-up renderer)"
    - "4-group segment classification: rapid/cut/plunge/retract with Z-dominant heuristic"
    - "Single VBO with draw ranges for grouped GL_LINES rendering"

key-files:
  created: []
  modified:
    - "src/render/renderer.h"
    - "src/render/renderer.cpp"
    - "src/ui/panels/viewport_panel.h"
    - "src/ui/panels/viewport_panel.cpp"

key-decisions:
  - "Height-line shader owned by Renderer (not per-panel) for shared access"
  - "No filter toggles in Phase 31 -- all move types render unconditionally"
  - "G-code lines are a separate rendering layer from existing toolpath mesh"

patterns-established:
  - "Renderer shader accessor pattern: compile in createShaders(), expose via typed accessor"
  - "ViewportPanel G-code API: setGCodeProgram/clearGCodeProgram/hasGCode"

requirements-completed: [VPR-01, VPR-07]

# Metrics
duration: 3min
completed: 2026-03-09
---

# Phase 31 Plan 01: Core Toolpath Line Rendering Summary

**GL_LINES toolpath rendering in unified ViewportPanel with height-colored cuts and flat-colored rapids/plunges/retracts using shared Renderer shader**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-09T14:42:40Z
- **Completed:** 2026-03-09T14:45:19Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Height-line shader compiled by Renderer and exposed via heightLineShader() accessor
- Full G-code line rendering pipeline in ViewportPanel: setGCodeProgram triggers geometry build, renderGCodeLines draws with per-group shading
- Segment classification using Z-dominant heuristic separates rapids, cuts, plunges, and retracts into a single VBO with draw ranges
- Y/Z coordinate swap correctly transforms G-code Z-up to renderer Y-up during geometry building

## Task Commits

Each task was committed atomically:

1. **Task 1: Add height-line shader to Renderer** - `3149098` (feat)
2. **Task 2: Add G-code line geometry infrastructure and rendering to ViewportPanel** - `e4469eb` (feat)

## Files Created/Modified
- `src/render/renderer.h` - Added m_heightLineShader member and heightLineShader() accessor
- `src/render/renderer.cpp` - Added HEIGHT_LINE shader compilation in createShaders()
- `src/ui/panels/viewport_panel.h` - Added G-code program storage, VAO/VBO members, draw range counters, public API
- `src/ui/panels/viewport_panel.cpp` - Implemented setGCodeProgram, clearGCodeProgram, buildGCodeGeometry, renderGCodeLines, integrated into renderViewport()

## Decisions Made
- Height-line shader is owned by Renderer (not created per-panel) -- follows existing pattern where flatShader/meshShader/gridShader are all compiled once and shared
- No filter toggles (m_showRapid, etc.) added yet -- plan explicitly defers those to Phase 32
- G-code line geometry uses a separate VAO/VBO from the existing toolpath mesh (m_gpuToolpath) which is triangle-based from Direct Carve

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed lint compliance: comment line too long**
- **Found during:** Task 2 (build verification)
- **Issue:** Comment on heightLineShader() accessor was 122 chars, exceeding 120-char lint limit
- **Fix:** Shortened the comment from detailed uniform listing to concise description
- **Files modified:** src/render/renderer.h
- **Verification:** All 934 tests pass including LintCompliance.NoLongLines
- **Committed in:** e4469eb (part of Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Minor formatting fix, no scope change.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- ViewportPanel can now receive and render G-code programs via setGCodeProgram()
- Plan 02 will wire the GCodePanel's loaded program into ViewportPanel and add visual verification checkpoint
- Filter toggles, color-by-tool, and simulation overlay deferred to Phases 32/34 as planned

## Self-Check: PASSED

All files exist, all commits verified.

---
*Phase: 31-core-toolpath-rendering*
*Completed: 2026-03-09*
