# Project State: Digital Workshop

**Last Updated:** 2026-02-09
**Current Session:** Phase 1.3 MainThreadQueue - Plan 02 Complete (Sub-phase Complete)

---

## Project Reference

**Core Value:**
A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

**Current Focus:**
Phase 1: Architectural Foundation & Thread Safety — Decompose the Application god class, establish thread-safe infrastructure, and fix existing bugs/debt.

---

## Current Position

**Active Phase:** Phase 1 — Architectural Foundation & Thread Safety

**Current Sub-Phase:** 1.3 MainThreadQueue (Plan 2/2 complete) ✓

**Status:** Complete - MainThreadQueue fully integrated with Application lifecycle

**Progress:**
```
Phase 1: [█████████░░░░░░░░░░░] 3/6 sub-phases (50%)

1.1 EventBus                 [██████████] Plan 2/2 complete ✓
1.2 ConnectionPool           [██████████] Plan 2/2 complete ✓
1.3 MainThreadQueue          [██████████] Plan 2/2 complete ✓
1.4 God Class Decomposition  [░░░░░░░░░░] Not Started
1.5 Bug Fixes                [░░░░░░░░░░] Not Started
1.6 Dead Code Cleanup        [░░░░░░░░░░] Not Started
```

---

## Performance Metrics

| Phase | Plan | Duration | Tasks | Files | Completed |
|-------|------|----------|-------|-------|-----------|
| 1.1   | 01   | 3m 55s   | 2     | 5     | 2026-02-08 |
| 1.1   | 02   | 1m 40s   | 1     | 2     | 2026-02-09 |
| 1.2   | 01   | 2m 21s   | 2     | 6     | 2026-02-09 |
| 1.2   | 02   | 3m 44s   | 2     | 6     | 2026-02-09 |
| 1.3   | 01   | 2m 30s   | 2     | 5     | 2026-02-09 |
| 1.3   | 02   | 4m 10s   | 2     | 8     | 2026-02-09 |

**Cycle Time:** 3m 3s per plan (6 plans completed)

**Completion Rate:** 1 sub-phase / 1 day = 1 sub-phase/day

**Estimated Remaining:**
- 1.1 EventBus: 3-5 days (M)
- 1.2 ConnectionPool: 5-10 days (L)
- 1.3 MainThreadQueue: 3-5 days (M)
- 1.4 God Class Decomposition: 5-10 days (L)
- 1.5 Bug Fixes: 3-5 days (M)
- 1.6 Dead Code Cleanup: 1-3 days (S)

**Total Estimate:** 20-38 days (4-8 weeks)

---

## Accumulated Context

### Key Decisions Made

1. **Roadmap Structure (2026-02-08)**
   - Decision: 6 sub-phases for Phase 1
   - Rationale: Standard depth (5-8), natural dependency boundaries
   - Impact: Clear execution path with low-risk foundation first

2. **Sub-Phase Ordering (2026-02-08)**
   - Decision: EventBus → ConnectionPool → MainThreadQueue → God Class Decomposition → Bug Fixes → Cleanup
   - Rationale: Infrastructure before refactoring; low-risk before high-risk
   - Impact: Safer decomposition with support systems in place

3. **Application Decomposition Approach (2026-02-08)**
   - Decision: Extract UIManager and FileIOManager from Application
   - Rationale: Two focused managers cover ~60% of Application's responsibilities
   - Target: Application.cpp 1,071 → ~300 lines
   - Impact: Maintainability, testability, future extensibility

4. **Threading Model (2026-02-08)**
   - Decision: MainThreadQueue for worker-to-UI communication, EventBus main-thread-only
   - Rationale: Prevents ImGui threading violations (research Pitfall 3)
   - Impact: Safe background processing, clear threading contracts

5. **SQLite Strategy (2026-02-08)**
   - Decision: WAL mode + ConnectionPool (2-4 connections)
   - Rationale: Eliminates SQLITE_BUSY, supports concurrent read/write
   - Impact: ImportQueue can run concurrently with UI queries

6. **EventBus Threading Contract (2026-02-08, Plan 1.1-01)**
   - Decision: Main-thread-only, no internal synchronization
   - Rationale: Matches ImGui threading model, avoids mutex overhead
   - Impact: EventBus must only be used from main thread

7. **Subscription Lifetime Management (2026-02-08, Plan 1.1-01)**
   - Decision: shared_ptr<void> subscription tokens control handler lifetime
   - Rationale: RAII pattern, cleaner than explicit unsubscribe()
   - Impact: Handlers automatically cleaned up when token destroyed

8. **Event Handler Reentrancy Safety (2026-02-08, Plan 1.1-01)**
   - Decision: Lock all handlers upfront before iteration
   - Rationale: Prevents iterator invalidation, ensures handlers alive at publish start remain valid
   - Impact: Safe to subscribe/unsubscribe during event dispatch

9. **Exception Isolation in EventBus (2026-02-08, Plan 1.1-01)**
   - Decision: Wrap each handler invocation in try/catch
   - Rationale: One misbehaving subsystem should not break others
   - Impact: Exceptions logged but don't prevent other handlers from running

10. **ConnectionPool NOMUTEX Strategy (2026-02-09, Plan 1.2-01)**
   - Decision: Use NOMUTEX flag with application-level mutex in ConnectionPool
   - Rationale: Avoids SQLite's internal locking overhead, simpler concurrency reasoning
   - Impact: Better performance, ConnectionPool's mutex provides thread safety

11. **Busy Timeout Configuration (2026-02-09, Plan 1.2-01)**
   - Decision: Set sqlite3_busy_timeout to 5000ms for pooled connections
   - Rationale: Prevents immediate SQLITE_BUSY on contention, allows reasonable wait
   - Impact: More robust multi-threaded behavior without blocking indefinitely

12. **Synchronous=NORMAL with WAL (2026-02-09, Plan 1.2-01)**
   - Decision: PRAGMA synchronous=NORMAL after enabling WAL mode
   - Rationale: WAL + NORMAL provides good durability/performance balance
   - Impact: Faster writes while maintaining crash safety

13. **Pool Exhaustion Behavior (2026-02-09, Plan 1.2-01)**
   - Decision: Throw runtime_error on exhaustion rather than blocking
   - Rationale: Fail-fast makes pool sizing issues visible and debuggable
   - Impact: Callers must handle exhaustion or size pool appropriately

14. **MainThreadQueue Bounded Size (2026-02-09, Plan 1.3-01)**
   - Decision: Default max size 1000, configurable per instance
   - Rationale: 1000 messages at 60fps = 16 seconds backlog, prevents memory exhaustion
   - Impact: Provides backpressure, blocks producers if queue full

15. **Condition Variable Over Lock-Free Queue (2026-02-09, Plan 1.3-01)**
   - Decision: Use std::mutex + std::condition_variable instead of lock-free
   - Rationale: Lock-free adds complexity, not needed for ~60fps message rate
   - Impact: Standard, debuggable, portable implementation

16. **Execute Callbacks Outside Lock (2026-02-09, Plan 1.3-01)**
   - Decision: Drain queue to local vector under lock, execute callbacks outside lock
   - Rationale: Prevents deadlocks if callbacks enqueue or acquire other locks
   - Impact: Safer concurrency, minimizes lock hold time

### Open Questions

1. **File Dialog Implementation (DEAD-04)**
   - Question: Implement native file dialogs or remove stubs?
   - Context: UIManager has stub methods that always return false
   - Decision point: Sub-phase 1.4 (after FileIOManager extraction)
   - Options: Platform-specific native dialog, ImGui file browser, or remove if unused

2. **Icon Font System (DEAD-05)**
   - Question: Load FontAwesome icons or remove icon constants?
   - Context: 93 icon constants defined but font never loaded
   - Decision point: Sub-phase 1.6 (dead code cleanup)
   - Options: Load FontAwesome TTF and use icons, or remove if no UI code references them

3. **Manager Namespace Organization**
   - Question: Where to place new managers (UIManager, FileIOManager)?
   - Context: Existing `src/ui/ui_manager.h` vs new `src/managers/`
   - Decision point: Sub-phase 1.4 (decomposition planning)
   - Options: Consolidate with existing, create new directory, or keep separate

### Active TODOs

*From ROADMAP.md — tracked per sub-phase*

**Next Immediate Actions:**
1. Execute plan 1.3-02 (MainThreadQueue integration into Application and ImportQueue)
2. Complete sub-phase 1.3 MainThreadQueue
3. Begin sub-phase 1.4 God Class Decomposition

### Current Blockers

**None** — Roadmap is complete and ready for execution.

---

## Session Continuity

### What Was Just Accomplished

**Session Goal:** Execute Phase 1.3 MainThreadQueue - Plan 02 (Integration and Documentation)

**Completed:**
- Plan 02: Integrated MainThreadQueue into Application lifecycle (init, update, shutdown)
- Plan 02: Added threading::initMainThread() call at Application startup
- Plan 02: MainThreadQueue::processAll() called every frame in update loop
- Plan 02: Added ASSERT_MAIN_THREAD() to EventBus::publish()
- Plan 02: Added ASSERT_MAIN_THREAD() to Application::renderPanels()
- Plan 02: Added ASSERT_MAIN_THREAD() to all 7 Workspace setters/clearers
- Plan 02: Created comprehensive threading contracts documentation (docs/THREADING.md)
- Plan 02: Updated EventBus tests to use fixture with initMainThread()
- All 410 tests pass, application builds successfully

**Artifacts Created:**
- `.planning/phases/01.3-mainthreadqueue/1.3-02-SUMMARY.md` — Plan 02 summary
- `docs/THREADING.md` — Threading contracts documentation (60 lines)

**Artifacts Modified:**
- `src/app/application.h` — Added MainThreadQueue forward decl, member, accessor
- `src/app/application.cpp` — Integration (init, update, shutdown), renderPanels assertion
- `src/core/events/event_bus.h` — Added ASSERT_MAIN_THREAD to publish()
- `src/ui/panels/panel.h` — Updated render() comment, added thread_utils include
- `src/app/workspace.cpp` — Added ASSERT_MAIN_THREAD to all setters/clearers
- `src/CMakeLists.txt` — Added main_thread_queue.cpp to build
- `tests/test_event_bus.cpp` — Added EventBusTest fixture

**Commits:**
- `1421af0` — feat(1.3-02): integrate MainThreadQueue into Application lifecycle
- `65115c5` — docs(1.3-02): create threading contracts documentation

### What to Do Next

**Immediate Next Step:**
```bash
/gsd:plan-phase 1.4
```

Sub-phase 1.3 (MainThreadQueue) is complete (2/2 plans done). Begin planning Phase 1.4 God Class Decomposition.

**Phase 1.4 Objectives:**
- Extract UIManager from Application
- Extract FileIOManager from Application
- Migrate ImportQueue to use MainThreadQueue for completed tasks
- Reduce Application.cpp from 1,071 lines to ~300 lines

**Context for Next Session:**
- EventBus foundation complete (2/2 plans done) ✓
- ConnectionPool complete (2/2 plans done) ✓
- MainThreadQueue complete (2/2 plans done) ✓
- Next: Phase 1.4 god class decomposition
- Infrastructure in place for safe threading and decoupled communication

---

## Project Health

**Code Quality:**
- Baseline: 1,071-line god class (Application.cpp)
- Target: ~300 lines after sub-phase 1.4
- Current: No refactoring yet (0% progress)

**Test Coverage:**
- Baseline: Core modules tested (loaders, database, mesh, optimizer)
- Infrastructure: EventBus (10 tests), ConnectionPool (10 tests), MainThreadQueue (10 tests) — all passing
- Target: 100% coverage of new infrastructure ✓

**Tech Debt:**
- Baseline: 58 items in TODOS.md
- Phase 1 Scope: 7 bugs + 6 dead code items + 1 god class = 14 items
- Target: 0 items from Phase 1 scope remaining

**Threading Safety:**
- Baseline: Single SQLite connection (not thread-safe), ImportProgress race condition
- Infrastructure: WAL mode enabled ✓, ConnectionPool (2 connections) ✓, MainThreadQueue ✓, ASSERT_MAIN_THREAD assertions ✓
- Documentation: docs/THREADING.md created with complete threading contracts ✓
- Target: All threading infrastructure complete ✓

---

## Quick Reference

**Project Root:** `/data/DW/`

**Key Files:**
- Roadmap: `.planning/ROADMAP.md`
- Requirements: `.planning/REQUIREMENTS.md`
- Project Definition: `.planning/PROJECT.md`
- Gap Analysis: `.planning/TODOS.md`
- Research: `.planning/research/SUMMARY.md`

**Commands:**
- Plan next sub-phase: `/gsd:plan-phase 1.1`
- Execute plan: `/gsd:execute-plan`
- Mark complete: `/gsd:complete-phase 1.1`
- Insert urgent work: `/gsd:insert-phase 1.1`

**Resources:**
- Architecture doc: `.planning/codebase/ARCHITECTURE.md`
- Code conventions: `.planning/codebase/CONVENTIONS.md`
- Threading contracts: `docs/THREADING.md` (to be created in 1.3)

---

*State initialized: 2026-02-08*
*Next update: After sub-phase 1.1 planning*
