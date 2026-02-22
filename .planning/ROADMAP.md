# Roadmap: Digital Workshop

## Milestones

- ✅ **v1.0 Foundation & Import Pipeline** — Phases 1-2 (shipped 2026-02-10)
- ✅ **v1.1 Library Storage & Organization** — Phases 2-5 (completed 2026-02-22)

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

<details>
<summary>✅ Post-v1.0 Materials (Phase 1) — COMPLETED 2026-02-20</summary>

- [x] Phase 1: Materials System (6/6 plans) — completed 2026-02-20

</details>

### v1.1 Library Storage & Organization

- [x] **Phase 2: Content-Addressable Storage** — StorageManager with hash-based blob directories, atomic writes, orphan cleanup
- [x] **Phase 3: Import File Handling** — Filesystem detection, import dialog with keep/copy/move options
- [x] **Phase 4: Organization & Graph** — DB schema with FTS5 search, GraphQLite graph extension, categories, Cypher queries
- [x] **Phase 5: Project Export** — Portable .dwproj ZIP archives with manifest and embedded blobs

## Phase Details

### Phase 2: Content-Addressable Storage
**Goal:** Imported models are stored in a reliable, hash-addressed blob store that survives crashes and network failures
**Depends on:** v1.0 import pipeline, post-v1.0 materials system
**Requirements:** STOR-01, STOR-02, STOR-03
**Success Criteria** (what must be TRUE):
  1. Importing a model places its file in `blobs/ab/cd/abcdef...ext` based on content hash
  2. Killing the application mid-import leaves no corrupt or partial files in the blob store
  3. Orphaned temp files from prior crashes are cleaned up automatically on next startup
  4. Importing the same file twice does not create a duplicate blob (dedup by hash)
**Plans:** 2 plans
Plans:
- [x] 02-01-PLAN.md — StorageManager core component + app_paths + unit tests
- [x] 02-02-PLAN.md — Wire StorageManager into ImportQueue and Application lifecycle

### Phase 3: Import File Handling
**Goal:** User understands where their files will go and the application makes smart defaults based on source filesystem
**Depends on:** Phase 2 (CAS must exist to copy/move into)
**Requirements:** STOR-04, IMPORT-01, IMPORT-02
**Success Criteria** (what must be TRUE):
  1. User sees a file handling choice (keep in place / copy to library / move to library) during import
  2. Importing from a NAS or cloud-synced folder auto-selects "copy" and displays a recommendation explaining why
  3. Importing from a local drive defaults to "move" (user can override)
  4. Files imported via "copy" or "move" land in the CAS blob store; "keep in place" preserves original path
**Plans:** 2 plans
Plans:
- [x] 03-01-PLAN.md — Filesystem detector + import options dialog UI
- [x] 03-02-PLAN.md — Wire dialog into import flow with per-batch mode

### Phase 4: Organization & Graph
**Goal:** User can organize models into categories, search across all metadata, and traverse relationships via graph queries
**Depends on:** Phase 2 (CAS blob store for model storage), Phase 3 (import pipeline integrated)
**Requirements:** ORG-01, ORG-02, ORG-03, ORG-04, ORG-05
**Success Criteria** (what must be TRUE):
  1. User can assign a model to a 2-level category (e.g., "Furniture > Chair") and filter the library by category
  2. User can type a search query and get ranked results matching across model name, tags, filename, and category
  3. Models, categories, and projects exist as graph nodes with relationship edges (belongs_to, contains, related_to)
  4. User can query relationships via Cypher (e.g., "all models in project X", "models related to this model")
  5. GraphQLite extension loads at DB init; application starts cleanly with graph support enabled
**Plans:** 4 plans
Plans:
- [x] 04-01-PLAN.md — Schema v7: categories tables, FTS5 virtual table + triggers, ModelRepository methods
- [x] 04-02-PLAN.md — GraphQLite extension loading, Database extension API, GraphManager foundation
- [x] 04-03-PLAN.md — Graph node/edge CRUD, LibraryManager dual-write integration
- [x] 04-04-PLAN.md — Category UI, FTS5 search bar, Application wiring, human verification

### Phase 5: Project Export
**Goal:** User can share a project as a portable, self-contained archive that works on another machine
**Depends on:** Phase 2 (CAS for blob access), Phase 4 (graph for project-model relationships)
**Requirements:** EXPORT-01, EXPORT-02
**Success Criteria** (what must be TRUE):
  1. User can export a project to a .dwproj file containing manifest, model blobs, materials, and thumbnails
  2. A .dwproj file exported on one machine can be imported on a different machine with all models and metadata intact
  3. Export progress is visible and does not block the UI
**Plans:** 2 plans
Plans:
- [x] 05-01-PLAN.md — ProjectExportManager core: export/import logic with miniz ZIP and nlohmann/json manifest
- [x] 05-02-PLAN.md — UI wiring: Export button in ProjectPanel, Import via File menu, ProgressDialog integration

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
| 1 Materials System | Post-v1.0 | 6/6 | Complete | 2026-02-20 |
| 2 Content-Addressable Storage | v1.1 | 2/2 | Complete | 2026-02-22 |
| 3 Import File Handling | v1.1 | 2/2 | Complete | 2026-02-22 |
| 4 Organization & Graph | v1.1 | 4/4 | Complete | 2026-02-22 |
| 5 Project Export | v1.1 | 2/2 | Complete | 2026-02-22 |

---

*Roadmap created: 2026-02-08*
*v1.0 milestone shipped: 2026-02-10*
*v1.1 phases added: 2026-02-21*
*Phase 3 planned: 2026-02-21*
*Phase 4 planned: 2026-02-21*
