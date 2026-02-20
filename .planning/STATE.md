# Project State: Digital Workshop

**Last Updated:** 2026-02-20
**Current Session:** Phase 01 in progress — Plan 05 complete (MaterialsPanel UI)

---

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-02-10)

**Core Value:** A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

**Current Focus:** Phase 01 - Materials mapping with coordinated materials database (Plan 05/6 complete)

---

## Current Position

**Status:** v1.0 Foundation & Import Pipeline shipped 2026-02-10

**Milestone v1.0 Summary:**
- 7 sub-phases, 23 plans, 422 tests passing
- Application.cpp: 1,108 → 374 lines (66% reduction)
- 84 files changed (9,698 insertions, 1,420 deletions)
- 19,511 LOC C++ in src/

**Archives:**
- `.planning/milestones/v1.0-ROADMAP.md`
- `.planning/milestones/v1.0-REQUIREMENTS.md`
- `.planning/milestones/v1.0-MILESTONE-AUDIT.md`
- `.planning/MILESTONES.md`

---

## Performance Metrics (v1.0)

**Cycle Time:** 3m 36s per plan (23 plans completed)
**Timeline:** 2 days (Feb 8-9, 2026)

---

## Project Health

**Code Quality:**
- Application.cpp: 374 lines (thin coordinator)
- Manager architecture: UIManager, FileIOManager, ConfigManager

**Test Coverage:**
- 422 tests passing
- Infrastructure: EventBus (10), ConnectionPool (10), MainThreadQueue (10)
- Bug regression tests: 12

**Tech Debt (from v1.0 audit):**
- EventBus wired but unused (6 event types, zero pub/sub calls in production)
- Phase 01.2 missing formal VERIFICATION.md
- 7 human verification items for import UX

**Remaining Gaps:**
- See `.planning/TODOS.md` for ~44 remaining items
- See `OBSERVATION_REPORT.md` for feature gaps vs old Qt version

---

## Quick Reference

**Project Root:** `/data/DW/`

**Key Files:**
- Roadmap: `.planning/ROADMAP.md`
- Project Definition: `.planning/PROJECT.md`
- Milestone History: `.planning/MILESTONES.md`
- Gap Analysis: `.planning/TODOS.md`

**Manager Architecture:**
- UIManager: `src/managers/ui_manager.h/.cpp`
- FileIOManager: `src/managers/file_io_manager.h/.cpp`
- ConfigManager: `src/managers/config_manager.h/.cpp`

**Resources:**
- Architecture doc: `.planning/codebase/ARCHITECTURE.md`
- Code conventions: `.planning/codebase/CONVENTIONS.md`
- Threading contracts: `docs/THREADING.md`

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 2 | Fix panel UI clipping when docks are resized smaller than content | 2026-02-10 | dc897b7 | [2-fix-panel-ui-clipping-when-docks-are-res](./quick/2-fix-panel-ui-clipping-when-docks-are-res/) |

---

## Accumulated Context

### Roadmap Evolution

- Phase 1 added: Add materials mapping to object view with coordinated materials database

### Phase 01 Decisions (2026-02-20)

- **01-01:** Schema v4 adds materials table; material_id FK on models; MaterialRepository follows ModelRepository pattern
- **01-03:** 32 built-in materials seeded via MaterialManager.seedDefaults() using count()==0 guard (idempotent)
- **01-03:** importMaterial() copies .dwmat to getMaterialsDir() to prevent path invalidation (Pitfall 3)
- **01-03:** MaterialArchive::create() handles empty texturePath for metadata-only archives (default species export)
- **01-04:** Planar UV projection selects largest-area face (XY/XZ/YZ) from mesh bounds
- **01-04:** uUseTexture shader toggle — toolpath overrides texture, texture overrides solid color
- **01-04:** Vertex(pos) and Vertex(pos,norm) constructors zero-initialize texCoord (bug fix)
- **01-05:** MaterialsPanel edit form uses modal popup (not inline) to avoid grid layout height complexity
- **01-05:** Category placeholders use distinct hues: brown (Hardwood), green (Softwood), purple (Domestic), slate (Composite)
- **01-05:** File dialog and insertMaterial() deferred to plan 06 (application wiring)

### Phase 01 Progress (2026-02-20)

- **01-01 DONE:** Dependencies (miniz/stb/nlohmann-json), MaterialRecord, schema v4, MaterialRepository
- **01-02 DONE:** MaterialArchive, TextureLoader, Texture RAII wrapper
- **01-03 DONE:** Default materials (32 species), MaterialManager, getMaterialsDir()
- **01-04 DONE:** Texture mapping pipeline — planar UV generation on Mesh, MESH_FRAGMENT shader with uUseTexture/uMaterialTexture, Renderer textured renderMesh overloads
- **01-05 DONE:** MaterialsPanel — category tabs, thumbnail grid, edit modal, import/export/add/delete toolbar
- **491 tests passing**

---

*State initialized: 2026-02-08*
*Last activity: 2026-02-20 — Phase 01 plan 04 complete (texture mapping pipeline)*
