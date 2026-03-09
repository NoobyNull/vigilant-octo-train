---
gsd_state_version: 1.0
milestone: v0.5.5
milestone_name: Unified 3D Viewport
status: executing
last_updated: "2026-03-09T15:04:06.000Z"
progress:
  total_phases: 5
  completed_phases: 2
  total_plans: 3
  completed_plans: 3
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-09)

**Core value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.
**Current focus:** Milestone v0.5.5 Unified 3D Viewport -- Phase 32 complete, Phase 33 next

## Current Position

Phase: 32 of 35 (Viewport Toolbar & Toggles)
Plan: 1 of 1
Status: Phase 32 Complete
Last activity: 2026-03-09 -- Completed 32-01-PLAN.md (Viewport Toolbar & Toggles)

Progress: [####......] 40%

## Accumulated Context

### Decisions
- Unified viewport (Option B) -- merge all 3D rendering into ViewportPanel
- FitParams as alignment source of truth for model-gcode overlay
- Point-match validation (1% sample) confirms alignment, doesn't solve it
- GCodePanel loses 3D rendering, keeps control/info role
- External .nc files render in viewport, no model required
- Phase numbering continues from 31 (v0.5.0 ended at 30)
- No research phase -- internal architectural work
- Height-line shader owned by Renderer (not per-panel) for shared access
- No filter toggles in Phase 31 -- all move types render unconditionally
- G-code lines are separate rendering layer from existing toolpath mesh
- Callback wiring in application_wiring_cnc.cpp alongside existing GCodePanel setup
- Null-check on viewportPanel() pointer for safety against panel ordering changes
- Reused GCodePanel's 8-color toolColor palette and ToolGroup pattern for consistency
- Z-clip filtering in G-code space (not renderer Y-up space) matching GCodePanel convention
- Move-type filtering at geometry build time for GPU efficiency

### Pending Todos
None.

### Blockers/Concerns
None.

## Session Continuity

Last session: 2026-03-09
Stopped at: Completed 32-01-PLAN.md
Resume file: None
Next action: Plan Phase 33 (Alignment Overlay)

---
*State initialized: 2026-02-27*
*Last updated: 2026-03-09 -- 32-01 completed (Viewport Toolbar & Toggles)*
