# Digital Workshop

## What This Is

Digital Workshop is a cross-platform C++17 desktop application for CNC woodworkers — combining 3D model library management, G-code analysis, and 2D cut optimization in a single workspace. Built with SDL2, Dear ImGui (docking), OpenGL 3.3, and SQLite3. Currently at v0.1.1 with core systems working but incomplete.

## Core Value

A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

## Requirements

### Validated

<!-- Shipped and working in v0.1.1 -->

- ✓ Import STL, OBJ, 3MF models with background processing — existing
- ✓ Content-based hash deduplication on import — existing
- ✓ Automatic thumbnail generation (offscreen OpenGL render) — existing
- ✓ Model library with grid/list view, search, tags — existing
- ✓ 3D viewport with orbit camera, ViewCube, lighting — existing
- ✓ G-code parsing (G0/G1/G2/G3, feed rates, tool changes) — existing
- ✓ 2D G-code path visualization — existing
- ✓ G-code timing and statistics — existing
- ✓ 2D cut optimization (bin-pack + guillotine algorithms) — existing
- ✓ Project create/open/save as .dwp archive — existing
- ✓ Project-model associations — existing
- ✓ Cost estimation database (CostRepository) — existing
- ✓ Dockable ImGui panel layout with persistence — existing
- ✓ Dark/light/high-contrast theme system — existing
- ✓ Separate settings application with hot-reload — existing
- ✓ Configurable input bindings — existing
- ✓ Cross-platform build (Linux + Windows CI) — existing
- ✓ Self-extracting Linux installer (makeself) — existing
- ✓ Windows NSIS installer — existing
- ✓ GitHub Actions CI/CD with auto-release — existing
- ✓ Model export (STL binary/ASCII, OBJ) — existing

### Active

<!-- Current scope — gaps in existing systems + new features for this milestone -->

(To be defined from TODOS.md gap analysis + feature priorities)

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

**Brownfield project:** Rewrite of an older Qt-based Digital Workshop. The old codebase had ~310 source files, 14 repositories, ~30 UI widgets. The new codebase has ~116 source files, 3 repositories, 7 panels. Many features from the old version haven't been ported yet.

**Feature gap reference:** `OBSERVATION_REPORT.md` documents 17 categories of gaps between old and new versions. Major missing subsystems include CNC machine control, cost/quoting workflow, tool database, wood species database, undo/redo, and advanced search.

**Existing system gaps:** `.planning/TODOS.md` catalogs 58 items across bugs, dead code, incomplete implementations, missing error handling, tech debt, and test coverage gaps in the current working systems.

**Architecture:** Layered N-tier — UI (ImGui) → Managers → Core (domain) → Data (SQLite). Workspace pattern centers on focused object (mesh, G-code, or cut plan). Repository pattern for data access. Factory pattern for format loading. Background import pipeline.

**Tech debt highlights:**
- Application.cpp is a 1,071-line god class
- Duplicate theme code paths
- Single SQLite connection (not thread-safe)
- Import pipeline has race condition on progress reporting
- Icon font system defined but never loaded
- CostPanel header exists with no implementation

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

---
*Last updated: 2026-02-08 after initialization*
