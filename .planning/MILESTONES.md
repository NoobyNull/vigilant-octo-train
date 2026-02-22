# Milestones

## v1.0 Foundation & Import Pipeline (Shipped: 2026-02-10)

**Phases completed:** 7 sub-phases, 23 plans, ~46 tasks
**Files:** 84 files changed (9,698 insertions, 1,420 deletions)
**Codebase:** 19,511 LOC C++ in src/
**Tests:** 422 passing
**Timeline:** 2 days (Feb 8-9, 2026)
**Git range:** `refactor(1.4-01)` → `docs(02-10)`

**Delivered:** Decomposed the Application god class, established thread-safe infrastructure (EventBus, ConnectionPool, MainThreadQueue), and built a complete import pipeline with parallel workers, G-code toolpath visualization, and non-blocking background UX.

**Key accomplishments:**
1. Application god class decomposed — 1,108 to 374 lines (66% reduction), extracted UIManager, FileIOManager, ConfigManager
2. Thread-safe infrastructure — EventBus pub/sub, ConnectionPool with WAL+NOMUTEX, MainThreadQueue with documented threading contracts
3. Complete import pipeline — parallel ThreadPool workers, G-code import with auto-association, non-blocking batch processing
4. G-code toolpath visualization — parser, toolpath-to-mesh conversion, color-coded cutting/rapid rendering in viewport
5. Background import UX — StatusBar progress, toast notifications, ImportSummaryDialog, per-frame throttled completion
6. 7 bugs fixed + 6 dead code items resolved — ViewCube cache, shader uniforms, normal matrix, icon system cleanup

---

## v1.2 UI Freshen-Up (Completed: 2026-02-21)

**Phases completed:** 1 phase, 3 plans
**Tests:** 544 passing (no regressions)
**Timeline:** Same day (Feb 21, 2026)

**Delivered:** Modern Inter font replacing the ImGui default bitmap font, DPI-aware scaling with per-monitor support on Windows, and fully hand-tuned dark/light/high-contrast themes.

**Key accomplishments:**
1. Inter font embedded as compressed C array — replaces ProggyClean bitmap font across main app and settings app
2. DPI detection from drawable/window size ratio with Windows per-monitor DPI v2 support
3. Font atlas rebuilt at correct pixel size (not blurry `FontGlobalScale` upscale)
4. Runtime DPI change detection when window moves between monitors
5. Light theme fully customized — all color slots hand-tuned, no more `StyleColorsLight` delegation
6. High contrast theme expanded from 8 colors to full color set for accessibility

---

## v1.1 Library Storage & Organization (Completed: 2026-02-22)

**Phases completed:** 4 phases, 12 plans
**Tests:** 544 passing
**Cycle time:** 3m 4s avg per plan
**Timeline:** 1 day (Feb 21-22, 2026)

**Delivered:** Content-addressable model storage, smart file handling with NAS detection, category-based organization with FTS5 search, GraphQLite graph queries via Cypher, and portable .dwproj project archives.

**Key accomplishments:**
1. Content-addressable storage — hash-based blob directories with atomic writes and orphan cleanup
2. Smart file handling — auto-detect NAS/cloud/local filesystem, recommend copy vs move
3. Organization — 2-level category hierarchy, FTS5 full-text search across all metadata
4. GraphQLite — Cypher graph queries via SQLite extension, models/categories/projects as graph nodes
5. Project export — portable .dwproj ZIP archives with manifest, model blobs, materials, and thumbnails

---

