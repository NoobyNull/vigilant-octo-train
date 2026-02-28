# Milestones: Digital Workshop

## Completed Milestones

### v0.1.x: CNC Controller Suite (2026-02-24 → 2026-02-25)

**Goal:** Integrate existing subsystems (tool database, feeds/speeds calculator, material system, GRBL controller) into a usable CNC workflow.

**Phases:** 8 (all complete)
- Phase 1: Fix Foundation (thread safety, disconnect detection, DTR suppression)
- Phase 2: Status Display (DRO, machine state, feed/spindle)
- Phase 3: Manual Control (jog, homing, WCS, MDI console)
- Phase 4: Tool Integration (tool selector, material selector, feeds/speeds)
- Phase 5: Job Streaming (progress, time estimation, feed deviation)
- Phase 6: Job Safety (pause/resume/abort, sensor display, pre-flight, draw outline)
- Phase 7: Firmware Settings (GRBL $$ editor, NC/NO toggles, backup/restore)
- Phase 8: Macros (SQLite storage, built-in macros, keyboard shortcuts)

**Requirements:** 39 total, 39 complete
**Last phase number:** 8

## Planned Milestones

### v0.3.0: Direct Carve (planned 2026-02-28)

**Goal:** Stream 2.5D carving toolpaths directly from STL models to the CNC machine — no G-code files, no external CAM software — using a guided workflow with automatic tool recommendation and surgical clearing.

**Phases:** 6 (planned)
- Phase 14: Heightmap Engine (STL → 2D heightmap, model fitting, background computation)
- Phase 15: Model Analysis (minimum feature radius, island detection, visual annotation)
- Phase 16: Tool Recommendation (automatic tool selection from database, V-bit/ball nose/clearing)
- Phase 17: Toolpath Generation (raster scan, stepover presets, milling direction, surgical island clearing)
- Phase 18: Guided Workflow (step-by-step wizard with safety checks, preview, outline test)
- Phase 19: Streaming Integration (point-by-point streaming, job control, G-code file export)

**Requirements:** 30 total (DC-01 through DC-30), see REQUIREMENTS-DIRECT-CARVE.md
**Branch:** feature/direct-carve

---
*Created: 2026-02-26*
*Last updated: 2026-02-28 — v0.3.0 Direct Carve milestone added*
