# Digital Workshop

## What This Is

A cross-platform C++17 desktop application for CNC woodworkers that combines 3D model management, G-code analysis, material tracking, and CNC machine control in a single unified workspace. Built with SDL2 + Dear ImGui + OpenGL 3.3 + SQLite3. Currently at v0.2.0 with no external userbase.

## Core Value

A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds — all without leaving the application.

## Current Milestone: v0.3.0 Direct Carve

**Goal:** Stream 2.5D carving toolpaths directly from STL models to the CNC machine — no G-code files, no external CAM software — using a guided workflow with automatic tool recommendation and surgical clearing.

**Target features:**
- Rasterize STL model to 2D heightmap via ray-mesh intersection
- Analyze heightmap for minimum feature radius and island regions
- Automatically recommend tools from .vtdb database based on geometry
- Generate raster scan toolpath with configurable axis, stepover, milling direction
- Surgical clearing for island regions only (not entire surface)
- Step-by-step wizard: machine check → model fit → tool select → preview → outline test → commit → run
- Stream toolpath point-by-point via existing CncController protocol
- Optional G-code file export for future replay

**Branch:** feature/direct-carve

## Requirements

### Validated

- ✓ 3D model import (STL, OBJ, 3MF) with async pipeline and thumbnail generation — Bootstrap
- ✓ G-code import with toolpath-to-mesh conversion and metadata extraction — Import Pipeline
- ✓ Content-addressable storage for imported files — Library Storage
- ✓ SQLite database with FTS5 search, categories, and graph relationships — Library Storage
- ✓ Materials system with 31 built-in species, .dwmat archives, Janka hardness — Materials System
- ✓ AI material generation via Gemini API — Materials System
- ✓ Material-to-model assignment with texture mapping — Materials System
- ✓ .dwproj export/import with materials and thumbnails — Library Storage
- ✓ EventBus, ConnectionPool, MainThreadQueue architecture — Core Architecture
- ✓ Application decomposition (UIManager, FileIOManager, ConfigManager) — Core Architecture
- ✓ Vectric-compatible tool database (.vtdb format, bidirectional) — CNC Tooling
- ✓ Feeds & speeds calculator with rigidity derating and power limiting — CNC Tooling
- ✓ GRBL-compatible CNC controller with character-counting streaming — CNC Controller Suite
- ✓ Serial port communication (cross-platform POSIX wrapper) — CNC Controller Suite
- ✓ Machine profiles with GRBL $ settings mapping — CNC Controller Suite
- ✓ Tool Browser panel with tree view, detail view, calculator — CNC Controller Suite
- ✓ Universal context menu subsystem — UI Polish
- ✓ Cross-platform CI (Linux, Windows, macOS) with packaging — Bootstrap
- ✓ CNC status display (DRO, machine state, feed/spindle) — CNC Controller Suite
- ✓ Manual control (jog, homing, WCS, MDI console) — CNC Controller Suite
- ✓ Tool + material integration with feeds/speeds — CNC Controller Suite
- ✓ Job streaming with progress, time estimation, feed deviation — CNC Controller Suite
- ✓ Job safety (pause/resume/abort, sensor display, pre-flight, draw outline) — CNC Controller Suite
- ✓ Firmware settings (GRBL $$ editor, backup/restore, NC/NO toggles) — CNC Controller Suite
- ✓ Macro system (SQLite storage, built-in macros, keyboard shortcuts) — CNC Controller Suite
- ✓ Configurable safety guards (long-press, dead-man, door interlock, soft limits) — Sender Feature Parity
- ✓ Override controls (spindle %, rapid %, feed %), coolant toggles, alarm handling — Sender Feature Parity
- ✓ Extended jog (0.01mm, diagonals, move-to), DRO click-to-zero, recent files — Sender Feature Parity
- ✓ Probe workflows (Z, TLS, 3D), M6 detection, nested macros, gamepad input — Sender Feature Parity
- ✓ CNC job history with database persistence and crash recovery — Sender Feature Parity

### Active

- [ ] STL-to-heightmap rasterization with configurable resolution
- [ ] Heightmap analysis: minimum feature radius, island detection
- [ ] Automatic tool recommendation from .vtdb database
- [ ] Raster scan toolpath with surgical island clearing
- [ ] Guided wizard workflow with safety checks
- [ ] Point-by-point CNC streaming with pause/resume/abort

### Out of Scope

- Full 3D CAM (3+2 axis, undercuts, rest machining) — Direct Carve is 2.5D top-down only
- Rest machining optimization — future milestone, requires full CAM foundation
- Mobile companion app — desktop-first
- Cloud sync — local-only for now
- Multi-machine control — single machine at a time
- Remote/headless server mode — desktop CNC machines are physically attended
- Gamepad/joystick jog — shipped in v0.2.0
- Full G-code editor — MDI console covers single commands; full editing is a separate tool
- Plugin/extensibility system — premature for current userbase

## Context

The CNC sender reached feature parity with dedicated senders in v0.2.0 (ncSender benchmark). Job history with database persistence and crash recovery was added post-milestone. TCP/IP transport was added in v0.2.2. Live CNC tool position overlay, work envelope wireframe, and DRO readout are available in both viewports.

Direct Carve is a 2.5D-only approach: pure top-down heightmap from STL, no undercuts. V-bit is the primary finishing tool (taper geometry eliminates roughing). Ball nose/TBN is secondary when minimum feature radius analysis indicates it. Surgical clearing targets only island regions (enclosed depressions), not the entire surface. The system uses the existing Vectric .vtdb tool database for tool geometry, feeds, and speeds.

## Constraints

- **Tech stack**: C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3 — established, no changes
- **Serial protocol**: GRBL character-counting streaming — already implemented
- **UI scope**: Modifications to existing panels only — no new windows or layout changes
- **Compatibility**: Must maintain Vectric .vtdb bidirectional compatibility
- **Platform**: Linux primary, Windows/macOS CI must pass
- **Threading**: MainThreadQueue for UI updates, ThreadPool for background work — established patterns
- **Safety**: All safety features must be individually configurable (enable/disable per feature)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Vectric .vtdb compatibility | Industry standard, users can share tool libraries | ✓ Good |
| GRBL as primary protocol | Most common hobbyist CNC firmware | ✓ Good |
| Janka-based material classification | Physically meaningful for wood cutting | ✓ Good |
| Conservative default feeds/speeds | Safety-first for hobbyist users | ✓ Good |
| All safeties configurable | Different machines have different safety needs | — Pending |
| UI-only changes, no layout restructuring | Minimize disruption, focus on feature density | — Pending |
| ncSender as feature benchmark | Mature, dedicated sender with 170+ releases | — Pending |

| FluidNC settings use YAML paths not numeric IDs | FluidNC is increasingly popular, distinct protocol from GRBL | — Deferred |
| 2.5D top-down only for Direct Carve | Simplest viable approach, covers majority of hobby CNC use | — Pending |
| V-bit primary, ball nose secondary | V-bit taper eliminates roughing; ball nose only for small-radius features | — Pending |
| Surgical island clearing only | Avoid full-surface clearing when only specific regions need it | — Pending |
| Guided wizard workflow | Safety-first approach prevents common operator mistakes | — Pending |

---
*Last updated: 2026-02-28 after Direct Carve milestone start*
