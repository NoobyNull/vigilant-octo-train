---
gsd_state_version: 1.0
milestone: v0.3.0
milestone_name: Direct Carve
status: in-progress
last_updated: "2026-02-28T19:29:08.000Z"
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 14
  completed_plans: 1
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-28)

**Core value:** A woodworker can load an STL model, select a tool and material, and stream a 2.5D carving toolpath directly to the CNC machine — no G-code files, no external CAM software.
**Current focus:** Milestone v0.3.0 Direct Carve — implementing heightmap engine

## Current Position

Phase: Phase 14 — Heightmap Engine
Plan: 14-02 — Heightmap visualization and depth analysis
Status: Plan 14-01 complete, ready for 14-02
Last activity: 2026-02-28 — Heightmap data structure and ray-mesh intersection implemented

## Accumulated Context

### Decisions
- Heightmap API adapted to Vertex+indices (not Triangle vector) matching existing Mesh class
- XY barycentric projection for vertical ray intersection (avoids INF origin issues)
- 64-bucket spatial grid for triangle binning acceleration
- dw::carve namespace established for Direct Carve pipeline
- Branch: feature/direct-carve (from main at commit 9719dd3)
- Pure 2.5D top-down heightmap approach (no undercuts, no 3D CAM)
- V-bit primary finishing tool (taper geometry eliminates need for roughing)
- Ball nose / TBN secondary when minimum feature radius >= tip radius
- Surgical island clearing only (not full surface)
- Stepover presets: Ultra Fine 1%, Fine 8%, Basic 12%, Rough 25%, Roughing 40%
- Scan axis options: X only, Y only, X then Y, Y then X
- Milling direction: climb, conventional, alternating
- Guided wizard workflow enforcing safety at each step
- Model fitting: uniform scale (locked aspect), independent Z depth, XY position
- Streaming via existing CncController character-counting protocol

### Pending Todos
None.

### Roadmap Evolution
- Phases 14-19 added for Direct Carve milestone
- 30 new requirements (DC-01 through DC-30)
- 14 plan files across 6 phases

### Blockers/Concerns
None.

## Session Continuity

Last session: 2026-02-28
Stopped at: Completed 14-01-PLAN.md
Resume file: None
Next action: Implement Phase 14, Plan 14-02 (Heightmap visualization and depth analysis)

---
*State initialized: 2026-02-27*
*Last updated: 2026-02-28T19:29:08Z*
