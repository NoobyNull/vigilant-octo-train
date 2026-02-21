# Digital Workshop

## What This Is

Digital Workshop is a cross-platform C++17 desktop application for CNC woodworkers — combining 3D model library management, G-code analysis with toolpath visualization, and 2D cut optimization in a single workspace. Built with SDL2, Dear ImGui (docking), OpenGL 3.3, and SQLite3. Architecture decomposed into focused managers (UIManager, FileIOManager, ConfigManager) with thread-safe infrastructure (EventBus, ConnectionPool, MainThreadQueue).

## Core Value

A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

## Requirements

### Validated

<!-- Shipped and working in v1.0 -->

- ✓ Import STL, OBJ, 3MF models with background processing — v1.0
- ✓ Content-based hash deduplication on import — v1.0
- ✓ Automatic thumbnail generation (offscreen OpenGL render) — v1.0
- ✓ Model library with grid/list view, search, tags — v1.0
- ✓ 3D viewport with orbit camera, ViewCube, lighting — v1.0
- ✓ G-code parsing (G0/G1/G2/G3, feed rates, tool changes) — v1.0
- ✓ 2D G-code path visualization — v1.0
- ✓ G-code timing and statistics — v1.0
- ✓ 2D cut optimization (bin-pack + guillotine algorithms) — v1.0
- ✓ Project create/open/save as .dwp archive — v1.0
- ✓ Project-model associations — v1.0
- ✓ Cost estimation database (CostRepository) — v1.0
- ✓ Dockable ImGui panel layout with persistence — v1.0
- ✓ Dark/light/high-contrast theme system — v1.0
- ✓ Separate settings application with hot-reload — v1.0
- ✓ Configurable input bindings — v1.0
- ✓ Cross-platform build (Linux + Windows CI) — v1.0
- ✓ Self-extracting Linux installer (makeself) — v1.0
- ✓ Windows NSIS installer — v1.0
- ✓ GitHub Actions CI/CD with auto-release — v1.0
- ✓ Model export (STL binary/ASCII, OBJ) — v1.0
- ✓ Application god class decomposed (1,108 → 374 lines) — v1.0
- ✓ EventBus pub/sub for decoupled communication — v1.0
- ✓ SQLite ConnectionPool with WAL mode — v1.0
- ✓ MainThreadQueue for thread-safe worker communication — v1.0
- ✓ Threading contracts documented and enforced — v1.0
- ✓ Parallel import pipeline with ThreadPool — v1.0
- ✓ G-code import with auto-association to models — v1.0
- ✓ 3D toolpath visualization (cutting/rapid color-coded) — v1.0
- ✓ Background import UX (StatusBar, toasts, summary dialog) — v1.0
- ✓ Recursive folder drag-and-drop import — v1.0
- ✓ Hardened mesh loaders (NaN/Inf validation, corrupt file handling) — v1.0
- ✓ 7 bugs fixed (ViewCube cache, shader uniforms, normal matrix, etc.) — v1.0
- ✓ 6 dead code items resolved — v1.0

### Active

<!-- To be defined for next milestone -->

(To be defined — run `/gsd:new-milestone` to start next milestone)

### Out of Scope

- STEP/IGES CAD format support — not a CAD tool
- AI/ML features — keep it deterministic
- Cloud-based operation — fully offline desktop app
- CAD editing — view/import only
- Scientific precision analysis — workshop-grade accuracy
- Mobile platforms — desktop only
- Real-time chat/collaboration — single-user
- Database migration system — no user base, delete and recreate per rulebook

## Context

**Current state:** Shipped v1.0 with 19,511 LOC C++ across ~116 source files. 422 tests passing. Architecture is clean — Application is a thin coordinator (374 lines) delegating to UIManager, FileIOManager, ConfigManager. Thread-safe infrastructure in place (EventBus, ConnectionPool with WAL, MainThreadQueue). Complete import pipeline with parallel workers, G-code support, and non-blocking UX.

**Tech stack:** C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3, CMake 3.20+

**Feature gap reference:** `OBSERVATION_REPORT.md` documents 17 categories of gaps between old and new versions. Major missing subsystems include CNC machine control, cost/quoting workflow, tool database, wood species database, undo/redo, and advanced search.

**Existing system gaps:** `.planning/TODOS.md` catalogs remaining items across incomplete implementations, missing error handling, and test coverage gaps.

**Known tech debt from v1.0:**
- EventBus infrastructure wired but unused (6 event types defined, zero pub/sub calls in production) — ready for callback-to-event migration
- Phase 01.2 (ConnectionPool) missing formal VERIFICATION.md
- 7 human verification items for import UX (visual/UX testing recommended)

## Constraints

- **Tech stack**: C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3, CMake 3.20+ — per rulebook
- **Licensing**: Permissive only (MIT, zlib, BSD, Public Domain)
- **File limits**: .cpp max 800 lines, .h max 400 lines, one class per header
- **Deployment**: 8-25 MB binary, cross-platform (Linux, Windows, macOS)
- **Performance**: Handle 600MB+ models, 8GB minimum RAM target
- **No migrations**: Delete and recreate DB/config on schema change (no user base)
- **Versioning**: Git commit hash only, no semantic versioning
- **Core independence**: Core layer has zero dependencies on UI or rendering

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| SDL2 + ImGui over Qt | Smaller footprint, permissive license, simpler build | ✓ Good |
| OpenGL 3.3 core profile | Wide hardware support, sufficient for 3D preview | ✓ Good |
| SQLite embedded | No server, single-file DB, zero config | ✓ Good |
| FetchContent for deps | No external package manager, reproducible builds | ✓ Good |
| No schema migrations | No user base, source files are permanent record | ✓ Good |
| Object-centric workspace | Panels act on focused object, clear UX model | ✓ Good |
| Separate settings app | Config hot-reload without restart, decoupled UI | — Pending |
| makeself for Linux installer | Self-extracting, no package manager dependency | ✓ Good |
| Extract UIManager/FileIOManager/ConfigManager | 3 focused managers, Application as thin coordinator | ✓ Good |
| WAL mode + ConnectionPool (NOMUTEX) | Eliminates SQLITE_BUSY, app-level mutex for simplicity | ✓ Good |
| MainThreadQueue for worker→UI | Prevents ImGui threading violations, clear contract | ✓ Good |
| ThreadPool with lazy init | Zero overhead when imports unused, parallel workers on demand | ✓ Good |
| Per-frame import completion throttling | Prevents UI blocking during large batch imports | ✓ Good |
| G-code toolpath as extruded quads | Reuses existing mesh renderer, color-coded cutting vs rapid | ✓ Good |

---
*Last updated: 2026-02-10 after v1.0 milestone*
