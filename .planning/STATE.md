---
gsd_state_version: 1.0
milestone: v0.1
milestone_name: milestone
status: unknown
last_updated: "2026-02-28T19:36:32.470Z"
progress:
  total_phases: 28
  completed_phases: 14
  total_plans: 55
  completed_plans: 41
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-28)

**Core value:** A woodworker can load an STL model, select a tool and material, and stream a 2.5D carving toolpath directly to the CNC machine — no G-code files, no external CAM software.
**Current focus:** Milestone v0.3.0 Direct Carve — Phase 15 complete, ready for Phase 16

## Current Position

Phase: Phase 16 — Tool Recommendation (next to implement)
Plan: 16-01 — (first plan in Phase 16)
Status: Phase 15 complete, ready for Phase 16
Last activity: 2026-02-28 — Island detection, analysis overlay, and curvature analysis

## Accumulated Context

### Decisions
- BFS propagation from accessible cells for accurate burial mask computation
- BFS from island boundary inward for minimum clearing tool diameter
- HSV color space with 37-degree hue rotation per island for visually distinct overlays
- Noise threshold for curvature analysis: 0.001/res^2 (plan's 2.0/res^2 too aggressive)
- 2+ concave neighbor requirement filters isolated noise spikes
- Free functions for stateless curvature analysis (no class needed)
- Vertex transformation done eagerly before async launch to avoid shared mutable state
- CarveJob destructor calls cancel+wait for clean shutdown
- Cancellation via atomic bool checked in heightmap progress callback
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
Stopped at: Completed 15-02-PLAN.md (Phase 15 complete)
Resume file: None
Next action: Implement Phase 16, Plan 16-01

---
*State initialized: 2026-02-27*
*Last updated: 2026-02-28T19:45:00Z*
