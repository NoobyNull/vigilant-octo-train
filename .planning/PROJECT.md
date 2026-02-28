# Digital Workshop

## What This Is

A cross-platform C++17 desktop application for CNC woodworkers that combines 3D model management, G-code analysis, material tracking, and CNC machine control in a single unified workspace. Built with SDL2 + Dear ImGui + OpenGL 3.3 + SQLite3. Currently at v0.2.0 with no external userbase.

## Core Value

A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds — all without leaving the application.

## Current Milestone: v0.2.1 FluidNC Config Editor

**Goal:** View and edit FluidNC YAML configuration over serial from within the existing CNC Settings panel — detect FluidNC firmware, parse `$S` output into a tree, edit settings, and persist to flash.

**Target features:**
- Detect FluidNC firmware and show FluidNC-specific settings tab
- Parse `$S` / `$SC` output (`$path=value` format) into hierarchical tree
- Display settings as collapsible tree with inline editing
- Send `$/path/to/setting=value` to change settings at runtime
- Persist changes to flash via `$CD=config.yaml`
- Show changed-vs-default highlighting via `$SC`

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

- [ ] FluidNC firmware detection and settings protocol (`$S`, `$SC`, `$/path=value`)
- [ ] FluidNC settings tree view with collapsible hierarchy and inline editing
- [ ] Persist FluidNC config changes to flash via `$CD=config.yaml`

### Out of Scope

- Toolpath generation / CAM — future milestone, not current focus
- Rest machining optimization — future milestone, requires CAM foundation
- Mobile companion app — desktop-first
- Cloud sync — local-only for now
- Multi-machine control — single machine at a time
- Remote/headless server mode — desktop CNC machines are physically attended
- Gamepad/joystick jog — shipped in v0.2.0
- Full G-code editor — MDI console covers single commands; full editing is a separate tool
- Plugin/extensibility system — premature for current userbase

## Context

The CNC sender reached feature parity with dedicated senders in v0.2.0 (ncSender benchmark). Job history with database persistence and crash recovery was added post-milestone. The settings panel currently speaks GRBL `$N=value` numeric protocol only.

FluidNC uses a different settings protocol: YAML-based paths instead of numeric IDs. `$S` dumps all settings as `$path=value` lines, `$SC` shows changed-from-default, and `$/path=value` changes a setting in RAM. `$CD=config.yaml` persists to flash. FluidNC firmware is already detected in `cnc_controller.cpp` via banner parsing.

The existing CncSettingsPanel already has tabs (Settings, Tuning, Safety, Raw) and raw line parsing infrastructure via `onRawLine()`. FluidNC settings will integrate as an additional tab or replacement behavior when FluidNC is detected.

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

| FluidNC settings use YAML paths not numeric IDs | FluidNC is increasingly popular, distinct protocol from GRBL | — Pending |

---
*Last updated: 2026-02-27 after FluidNC Config Editor milestone start*
