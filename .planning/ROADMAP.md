# Roadmap: Digital Workshop

## Milestones

- ✅ **v1.0 Foundation & Import Pipeline** — Phases 1-2 (shipped 2026-02-10)

## Phases

<details>
<summary>✅ v1.0 Foundation & Import Pipeline (Phases 1-2) — SHIPPED 2026-02-10</summary>

- [x] Phase 1.1: EventBus (2/2 plans) — completed 2026-02-08
- [x] Phase 1.2: ConnectionPool & WAL (2/2 plans) — completed 2026-02-08
- [x] Phase 1.3: MainThreadQueue & Threading (2/2 plans) — completed 2026-02-08
- [x] Phase 1.4: God Class Decomposition (3/3 plans) — completed 2026-02-08
- [x] Phase 1.5: Bug Fixes (3/3 plans) — completed 2026-02-09
- [x] Phase 1.6: Dead Code Cleanup (1/1 plan) — completed 2026-02-09
- [x] Phase 2: Import Pipeline (10/10 plans) — completed 2026-02-09

See: `.planning/milestones/v1.0-ROADMAP.md` for full details.

</details>

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|---------------|--------|-----------|
| 1.1 EventBus | v1.0 | 2/2 | Complete | 2026-02-08 |
| 1.2 ConnectionPool | v1.0 | 2/2 | Complete | 2026-02-08 |
| 1.3 MainThreadQueue | v1.0 | 2/2 | Complete | 2026-02-08 |
| 1.4 God Class Decomposition | v1.0 | 3/3 | Complete | 2026-02-08 |
| 1.5 Bug Fixes | v1.0 | 3/3 | Complete | 2026-02-09 |
| 1.6 Dead Code Cleanup | v1.0 | 1/1 | Complete | 2026-02-09 |
| 2 Import Pipeline | v1.0 | 10/10 | Complete | 2026-02-09 |

### Phase 1: Add materials mapping to object view with coordinated materials database

**Goal:** CNC woodworker can assign wood species materials to 3D objects with texture-mapped visualization, browsable materials database with 23 built-in species, and portable .dwmat archive import/export
**Depends on:** v1.0
**Plans:** 3/6 plans executed

Plans:
- [x] 01-01-PLAN.md — Dependencies, material types, database schema, MaterialRepository (completed 2026-02-19)
- [x] 01-02-PLAN.md — MaterialArchive (.dwmat ZIP), TextureLoader, Texture RAII wrapper (completed 2026-02-20)
- [ ] 01-03-PLAN.md — Default materials list, MaterialManager import/export/seeding
- [ ] 01-04-PLAN.md — Shader texture support, UV generation, Renderer integration
- [ ] 01-05-PLAN.md — MaterialsPanel UI with category tabs, thumbnail grid, edit form
- [ ] 01-06-PLAN.md — Application wiring, PropertiesPanel integration, human verification

---

*Roadmap created: 2026-02-08*
*v1.0 milestone shipped: 2026-02-10*
