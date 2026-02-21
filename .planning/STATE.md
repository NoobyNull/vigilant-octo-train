# Project State: Digital Workshop

**Last Updated:** 2026-02-21 (Milestone v1.1 started)
**Current Session:** Defining requirements for v1.1 Library Storage & Organization

---

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-02-21)

**Core Value:** A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

**Current Focus:** v1.1 Library Storage & Organization — content-addressable storage, categories, FTS5, Kuzu graph, project export

---

## Current Position

**Status:** Defining requirements

Phase: Not started (defining requirements)
Plan: —
Status: Defining requirements
Last activity: 2026-02-21 — Milestone v1.1 started

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
- 491 tests passing (1 pre-existing failure in STLLoader)
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
- Project Definition: `.planning/PROJECT.md`
- Milestone History: `.planning/MILESTONES.md`
- Gap Analysis: `.planning/TODOS.md`

**Manager Architecture:**
- UIManager: `src/managers/ui_manager.h/.cpp`
- FileIOManager: `src/managers/file_io_manager.h/.cpp`
- ConfigManager: `src/managers/config_manager.h/.cpp`

---

## Accumulated Context

### Roadmap Evolution

- v1.0: Foundation & Import Pipeline (shipped 2026-02-10)
- Post-v1.0 Phase 01: Materials system (completed 2026-02-20)
- v1.1: Library Storage & Organization (started 2026-02-21)

### v1.1 Design Decisions (2026-02-21)

- **Storage:** Content-addressable with iTunes-style 2-byte hash prefix directories (e.g., `library/models/a7/3b/a73b4f...c821.stl`)
- **Database:** SQLite remains primary (relational + FTS5). Kuzu added alongside for graph relationships (categories, projects)
- **File handling:** Auto-detect source filesystem — copy from NAS/remote, move if same local drive. User picks organizational strategy (keep in place / organize locally / custom location)
- **Categories:** Manual genus assignment by default. AI-assisted classification (Gemini Vision) deferred — not required for customers
- **Projects:** Lightweight graph links in DB, exportable as .dwproj zip (manifest + model blobs + materials + thumbnails)
- **Rejected alternatives:** DuckDB (wrong workload — columnar/analytical, weak OLTP, no graph), MySQL (server-based), SurrealDB (no C++ SDK)

---

*State initialized: 2026-02-08*
*v1.1 milestone started: 2026-02-21*
