# Digital Workshop

## What This Is

A cross-platform C++17 desktop application for CNC woodworkers that combines 3D model management, G-code analysis, material tracking, and CNC machine control in a single unified workspace. Built with SDL2 + Dear ImGui + OpenGL 3.3 + SQLite3. Currently at v0.1.5 with no external userbase.

## Core Value

A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds — all without leaving the application.

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
- ✓ GRBL-compatible CNC controller with character-counting streaming — CNC Tooling
- ✓ Serial port communication (cross-platform POSIX wrapper) — CNC Tooling
- ✓ Machine profiles with GRBL $ settings mapping — CNC Tooling
- ✓ Tool Browser panel with tree view, detail view, calculator — CNC Tooling
- ✓ Universal context menu subsystem — UI Polish
- ✓ Cross-platform CI (Linux, Windows, macOS) with packaging — Bootstrap

### Active

- [ ] Tool database → CNC controller integration (select tool, auto-populate parameters)
- [ ] Wood species drives feeds/speeds (material hardness → calculator → controller)
- [ ] Real-time CNC status display (XYZ position, spindle RPM, feed rate)
- [ ] Job progress tracking (current line, remaining lines, elapsed/remaining time)
- [ ] Pause, resume, E-stop controls
- [ ] Safe resume after pause or power loss (remember position + line)
- [ ] Firmware settings modification (GRBL $$, FluidNC YAML)
- [ ] Machine tuning interface (steps/mm, acceleration, jerk)
- [ ] Sensor monitoring (endstops, probe, door)
- [ ] Macro system (homing, tool change, probe, custom G-code sequences)
- [ ] CNC controller real-time feedback loop (status polling → UI update)

### Out of Scope

- Toolpath generation / CAM — future milestone, not current focus
- Rest machining optimization — future milestone, requires CAM foundation
- Mobile companion app — desktop-first
- Cloud sync — local-only for now
- Multi-machine control — single machine at a time

## Context

The application already has a working GRBL controller with character-counting streaming, serial port communication, and basic status polling. The tool database is Vectric-compatible (.vtdb) with full CRUD. The feeds & speeds calculator classifies materials by Janka hardness and applies rigidity derating. Machine profiles map to GRBL $ settings.

The gap is integration: these subsystems exist in isolation. Tools aren't connected to the controller. Material selection doesn't flow into feed calculations. The controller UI lacks real-time feedback, job management, and safety features. This milestone bridges the gap from "parts exist" to "usable CNC workflow."

Existing firmware research covers Smoothieware config keys and FluidNC YAML paths mapping to GRBL $ settings.

## Constraints

- **Tech stack**: C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3 — established, no changes
- **Serial protocol**: GRBL character-counting streaming — already implemented
- **Compatibility**: Must maintain Vectric .vtdb bidirectional compatibility
- **Platform**: Linux primary, Windows/macOS CI must pass
- **Threading**: MainThreadQueue for UI updates, ThreadPool for background work — established patterns

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Vectric .vtdb compatibility | Industry standard, users can share tool libraries | ✓ Good |
| GRBL as primary protocol | Most common hobbyist CNC firmware | ✓ Good |
| Janka-based material classification | Physically meaningful for wood cutting | ✓ Good |
| Conservative default feeds/speeds | Safety-first for hobbyist users | ✓ Good |
| Named milestones (not version-numbered) | Clearer intent, version is just 0.1.x | — Pending |

---
*Last updated: 2026-02-24 after initialization*
