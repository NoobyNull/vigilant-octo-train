# Project State: Digital Workshop

**Last Updated:** 2026-02-10
**Current Session:** Phase 2 Import Pipeline - COMPLETE (10/10 Plans)

---

## Project Reference

**Core Value:**
A single focused workspace where a CNC woodworker can manage their model library, analyze toolpaths, and optimize material usage — without switching between tools.

**Current Focus:**
Phase 2: Import Pipeline — Build G-code/mesh file import system with background processing, metadata extraction, and library management.

---

## Current Position

**Active Phase:** Phase 2 — Import Pipeline

**Current Plan:** 02-10 (Plan 10/10 complete - PHASE COMPLETE)

**Status:** Final gap closure plan complete. Import completion throttled to one per frame (non-blocking UI), viewport focus preserved during imports (user decision), thumbnail failures now reported via toast with retry logic. Phase 2 Import Pipeline is 100% complete. All 422 tests pass.

**Progress:**
[██████████] 100%

Phase 1: [████████████████████] 6/6 sub-phases (100%)
1.1 EventBus                 [##########] Plan 2/2 complete
1.2 ConnectionPool           [##########] Plan 2/2 complete
1.3 MainThreadQueue          [##########] Plan 2/2 complete
1.4 God Class Decomposition  [##########] Plan 3/3 complete
1.5 Bug Fixes                [##########] Plan 3/3 complete
1.6 Dead Code Cleanup        [##########] Plan 1/1 complete

Phase 2: [████████████████████] Plan 10/10 (100%) - COMPLETE
2.1 ThreadPool & Config      [##########] Plan 1/1 complete
2.2 G-code Schema & Repo     [##########] Plan 1/1 complete
2.3 G-code Loader & Handler  [##########] Plan 1/1 complete
2.4 Parallel Import Queue    [##########] Plan 1/1 complete
2.5 Background Import UI     [##########] Plan 1/1 complete
2.6 Import Pipeline Wiring   [##########] Plan 1/1 complete
2.7 Toolpath Visualization   [##########] Plan 1/1 complete
2.8 Pipeline Hardening       [##########] Plan 1/1 complete
2.9 Folder Drag-and-Drop     [##########] Plan 1/1 complete
2.10 Import Gap Closure      [##########] Plan 1/1 complete

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
| 1.4   | 01   | 5m 32s   | 1     | 5     | 2026-02-09 |
| 1.4   | 02   | 4m 52s   | 2     | 5     | 2026-02-09 |
| 1.4   | 03   | 5m 46s   | 2     | 5     | 2026-02-09 |
| 1.5   | 01   | 2m 8s    | 2     | 4     | 2026-02-09 |
| 1.5   | 02   | 2m 51s   | 1     | 2     | 2026-02-09 |
| 1.5   | 03   | 2m 45s   | 1     | 3     | 2026-02-09 |
| 1.6   | 01   | 1m 59s   | 2     | 2     | 2026-02-09 |
| 2.0   | 01   | 4m 57s   | 2     | 7     | 2026-02-10 |
| 2.0   | 02   | 7m 12s   | 2     | 6     | 2026-02-10 |
| 2.0   | 03   | 5m 2s    | 2     | 7     | 2026-02-10 |
| 2.0   | 04   | 5m 44s   | 2     | 7     | 2026-02-10 |
| 2.0   | 05   | 4m 54s   | 2     | 11    | 2026-02-10 |
| 2.0   | 06   | 6m 20s   | 2     | 6     | 2026-02-10 |
| 2.0   | 07   | 5m 16s   | 2     | 11    | 2026-02-10 |
| 2.0   | 08   | 42s      | 2     | 3     | 2026-02-10 |

**Cycle Time:** 3m 36s per plan (21 plans completed)

**Completion Rate:** 6 sub-phases in 2 days (Phase 1 complete)

**Phase 1 Complete:** All architectural foundation and thread safety work done

**Phase 2 Started:** Import Pipeline infrastructure - ThreadPool and Config foundations in place

---
| Phase 02 P05 | 294 | 2 tasks | 11 files |
| Phase 02 P07 | 316 | 2 tasks | 11 files |
| Phase 02 P09 | 106 | 1 tasks | 2 files |
| Phase 02 P10 | 95 | 2 tasks | 3 files |

## Accumulated Context

### Roadmap Evolution

- Phase 2 added: Import Pipeline

### Key Decisions Made

1. **Roadmap Structure (2026-02-08)**
   - Decision: 6 sub-phases for Phase 1
   - Rationale: Standard depth (5-8), natural dependency boundaries
   - Impact: Clear execution path with low-risk foundation first

2. **Sub-Phase Ordering (2026-02-08)**
   - Decision: EventBus -> ConnectionPool -> MainThreadQueue -> God Class Decomposition -> Bug Fixes -> Cleanup
   - Rationale: Infrastructure before refactoring; low-risk before high-risk
   - Impact: Safer decomposition with support systems in place

3. **Application Decomposition Approach (2026-02-08)**
   - Decision: Extract UIManager, FileIOManager, and ConfigManager from Application
   - Rationale: Three focused managers cover all of Application's non-coordinator responsibilities
   - Result: Application.cpp 1,108 -> 374 lines (66% reduction)
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

17. **UIManager Callback Injection Pattern (2026-02-09, Plan 1.4-01)**
    - Decision: Application injects action callbacks into UIManager via setOn* methods
    - Rationale: UIManager handles rendering/UI state, Application handles business logic
    - Impact: Clean separation, UIManager has no dependency on Application

18. **FileIOManager setShowStartPage Callback Pattern (2026-02-09, Plan 1.4-02)**
    - Decision: FileIOManager methods receive `std::function<void(bool)> setShowStartPage` callback
    - Rationale: Decouples FileIOManager from UIManager's visibility state
    - Impact: FileIOManager has no dependency on UIManager, only coordinates subsystems

19. **FileIOManager Panel Pointers Per-Call (2026-02-09, Plan 1.4-02)**
    - Decision: processCompletedImports receives panel pointers as parameters, not stored as members
    - Rationale: Prevents lifetime issues, keeps dependency minimal and explicit
    - Impact: FileIOManager only couples to panel types at method signature level

20. **ConfigManager Owns ConfigWatcher (2026-02-09, Plan 1.4-03)**
    - Decision: ConfigManager owns ConfigWatcher, handles all config-related operations
    - Rationale: Natural ownership -- config watching, applying, and state persistence are all config responsibilities
    - Impact: Application no longer owns ConfigWatcher or any config methods

21. **ConfigManager Quit Callback Injection (2026-02-09, Plan 1.4-03)**
    - Decision: ConfigManager::relaunchApp() calls m_quitCallback() instead of Application::quit() directly
    - Rationale: Keeps ConfigManager decoupled from Application
    - Impact: ConfigManager has no dependency on Application

22. **Initial Config Application in ConfigManager::init() (2026-02-09, Plan 1.4-03)**
    - Decision: Moved initial theme, render settings, log level application from Application::init() to ConfigManager::init()
    - Rationale: Eliminates duplication since applyConfig() already has this logic for hot-reload
    - Impact: Single point of config application, Application further simplified

23. **BUG-03 Verification Without Unit Tests (2026-02-09, Plan 1.5-03)**
    - Decision: Verify framebuffer move constructor via code review instead of unit test
    - Rationale: Framebuffer requires OpenGL context for testing - adds complexity and external dependencies
    - Impact: BUG-03 verified (framebuffer.cpp:21-22, :36-37 zero width/height correctly) but no test coverage

24. **Comprehensive Edge Case Coverage for Angle Wrapping (2026-02-09, Plan 1.5-03)**
    - Decision: Add tests for negative angles, exact boundaries (0, 360), and small negative wraps
    - Rationale: fmod behavior with negative angles is subtle - needs explicit test coverage to catch regressions
    - Impact: 4 new camera tests cover critical edge cases for BUG-02

25. **Realistic LIKE Injection Test Patterns (2026-02-09, Plan 1.5-03)**
    - Decision: Include tests with combined wildcards like "box_v2 (50%)"
    - Rationale: Real search terms often contain both underscores and percent signs
    - Impact: 8 new string utils tests verify escapeLike works in realistic scenarios for BUG-05

26. **ViewCube Cache Storage as Offsets (2026-02-09, Plan 1.5-02)**
    - Decision: Cache projected vertices as offsets from origin, not absolute screen coordinates
    - Rationale: Viewport can move/resize without invalidating cache - only camera orientation matters
    - Impact: Cache remains valid through window resize/move, fewer invalidations, better performance

27. **ViewCube Cache Invalidation Epsilon (2026-02-09, Plan 1.5-02)**
    - Decision: Use epsilon threshold 0.001 for camera orientation change detection
    - Rationale: Avoids cache misses from floating-point imprecision while detecting meaningful movement
    - Impact: Robust cache hit rate without false misses from float comparison

28. **ParallelismTier Calculation Strategy (2026-02-10, Plan 02-01)**
    - Decision: Auto=60%, Fixed=90%, Expert=100% of cores
    - Rationale: Auto leaves headroom for UI and system tasks, Fixed maximizes throughput with minimal reserve, Expert uses all cores for maximum parallelism
    - Impact: User can control import concurrency level based on workflow needs

29. **Library Path Validation Approach (2026-02-10, Plan 02-01)**
    - Decision: Inline validation with filesystem::exists and create_directories
    - Rationale: Immediate feedback without platform-specific dialog, validates both existing paths and creatable paths
    - Impact: User gets instant visual feedback on path validity without modal dialogs

30. **FileHandlingMode Enum vs Int (2026-02-10, Plan 02-01)**
    - Decision: Dedicated enum with explicit values (LeaveInPlace/CopyToLibrary/MoveToLibrary)
    - Rationale: Type safety, clear intent, easy to extend, matches existing Config patterns
    - Impact: Safer code, self-documenting API, harder to misuse

31. **G-code Metadata JSON Serialization (2026-02-10, Plan 02-02)**
    - Decision: Feed rates and tool numbers stored as JSON arrays following ModelRepository tags pattern
    - Rationale: Consistent with existing patterns, simple parsing, no external dependencies
    - Impact: Uniform JSON handling across repositories, easy to extend

32. **Operation Groups Hierarchy Design (2026-02-10, Plan 02-02)**
    - Decision: Operation groups use model_id foreign key for Model -> Groups -> G-code hierarchy
    - Rationale: Natural parent-child relationship, cascade deletes handle cleanup, supports multiple groups per model
    - Impact: Clean hierarchy management, safe deletion, flexible organization

33. **G-code Template Storage (2026-02-10, Plan 02-02)**
    - Decision: Templates stored as JSON arrays of group names, applied by creating groups in order
    - Rationale: Simple data structure, easy to add new templates, preserves order
    - Impact: Extensible template system, CNC Router Basic seeded, users can define custom templates later

### Open Questions

1. **File Dialog Implementation (DEAD-04)**
   - Question: Implement native file dialogs or remove stubs?
   - Context: UIManager has stub methods that always return false
   - Decision point: Sub-phase 1.5 or 1.6
   - Options: Platform-specific native dialog, ImGui file browser, or remove if unused

2. **Icon Font System (DEAD-05)**
   - Question: Load FontAwesome icons or remove icon constants?
   - Context: 93 icon constants defined but font never loaded
   - Decision point: Sub-phase 1.6 (dead code cleanup)
   - Options: Load FontAwesome TTF and use icons, or remove if no UI code references them

3. **Manager Namespace Organization** (RESOLVED)
   - Decision: New `src/managers/` directory for extracted managers
   - Context: Old `src/ui/ui_manager.h` preserved untouched; new UIManager in `src/managers/`
   - Resolution: Separate directory, old .cpp excluded from build (Phase 1.6 cleanup)

### Active TODOs

*From ROADMAP.md -- tracked per phase*

**Next Immediate Actions:**
1. Execute Plan 02-02: ImportQueue overhaul with ThreadPool integration
2. Execute Plan 02-03: Metadata extraction and database schema
3. Execute remaining Phase 2 plans (02-04 through 02-08)

### Current Blockers

**None** -- Phase 2 Import Pipeline 100% complete (10/10 plans). All gap closure issues resolved. Ready for Phase 3 planning.

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 2 | Fix panel UI clipping when docks are resized smaller than content | 2026-02-10 | dc897b7 | [2-fix-panel-ui-clipping-when-docks-are-res](./quick/2-fix-panel-ui-clipping-when-docks-are-res/) |

---

## Session Continuity

### What Was Just Accomplished

**Session Goal:** Execute Phase 02 Plan 10 (Import Pipeline Gap Closure - Final Plan)

**Completed:**
- Task 1: Throttle processCompletedImports to one per frame and remove viewport auto-focus
- Task 2: Fix thumbnail generation error propagation, retry, and notification
- Added m_pendingCompletions queue to FileIOManager for throttled processing
- Process at most one completed import per frame to prevent UI blocking
- Removed viewport->setMesh() call (user decision: focus stays unchanged)
- Fixed null thumbnail generator to return false (not true)
- Added retry logic for thumbnail generation (handles transient GL errors)
- Show warning toast on thumbnail failure with model name
- All 422 tests pass
- Execution time: 1m 35s

**Artifacts Created:**
- `.planning/phases/02-import-pipeline/02-10-SUMMARY.md` -- Plan 10 summary

**Artifacts Modified:**
- `src/managers/file_io_manager.h` -- Added m_pendingCompletions queue, ImportTask forward declaration (abd71e0)
- `src/managers/file_io_manager.cpp` -- Throttled completion processing, removed viewport focus, added retry/toast (abd71e0)
- `src/core/library/library_manager.cpp` -- Fixed null generator return value (abd71e0)
- `.planning/STATE.md` -- Updated to Plan 10/10 complete, Phase 2 COMPLETE

**Commits:**
- `abd71e0` -- fix(02-10): throttle import completion and fix thumbnail error handling

### What to Do Next

**Phase 2 Import Pipeline - COMPLETE (10/10 Plans - 100%)**

All Phase 2 plans completed:
- Plan 02-01: ThreadPool Infrastructure & Import Config ✓
- Plan 02-02: Database Schema for G-code Storage ✓
- Plan 02-03: G-code Loader and File Handler ✓
- Plan 02-04: Parallel Import Queue ✓
- Plan 02-05: Background Import UI Feedback ✓
- Plan 02-06: Import Pipeline Integration & G-code Auto-Association ✓
- Plan 02-07: Toolpath Visualization and Library Panel G-code Integration ✓
- Plan 02-08: Import Pipeline Hardening and End-to-End Verification ✓
- Plan 02-09: Recursive Folder Drag-and-Drop (Gap Closure) ✓
- Plan 02-10: Import Pipeline Gap Closure (Non-blocking UI, Thumbnail Errors) ✓

**Success Criteria Met (Plan 02-10):**
- [x] processCompletedImports processes at most 1 task per frame
- [x] Remaining completions queued and processed in subsequent frames
- [x] Viewport focus does not change when imports complete
- [x] Properties panel still receives mesh for metadata display
- [x] Library panel still refreshes on each processed completion
- [x] Null thumbnail generator returns false (not true)
- [x] generateThumbnail return value checked with retry logic
- [x] User sees warning toast when thumbnail fails
- [x] Toast includes model name for identification
- [x] All 422 tests pass

**Next Actions:**
- Complete Phase 2 milestone review
- Review ROADMAP.md for Phase 3 planning
- Consider quick tasks or architectural improvements

---

## Project Health

**Code Quality:**
- Baseline: 1,108-line god class (Application.cpp)
- Target: Under 400 lines after sub-phase 1.4
- Result: 374 lines (66% reduction) -- TARGET MET

**Test Coverage:**
- Baseline: Core modules tested (loaders, database, mesh, optimizer)
- Infrastructure: EventBus (10 tests), ConnectionPool (10 tests), MainThreadQueue (10 tests) -- all passing
- Target: 100% coverage of new infrastructure -- ACHIEVED

**Tech Debt:**
- Baseline: 58 items in TODOS.md
- Phase 1 Scope: 7 bugs + 6 dead code items + 1 god class = 14 items
- Completed: All 14 Phase 1 items resolved
- Remaining: 44 items (incomplete implementations, error handling, etc.)

**Threading Safety:**
- Baseline: Single SQLite connection (not thread-safe), ImportProgress race condition
- Infrastructure: WAL mode enabled, ConnectionPool (2 connections), MainThreadQueue, ASSERT_MAIN_THREAD assertions
- Documentation: docs/THREADING.md created with complete threading contracts
- Target: All threading infrastructure complete -- ACHIEVED

---

## Quick Reference

**Project Root:** `/data/DW/`

**Key Files:**
- Roadmap: `.planning/ROADMAP.md`
- Requirements: `.planning/REQUIREMENTS.md`
- Project Definition: `.planning/PROJECT.md`
- Gap Analysis: `.planning/TODOS.md`
- Research: `.planning/research/SUMMARY.md`

**Manager Architecture:**
- UIManager: `src/managers/ui_manager.h/.cpp` (panels, dialogs, visibility, menu, shortcuts)
- FileIOManager: `src/managers/file_io_manager.h/.cpp` (import, export, project, drag-drop)
- ConfigManager: `src/managers/config_manager.h/.cpp` (config watch, apply, workspace, settings, relaunch)

**Commands:**
- Plan next sub-phase: `/gsd:plan-phase 1.5`
- Execute plan: `/gsd:execute-plan`
- Mark complete: `/gsd:complete-phase 1.4`
- Insert urgent work: `/gsd:insert-phase 1.5`

**Resources:**
- Architecture doc: `.planning/codebase/ARCHITECTURE.md`
- Code conventions: `.planning/codebase/CONVENTIONS.md`
- Threading contracts: `docs/THREADING.md`

---

*State initialized: 2026-02-08*
*Last activity: 2026-02-10 - Completed quick task 2: Fix panel UI clipping when docks are resized smaller than content*

29. **Toolpath Visualization via Extruded Quads (2026-02-10, Plan 02-03)**
    - Decision: Convert G-code path segments to extruded quads (0.5mm cutting, 0.2mm rapid)
    - Rationale: Existing renderer expects triangle meshes; thin extrusions provide visual distinction between move types
    - Impact: G-code toolpaths render in viewport using existing mesh infrastructure

30. **Move Type Encoding in Vertex TexCoord (2026-02-10, Plan 02-03)**
    - Decision: Store rapid/cutting flag in vertex texCoord.x (1.0 = rapid, 0.0 = cutting)
    - Rationale: Enables shader-based visual differentiation (e.g., different colors for rapid vs cutting moves)
    - Impact: Shader can distinguish move types without additional vertex attributes

31. **Cross-Filesystem Move Strategy (2026-02-10, Plan 02-03)**
    - Decision: Try atomic rename first, fallback to copy + size verification + delete source
    - Rationale: Atomic rename is faster when possible; cross-filesystem requires copy+delete (per research Pitfall 4)
    - Impact: Robust file moves work across different mount points and filesystems

32. **Unique Filename Generation Pattern (2026-02-10, Plan 02-03)**
    - Decision: filename_1.ext, filename_2.ext, etc., capped at 10000 attempts
    - Rationale: Prevents file overwrites while avoiding infinite loops in pathological cases
    - Impact: Library files never overwrite existing files, but won't hang on name collision storms

33. **ThreadPool Lazy Initialization (2026-02-10, Plan 02-04)**
    - Decision: Create ThreadPool on first enqueue, not at ImportQueue construction
    - Rationale: Avoids creating threads when import queue is unused, saves resources
    - Impact: Zero overhead when application doesn't perform imports

34. **Manual GCodeLoader Instantiation (2026-02-10, Plan 02-04)**
    - Decision: Directly create GCodeLoader for G-code files, not via LoaderFactory cast
    - Rationale: Type-safe approach, avoids unsafe downcast from base class pointer
    - Impact: Clean type-specific import paths for mesh vs G-code without runtime type checks

35. **Batch Completion Detection via Atomic Counter (2026-02-10, Plan 02-04)**
    - Decision: Atomic remaining tasks counter, last task fires callback
    - Rationale: Simple, lock-free, no sentinel task or periodic polling needed
    - Impact: Zero coordination overhead between workers, batch completion is instant

36. **Auto-Detect Suffix Stripping for G-code Matching (2026-02-10, Plan 02-06)**
    - Decision: Strip common G-code suffixes (_roughing, _finishing, etc.) before model name matching
    - Rationale: Enables matching of 'chair_roughing.gcode' to 'chair.stl' model
    - Impact: Auto-association works for typical CNC workflow naming patterns

37. **Auto-Association Group Creation Strategy (2026-02-10, Plan 02-06)**
    - Decision: Create 'Imported' group if model has no groups, otherwise use first existing group
    - Rationale: Simple, predictable behavior - always auto-associates to a sensible location
    - Impact: G-code files organized under models without requiring user intervention

38. **ConnectionPool Sizing for Parallel Workers (2026-02-10, Plan 02-06)**
    - Decision: Size pool to max(4, calculateThreadCount(tier) + 2) connections
    - Rationale: Ensures enough connections for parallel workers plus main thread and overhead
    - Impact: No connection exhaustion during parallel import batches

39. **Modal Import Overlay Removal (2026-02-10, Plan 02-06)**
    - Decision: Remove renderImportProgress() modal, rely on StatusBar for progress
    - Rationale: Aligns with non-blocking background import design, reduces UI clutter
    - Impact: Import is fully background - user can continue working without modal interruption

36. **StatusBar Widget Integrates LoadingState and ImportProgress (2026-02-10, Plan 02-05)**
    - Decision: Single StatusBar widget displays both model loading state and import progress
    - Rationale: Maintains existing LoadingState functionality while adding non-blocking import feedback
    - Impact: StatusBar serves dual purpose - eliminates need for separate status displays

37. **ToastManager Rate Limiting for Import Errors (2026-02-10, Plan 02-05)**
    - Decision: Limit to 10 error toasts per batch, then show summary toast
    - Rationale: Prevents notification flood during large batch imports with many failures
    - Impact: UI remains responsive even with hundreds of errors, user still aware of issues

38. **Background UI Rendering Method (2026-02-10, Plan 02-05)**
    - Decision: UIManager.renderBackgroundUI(deltaTime, loadingState) orchestrates StatusBar, ToastManager, ImportSummaryDialog
    - Rationale: Single entry point for all background UI rendering, clean separation from panels/dialogs
    - Impact: Application calls one method instead of three separate render calls

39. **Old Modal Methods Deprecated Not Removed (2026-02-10, Plan 02-05)**
    - Decision: Mark renderImportProgress and renderStatusBar as deprecated, keep functional
    - Rationale: Application still calls these methods - removal deferred to Plan 06/07 wiring
    - Impact: No breaking changes to Application in this plan, smooth migration path

40. **Toolpath Color Scheme (2026-02-10, Plan 02-07)**
    - Decision: Blue for cutting (G1), orange-red for rapid (G0) with shader blending
    - Rationale: Visually distinct colors, rapid moves slightly transparent (0.8 alpha)
    - Impact: Clear visual distinction between move types without separate render passes

41. **Library View Organization (2026-02-10, Plan 02-07)**
    - Decision: Tabbed view with All | Models | G-code tabs
    - Rationale: Clear separation, user can filter by type or see everything
    - Impact: Scalable UI as more file types added (e.g., cut plans in future)

42. **Camera Auto-Fit Behavior for Toolpath (2026-02-10, Plan 02-07)**
    - Decision: Auto-fit to toolpath bounds only when no mesh is displayed
    - Rationale: Prevents disrupting user's view when model + toolpath both loaded
    - Impact: Toolpath-only viewing gets proper framing, mixed viewing preserves model focus

43. **Grid and Axis Rendering (2026-02-10, Plan 02-07)**
    - Decision: Add renderGrid() and renderAxis() calls to ViewportPanel
    - Rationale: Renderer provides these methods, viewport should use them for spatial context
    - Impact: Users can see 3D orientation and scale reference (deviation - missing functionality)

44. **Loader Error Handling Strategy (2026-02-10, Plan 02-08)**
    - Decision: Return descriptive LoadResult with error strings, never crash on corrupt input
    - Rationale: Corrupt user files should provide helpful feedback, not crash the application
    - Impact: All loaders wrap parse logic in try/catch, return actionable error messages

45. **NaN/Inf Validation in Mesh Loaders (2026-02-10, Plan 02-08)**
    - Decision: Wrap float parsing in try/catch, reject file on NaN/Inf detection
    - Rationale: NaN/Inf values corrupt mesh rendering and physics, better to reject file early
    - Impact: STL/OBJ/3MF loaders validate all vertex coordinates before accepting file

46. **Extreme Coordinate Bounds Check (2026-02-10, Plan 02-08)**
    - Decision: Reject vertices with any coordinate >1e6 as likely corrupt
    - Rationale: Legitimate models rarely exceed 1000 units, >1e6 indicates corruption or unit errors
    - Impact: Early detection of corrupted files with nonsensical coordinate values

47. **Missing MTL File Handling (2026-02-10, Plan 02-08)**
    - Decision: Log warning and continue loading OBJ without materials
    - Rationale: Geometry is valid, missing colors/textures shouldn't prevent import
    - Impact: Graceful degradation - user gets model even if material file is missing

48. **3MF Archive Validation Level (2026-02-10, Plan 02-08)**
    - Decision: Validate archive size and structure, parse XML for geometry
    - Rationale: Balance between performance and robustness - catch common issues early
    - Impact: 3MF loader detects corrupt/truncated archives before attempting full parse

49. **Recursive Directory Scanning for Drag-and-Drop (2026-02-10, Plan 02-09)**
    - Decision: Use fs::recursive_directory_iterator to scan dropped folders
    - Rationale: Enables users to import entire project folders in one drag-and-drop operation
    - Impact: Gap closure - users can now drop folders of models instead of selecting files individually

50. **Filesystem Error Handling in Directory Scanning (2026-02-10, Plan 02-09)**
    - Decision: Log warnings and continue on filesystem_error (permission denied, broken symlinks)
    - Rationale: Partial results better than complete failure - one inaccessible subfolder shouldn't block entire import
    - Impact: Graceful degradation - application remains stable with problematic directory structures

51. **Per-Frame Import Completion Throttling (2026-02-10, Plan 02-10)**
    - Decision: Process at most one completed import per frame via m_pendingCompletions queue
    - Rationale: Thumbnail generation and GPU mesh upload are expensive - processing many in one frame blocks the UI
    - Impact: Import completion spreads across frames, UI remains responsive during large batch imports

52. **Viewport Focus Preservation During Import (2026-02-10, Plan 02-10)**
    - Decision: Remove viewport->setMesh() call from processCompletedImports, keep properties panel update
    - Rationale: User decision stated "After batch completes, viewport focus does not change"
    - Impact: Properties panel updates for metadata display (lightweight), but viewport rendering stays unchanged

53. **Null Thumbnail Generator Error Propagation (2026-02-10, Plan 02-10)**
    - Decision: Change null generator check from returning true to returning false
    - Rationale: Returning true masked the fact that no thumbnail was generated, preventing error handling upstream
    - Impact: Callers now correctly informed when thumbnails fail to generate

54. **Thumbnail Generation Retry with Toast Notification (2026-02-10, Plan 02-10)**
    - Decision: Check generateThumbnail return value, retry once, show warning toast on failure
    - Rationale: Framebuffer creation can fail transiently - one retry handles most GL errors, toast informs user of persistent failures
    - Impact: Reduced false-positive errors, users aware of thumbnail generation issues
