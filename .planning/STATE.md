# State: Sender Feature Parity

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-26)

**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

**Current Focus:** Sender Feature Parity milestone -- bring CNC sender to feature parity with dedicated senders, prioritizing safety, core controls, workflow polish, and extended capabilities.

## Current Position

**Phase:** Not started (defining requirements)
**Plan:** —
**Status:** Defining requirements

```
Phase Progress: [        ] 0/4
```

## Performance Metrics

| Metric | Value |
|--------|-------|
| Plans completed | 0 |
| Plans failed | 0 |
| Requirements done | 0/42 |
| Phases done | 0/4 |

## Accumulated Context

### Key Decisions
- Phase structure follows priority order: Safety → Core Sender → Niceties → Extended
- All safety features must be individually configurable via Config toggles
- UI-only changes to existing panels — no new windows or layout restructuring
- ncSender used as feature benchmark (170+ releases of dedicated sender polish)
- Branch: Sender-dev

### Known Issues
- Previous milestone ROADMAP.md shows phases 6-8 plans as "Pending" but code is actually shipped
- REQUIREMENTS.md traceability shows CUI-04 through CUI-10 as "Pending" but features are implemented

### Patterns Established
- C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3
- MainThreadQueue for UI updates from background threads
- EventBus for decoupled component communication
- Character-counting streaming for GRBL protocol
- CNC panels wired via CncCallbacks in application_wiring.cpp
- Config singleton for settings persistence (INI format)
- MacroManager uses own SQLite DB

### TODOs
- (none yet)

### Blockers
- (none)

## Session Continuity

**Last session:** Milestone initialization (Sender Feature Parity)
**Next action:** Define requirements, create roadmap, begin Phase 9 execution
**Context to carry:** Full ncSender feature comparison completed. 4 phases scoped. Branch Sender-dev created and pushed.

---
*State initialized: 2026-02-26*
*Last updated: 2026-02-26*
