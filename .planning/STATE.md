---
gsd_state_version: 1.0
milestone: v0.1
milestone_name: milestone
status: unknown
last_updated: "2026-02-28T20:37:28Z"
progress:
  total_phases: 28
  completed_phases: 17
  total_plans: 55
  completed_plans: 50
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-28)

**Core value:** A woodworker can load an STL model, select a tool and material, and stream a 2.5D carving toolpath directly to the CNC machine — no G-code files, no external CAM software.
**Current focus:** Milestone v0.3.0 Direct Carve — Phase 17 in progress

## Current Position

Phase: Phase 19 — Streaming Integration (in progress)
Plan: 19-01 — On-the-Fly G-code Generation and Streaming Adapter (complete)
Status: Phase 19 plan 01 complete
Last activity: 2026-02-28 — CarveStreamer for on-the-fly G-code generation

## Accumulated Context

### Decisions
- Coverage fraction scoring for clearing tool value (% of islands it can clear)
- End mill type bonus for clearing over ball nose (flat bottom = faster)
- Card-based ImGui widget with color badges per tool type for recommendations
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
- 40% stepover for roughing clearing passes (standard CNC roughing practice)
- Stepdown equals tool diameter for multi-depth island clearing passes
- 0.2mm margin above island floor prevents over-cutting
- Lead-in/out ramp distance from config (not hardcoded)
- [Phase 17]: V-bit 8-neighbor cone-gouge check, ball-nose drop-cutter, end-mill max-Z-within-radius for tool offset compensation
- V-bit cone flares upward from tip -- offset only when neighbor is above cone body
- Boundary-aware tool offset skips out-of-bounds neighbors to prevent defaultZ artifacts
- [Phase 18]: generateGcode() returns string for testability, exportGcode() wraps with file I/O
- Feed rate F word emitted only on first G1 move (modal G-code convention)
- FileDialog showSave with callback pattern for G-code export
- Abort sends Ctrl+X soft reset (0x18) for immediate machine stop
- [Phase 18]: Panel hidden by default (m_open=false), shown via View > Sender > Direct Carve menu
- [Phase 18]: Theme::Colors helper functions for all wizard status indicators (no hardcoded ImVec4)
- [Phase 18]: Font-relative step indicator sizing for DPI awareness
- [Phase 18-02]: CarveJob analysis/toolpath run synchronously on main thread (fast enough for interactive)
- [Phase 18-02]: Scale slider range derived from stock/model ratio (not hardcoded)
- [Phase 18-02]: ImDrawList polylines for toolpath preview with worldToScreen coordinate mapping
- [Phase 19]: Phase-based state machine (Preamble/Clearing/Finishing/Postamble/Complete) for streaming dispatch
- [Phase 19]: Modal feed rate optimization -- F word emitted only on change
- [Phase 19]: Postamble split into 3 individual lines for GRBL buffer compatibility

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
Stopped at: Completed 19-01-PLAN.md (CarveStreamer on-the-fly G-code generation)
Resume file: None
Next action: Execute 19-02-PLAN.md (streaming UI integration)

---
*State initialized: 2026-02-27*
*Last updated: 2026-02-28T20:37:28Z*
