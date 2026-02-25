# State: CNC Controller Suite

## Project Reference

**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

**Current Focus:** CNC Controller Suite milestone -- integrate existing subsystems (tool database, feeds/speeds calculator, material system, GRBL controller) into a usable CNC workflow.

## Current Position

**Phase:** 6 of 8 - Job Safety
**Plan:** Not yet planned
**Status:** Ready to plan

```
Phase Progress: [#####...] 5/8
```

## Performance Metrics

| Metric | Value |
|--------|-------|
| Plans completed | 14 |
| Plans failed | 0 |
| Requirements done | 22/39 |
| Phases done | 5/8 |

## Accumulated Context

### Key Decisions
- Phase structure follows natural requirement categories with CUI split into Status Display + Manual Control, TAC split into Tool Integration + Job Streaming, and FWC split into Firmware Settings + Macros
- Phase 7 (Firmware Settings) depends on Phase 1 only, not on Phases 2-6, allowing parallel work if desired
- Platform requirements (Windows serial, WebSocket) deferred to v2

### Known Issues
- Thread safety violation in real-time command dispatch (FND-01) -- must fix first
- Missing soft-reset-on-error in streaming engine (FND-02)
- No USB disconnect detection (FND-03)
- DTR suppression not implemented (FND-04)
- Arc resume edge case needs investigation during Phase 6

### Patterns Established
- C++17, SDL2, Dear ImGui (docking), OpenGL 3.3, SQLite3
- MainThreadQueue for UI updates from background threads
- EventBus for decoupled component communication
- Character-counting streaming for GRBL protocol
- Vectric .vtdb compatibility for tool database
- CNC panels wired via CncCallbacks in application_wiring.cpp
- Feed deviation uses override-aware comparison (accounts for feed override %)

### TODOs
- (none yet)

### Blockers
- (none)

## Session Continuity

**Last session:** Phase 5 execution (Job Streaming)
**Next action:** Plan Phase 6 (Job Safety)
**Context to carry:** Phase 5 complete -- CncJobPanel provides full job progress visibility (elapsed/remaining time, line counts, progress %, feed deviation warning). CncToolPanel exposes calculator results via getters. Feed deviation is override-aware.

---
*State initialized: 2026-02-24*
*Last updated: 2026-02-24*
