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

