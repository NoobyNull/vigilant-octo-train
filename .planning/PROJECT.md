# Digital Workshop

## What This Is

Digital Workshop is a cross-platform C++17 desktop application for CNC woodworkers — combining 3D model library management, G-code analysis with toolpath visualization, and 2D cut optimization in a single workspace. Built with SDL2, Dear ImGui (docking), OpenGL 3.3, and SQLite3. Architecture decomposed into focused managers (UIManager, FileIOManager, ConfigManager) with thread-safe infrastructure (EventBus, ConnectionPool, MainThreadQueue).

## Core Value

A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

## Requirements

### Validated

<!-- Shipped and working in v1.0 + Post-v1.0 -->


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
- ✓ Materials system with 32 built-in wood species — Post-v1.0
- ✓ Material texture mapping with planar UV projection — Post-v1.0
- ✓ Gemini AI material generation (optional, API key required) — Post-v1.0
- ✓ MaterialsPanel with category tabs, thumbnail grid, edit modal — Post-v1.0
- ✓ .dwmat archive import/export for material sharing — Post-v1.0
- ✓ Multi-select in library panel with batch operations — Post-v1.0
- ✓ Context menu system (ContextMenuManager) — Post-v1.0
- ✓ Batch thumbnail regeneration with progress dialog — Post-v1.0
- ✓ Modern Inter font with DPI-aware scaling — v1.2
- ✓ Fully hand-tuned dark/light/high-contrast themes — v1.2
- ✓ Content-addressable storage with hash-based blob directories — v1.1
- ✓ Smart file handling (auto-detect local vs NAS) — v1.1
- ✓ Category/genus hierarchy for model organization — v1.1
- ✓ FTS5 full-text search across library metadata — v1.1
- ✓ GraphQLite SQLite extension for Cypher graph queries — v1.1
- ✓ Project export as portable .dwproj ZIP archives — v1.1

## Current State: v1.3 shipped

**Latest milestone:** v1.3 AI Material UX & Fixes (2026-02-21)
- AI-first Add Material dialog with unified generate + manual entry
- Gemini API fixes for texture generation
- AI descriptor integration with category classification

**Previous milestones:** v1.0 (Foundation), v1.1 (Library Storage), v1.2 (UI Freshen-Up)

### Out of Scope (standing)

- wxWidgets migration — too deep an ImGui integration (~1,637 calls across 31 files)
- Cloud-based operation — fully offline desktop app
- Custom window chrome / frameless window — OS-native title bar is fine

## Context

**Current state:** v1.3 complete. AI-first material creation, descriptor integration, Gemini fixes all shipped on top of v1.2 UI polish and v1.1 library storage. 545 tests passing. Architecture remains clean — Application as thin coordinator, 3 focused managers.

**Tech stack:** C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3, GraphQLite, CMake 3.20+

**v1.2 UI context:** Current ImGui theme has rounding and custom colors but uses the default ProggyClean font — a monospace bitmap font that reads "programmer tool." Font loading exists in two places (application.cpp:144 and ui_manager.cpp:52) using `AddFontDefault()`. FontAwesome 6 icons are merged in. Theme system has dark/light/high-contrast variants but the light theme is barely customized (delegates to `StyleColorsLight`). DPI scaling exists via `setFontScale()` but isn't driven by platform DPI detection.

**Feature gap reference:** `OBSERVATION_REPORT.md` documents 17 categories of gaps between old and new versions. Major missing subsystems include CNC machine control, cost/quoting workflow, tool database, wood species database, undo/redo, and advanced search.

**Existing system gaps:** `.planning/TODOS.md` catalogs remaining items across incomplete implementations, missing error handling, and test coverage gaps.

**Known tech debt from v1.0:**
- EventBus infrastructure wired but unused (6 event types defined, zero pub/sub calls in production) — ready for callback-to-event migration
- Phase 01.2 (ConnectionPool) missing formal VERIFICATION.md
- 7 human verification items for import UX (visual/UX testing recommended)

## Constraints

- **Tech stack**: C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3, GraphQLite, CMake 3.20+ — per rulebook
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
| Content-addressable storage | Hash-based file store decouples physical location from logical org | ✓ Good |
| GraphQLite over Kuzu | Kuzu abandoned Oct 2025; GraphQLite is a SQLite extension — single DB, Cypher queries, MIT license | ✓ Good |
| Auto-detect copy vs move on import | Copy from NAS, move if local same filesystem — user picks strategy not mechanics | ✓ Good |
| Inter font over ProggyClean | Modern proportional font replaces monospace bitmap — biggest single visual improvement | ✓ Good |
| Stay on ImGui over wxWidgets | 1,637 ImGui calls across 31 files; wxWidgets would be a full rewrite for marginal gain | ✓ Good |

---
*Last updated: 2026-02-21 after v1.3 milestone completion*
