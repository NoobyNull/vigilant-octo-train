# Project State: Digital Workshop

**Last Updated:** 2026-02-22
**Current Session:** Phase 02 Plan 02 completed (Import Pipeline CAS wiring)

---

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-02-21)

**Core Value:** A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

**Current Focus:** v1.1 Library Storage & Organization — content-addressable storage, categories, FTS5, GraphQLite graph queries, project export

---

## Current Position

**Milestone:** v1.1 Library Storage & Organization
**Phase:** 2 — Content-Addressable Storage
**Plan:** 02-02 complete, phase 02 complete
**Status:** Ready to plan

```
v1.1 Progress: [=====_______________] 1/4 phases complete
Phase 02: Plan 2/2 complete
```

Last activity: 2026-02-22 — 02-02 Import Pipeline CAS wiring completed (2 tasks, 2m 32s)

---

## Performance Metrics

### v1.0
**Cycle Time:** 3m 36s per plan (23 plans completed)
**Timeline:** 2 days (Feb 8-9, 2026)

### Post-v1.0
**Phase 01:** 6 plans, materials system (completed 2026-02-20)

### v1.1
**Plans completed:** 2
**Cycle time:** 2m 49s avg (2 plans)

| Phase | Plan | Duration | Tasks | Tests |
|-------|------|----------|-------|-------|
| 02 | 01 - StorageManager | 3m 5s | 2 | 9 |
| 02 | 02 - Import Pipeline CAS | 2m 32s | 2 | 0 |

---

## Project Health

**Code Quality:**
- Application.cpp: 374 lines (thin coordinator)
- Manager architecture: UIManager, FileIOManager, ConfigManager

**Test Coverage:**
- 534 tests passing (1 pre-existing failure in STLLoader)
- StorageManager: 9 tests (path computation, atomic store, dedup, cleanup)
- Infrastructure: EventBus (10), ConnectionPool (10), MainThreadQueue (10)
- Bug regression tests: 12
- Phase 01 Materials tests: Comprehensive coverage

**Tech Debt:**
- EventBus wired but unused (6 event types, zero pub/sub calls in production)

---

## Quick Reference

**Project Root:** `/data/DW/`

**Key Files:**
- Roadmap: `.planning/ROADMAP.md`
- Requirements: `.planning/REQUIREMENTS.md`
- Project Definition: `.planning/PROJECT.md`
- Milestone History: `.planning/MILESTONES.md`
- Research: `.planning/research/SUMMARY.md`
- Gap Analysis: `.planning/TODOS.md`

**Manager Architecture:**
- UIManager: `src/managers/ui_manager.h/.cpp`
- FileIOManager: `src/managers/file_io_manager.h/.cpp`
- ConfigManager: `src/managers/config_manager.h/.cpp`

**Existing Infrastructure (relevant to v1.1):**
- ConnectionPool: `src/core/database/connection_pool.h/.cpp`
- MainThreadQueue: `src/core/threading/main_thread_queue.h/.cpp`
- ThreadPool: `src/core/threading/thread_pool.h/.cpp`
- miniz: Already linked (for ZIP support)

---

## Accumulated Context

### Roadmap Evolution

- v1.0: Foundation & Import Pipeline (shipped 2026-02-10)
- Post-v1.0 Phase 01: Materials system (completed 2026-02-20)
- v1.1: Library Storage & Organization (roadmap created 2026-02-21, 4 phases)

### v1.1 Phase Structure

| Phase | Goal | Requirements |
|-------|------|-------------|
| 2 - CAS Foundation | Hash-based blob store with atomic writes | STOR-01, STOR-02, STOR-03 |
| 3 - Import File Handling | Filesystem detection + import dialog | STOR-04, IMPORT-01, IMPORT-02 |
| 4 - Organization & Graph | Categories, FTS5 search, GraphQLite | ORG-01..05 |
| 5 - Project Export | Portable .dwproj ZIP archives | EXPORT-01, EXPORT-02 |

### v1.1 Design Decisions (2026-02-21)

- **Storage:** Content-addressable with 2-byte hash prefix directories (e.g., `blobs/a7/3b/a73b4f...c821.stl`)
- **Database:** SQLite remains sole database. GraphQLite loaded as SQLite extension for Cypher graph queries. FTS5 for full-text search. Single DB file, single connection pool.
- **Graph DB decision:** Kuzu abandoned Oct 2025 (repo archived). GraphQLite chosen instead — MIT-licensed SQLite extension, actively maintained (v0.3.5, Feb 2026), Cypher support, 15+ graph algorithms, loaded via `sqlite3_load_extension()`.
- **File handling:** Auto-detect source filesystem — copy from NAS/remote, move if same local drive. User picks organizational strategy (keep in place / organize locally / custom location)
- **Categories:** Manual genus assignment by default. AI-assisted classification (Gemini Vision) deferred.
- **Projects:** Lightweight graph links in DB, exportable as .dwproj zip (manifest + model blobs + materials + thumbnails)
- **No migrations:** Delete and recreate DB on schema change (no user base)

### Key Pitfalls to Watch (from research)

1. CAS: Never write directly to final hash path — use temp + verify + rename
2. FTS5: MUST use BEFORE UPDATE triggers (not AFTER) for delete phase
3. WAL mode + ATTACH breaks atomicity — keep all data in single SQLite file
4. Windows MAX_PATH: CAS paths can exceed 260 chars — need `longPathAware` manifest
5. NAS locking unreliable: Always copy from NAS, never move
6. Unicode filename normalization: NFC normalize before hashing
7. GraphQLite: Must call `sqlite3_enable_load_extension()` before loading

---

*State initialized: 2026-02-08*
*v1.1 roadmap created: 2026-02-21*
