# Research Summary: v1.1 Library Storage & Organization

**Synthesized:** 2026-02-21
**Sources:** STACK.md, FEATURES.md, ARCHITECTURE.md, PITFALLS.md

---

## Critical Finding: Kuzu Abandoned → GraphQLite

Kuzu Inc. archived `kuzudb/kuzu` on October 10, 2025. No migration path, docs domain unreachable. Two community forks exist (Bighorn, RyuGraph) but neither has traction.

**Resolution:** [GraphQLite](https://github.com/colliery-io/graphqlite) — an MIT-licensed SQLite extension (v0.3.5, Feb 18 2026) that adds Cypher graph queries directly inside SQLite. Written in C (80.4%). Loaded via `sqlite3_load_extension()`. Same DB file, same connection pool, same API. Includes 15+ graph algorithms (PageRank, Louvain, Dijkstra, BFS/DFS).

**Architecture simplification:** Single SQLite database contains relational tables + FTS5 virtual tables + GraphQLite graph data. No second database engine, no second connection pool, no second file.

---

## Technology Decisions (Post-Research)

| Technology | Decision | Notes |
|------------|----------|-------|
| Graph queries | GraphQLite SQLite extension | Cypher via SQL, loaded at startup |
| Full-text search | FTS5 (already in SQLite) | Enable with `SQLITE_ENABLE_FTS5` compile flag |
| Content-addressable storage | `std::filesystem` + atomic rename | `blobs/ab/cd/abcdef...ext` directory structure |
| .dwproj export | miniz (already linked) | ZIP archive with manifest + model blobs |
| NAS detection | `statfs(2)` (Linux), `f_fstypename` (macOS), `GetDriveTypeW()` (Windows) | No new dependency |

---

## Architecture Consensus

### StorageManager
- New component at `src/core/storage/storage_manager.h/.cpp`
- Peer of LibraryManager (both owned by Application)
- Hash-based blob path: `blobs/ab/cd/abcdef...ext`
- Write-to-temp + verify-hash + atomic rename pattern (prevents partial write corruption)
- Orphan temp file cleanup on startup

### Import Pipeline Changes (Additive)
- Stage 5: compute blob path from hash (pure function)
- Stage 5.5: StorageManager copies/moves file to blob store
- Stage 5.6: GraphQLite graph insert (non-fatal)
- Existing stages 1-4 unchanged

### FTS5 Integration
- External-content FTS5 table with BM25 column weighting
- **CRITICAL:** Use BEFORE UPDATE triggers (not AFTER UPDATE) for delete phase — AFTER causes silent token corruption
- Porter stemmer via `tokenize='porter ascii'`
- One-time `INSERT INTO models_fts(models_fts) VALUES('rebuild')` for existing rows

### Categories
- Extend `models` table with category columns (no new manager)
- Shallow 2-level hierarchy max (genus > species)
- 32 existing wood species from MaterialsPanel as seed data
- Many-to-many assignment

### GraphQLite Integration
- Load extension at DB init via `sqlite3_load_extension()`
- Graph nodes for: Model, Category, Project
- Graph edges for: belongs_to (model→category), contains (project→model), related_to (model→model)
- Cypher queries for relationship traversal, graph algorithms for similarity/clustering
- All in same SQLite DB file — no separate graph database

### Project Export
- New ProjectExportManager in `src/core/export/`
- .dwproj = ZIP with manifest (5 fields: format_version, app_version, created_at, project_id, models[])
- Internal `models/` directory mirrors CAS layout for direct extraction

---

## Top Pitfalls to Prevent

1. **Partial write corruption in CAS** — Never write directly to final hash path. Use temp + verify + rename. Orphan cleanup on startup.
2. **FTS5 BEFORE/AFTER trigger ordering** — MUST use BEFORE UPDATE for delete phase. Mandatory correctness test.
3. **WAL mode + ATTACH breaks atomicity** — Keep all data in single SQLite file (GraphQLite makes this natural).
4. **Windows MAX_PATH** — CAS paths in long install dirs can exceed 260 chars. Need `longPathAware` manifest + CI testing.
5. **NAS locking unreliability** — Never rely on file locks over NFS/SMB. Always copy from NAS, never move.
6. **Unicode filename normalization** — NFC normalize filenames before hashing for cross-platform consistency.
7. **GraphQLite extension loading** — Must enable `sqlite3_enable_load_extension()` before loading. Test extension availability at startup with graceful fallback if missing.

---

## Recommended Phase Order

1. **CAS Foundation** — StorageManager, blob directory structure, write-temp-verify-rename, orphan cleanup
2. **File Handling Mode** — Import dialog with Leave/Copy/Move options, NAS auto-detection
3. **Schema + FTS5** — New schema with FTS5 virtual table, BEFORE triggers, category columns
4. **Category System** — Category hierarchy UI, assignment, filtering
5. **Search** — FTS5 search bar in library panel, BM25 ranking, prefix-match
6. **GraphQLite Integration** — Extension loading, graph schema, relationship CRUD, Cypher queries
7. **Project Export** — .dwproj ZIP archive with manifest

---

## Open Questions for Requirements

- Target depth for category hierarchy? (Recommended: 2 levels max)
- Should thumbnails be content-addressed or stored by model ID? (Recommended: by model ID — simpler, regenerable)
- Multi-hop graph traversal needed, or only direct project-model and category-parent links?

---

## Confidence Assessment

| Area | Level | Reason |
|------|-------|--------|
| CAS design | HIGH | Git/Docker/iTunes patterns well-documented; POSIX rename atomicity is canonical |
| FTS5 integration | HIGH | Official SQLite docs cover all questions; trigger ordering confirmed on forums |
| GraphQLite fit | MEDIUM | Actively maintained, MIT, correct architecture — but v0.3.x is young, C++ integration via extension loading is standard but needs testing |
| NAS detection | HIGH | OS-level APIs well-documented (statfs, GetDriveTypeW) |
| File handling UX | HIGH | Lightroom Add/Copy/Move is authoritative pattern; existing FileHandlingMode enum ready |
| Category hierarchy | HIGH | NN/g breadcrumb research authoritative; AEM taxonomy patterns validated |
| Project export | MEDIUM | Pattern consistent across systems; no single standard for desktop app archives |

**Overall: MEDIUM-HIGH** — Core technologies are well-understood. GraphQLite is the only novel element requiring validation during implementation.

---

## Sources

- [GraphQLite](https://github.com/colliery-io/graphqlite) — SQLite graph extension with Cypher
- [SQLite FTS5](https://sqlite.org/fts5.html) — Full-text search
- [SQLite Run-Time Loadable Extensions](https://sqlite.org/loadext.html) — Extension loading API
- [Kuzu archived](https://github.com/kuzudb/kuzu) — Abandoned Oct 2025

---
*Research completed: 2026-02-21*
*Ready for requirements: yes*
