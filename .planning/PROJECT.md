# Digital Workshop

## What This Is

A cross-platform C++17 desktop application for CNC woodworkers that combines 3D model management, G-code analysis, material tracking, and CNC machine control in a single unified workspace. Built with SDL2 + Dear ImGui + OpenGL 3.3 + SQLite3. Currently at v0.2.0 with no external userbase.

## Core Value

A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds — all without leaving the application.

## Current Milestone: v0.2.0 Sender Feature Parity

**Goal:** Bring the CNC sender subsystem to feature parity with dedicated senders (ncSender benchmark), prioritizing safety, core sender controls, workflow polish, and extended capabilities.

**Target features:**
- Configurable safety features (long-press guards, dead-man watchdog, door interlock, soft limits, pause-before-reset)
- Full override controls (spindle, rapid, feed) with keyboard shortcuts
- Coolant toggles, alarm handling with codes + unlock, status polling config
- Finer jog control (0.01mm, per-step feedrates, diagonals, move-to dialog)
- Workflow niceties (DRO click-to-zero, recent files, job completion alerts, console source tags)
- Extended features (error/alarm references, probe workflow, M6 detection, nested macros, tool color viz)

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

### Active

- [ ] Configurable safety system with per-feature toggles
- [ ] Full override controls (spindle %, rapid %, enhanced feed %)
- [ ] Coolant control, alarm code handling, status polling configuration
- [ ] Extended jog capabilities (0.01mm, per-step feedrates, diagonals, move-to)
- [ ] Workflow niceties (DRO click-to-zero, recent files, job alerts, source tags)
- [ ] Extended sender features (alarm/error references, probe workflow, M6 detection, nested macros)

### Out of Scope

- Toolpath generation / CAM — future milestone, not current focus
- Rest machining optimization — future milestone, requires CAM foundation
- Mobile companion app — desktop-first
- Cloud sync — local-only for now
- Multi-machine control — single machine at a time
- Remote/headless server mode — desktop CNC machines are physically attended
- Gamepad/joystick jog — deferred, keyboard covers the need
- Full G-code editor — MDI console covers single commands; full editing is a separate tool
- Plugin/extensibility system — premature for current userbase

## Context

The CNC Controller Suite milestone shipped all 8 phases: foundation fixes, status display, manual control, tool integration, job streaming, job safety, firmware settings, and macros. The sender is functional but lacks feature parity with dedicated senders like ncSender.

Key gaps identified through ncSender comparison: no spindle/rapid override controls, no coolant toggles, no alarm code reference, no configurable safety guards (long-press, dead-man), limited jog options (no 0.01mm, no diagonals), no M6 tool change detection, no probe workflow, basic console without source tagging.

All changes in this milestone are UI-only modifications to existing panels — no layout changes, no new panel windows.

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

---
*Last updated: 2026-02-26 after Sender Feature Parity milestone start*
