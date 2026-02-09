# Roadmap: Phase 1 — Architectural Foundation & Thread Safety

**Milestone Goal:** Decompose the Application god class, establish thread-safe infrastructure, and fix existing bugs/debt — creating a solid foundation for all future feature phases.

**Scope:** Phase 1 only (of 8 identified phases)
**Total Sub-Phases:** 6
**Depth:** Standard (5-8 sub-phases)

---

## Success Criteria

At completion of Phase 1, the following must be TRUE:

1. **Application.cpp is under 400 lines** — Thin coordinator delegates to focused managers
2. **All existing tests pass** — No behavioral regressions
3. **New unit tests exist** — EventBus, ConnectionPool, MainThreadQueue have test coverage
4. **SQLite WAL mode enabled** — Concurrent import + UI query works without SQLITE_BUSY
5. **No dead code from DEAD-01 through DEAD-06 remains** — Codebase is clean
6. **All 7 bugs (BUG-01 through BUG-07) are fixed** — Known issues resolved
7. **Threading contract documented and enforced** — Debug assertions catch violations

---

## Sub-Phase Overview

| Sub-Phase | Goal | Complexity | Risk |
|-----------|------|------------|------|
| 1.1 | EventBus for decoupled communication | M | LOW |
| 1.2 | SQLite ConnectionPool & WAL mode | L | MEDIUM |
| 1.3 | MainThreadQueue & threading contracts | M | MEDIUM |
| 1.4 | Application god class decomposition | L | HIGH |
| 1.5 | Bug fixes (BUG-01 through BUG-07) | M | LOW |
| 1.6 | Dead code cleanup & validation | S | LOW |

**Complexity:** S = Small (1-3 days), M = Medium (3-5 days), L = Large (5-10 days)

---

## Sub-Phase 1.1: EventBus for Decoupled Communication

**Goal:** Implement publish/subscribe system for cross-subsystem communication, establishing the foundation for decoupled component interactions.

**Why First:** Zero dependencies, foundational for all future work. Low risk, high value. Application decomposition (1.4) will use EventBus to decouple subsystems.

**Plans:** 2 plans

Plans:
- [x] 1.1-01-PLAN.md — TDD: EventBus core implementation with type-safe pub/sub, weak_ptr cleanup, and unit tests
- [x] 1.1-02-PLAN.md — Integrate EventBus into Application lifecycle

**Tasks:**

1. **Design EventBus interface**
   - Template-based event type registration
   - Compile-time type safety for event dispatch
   - Weak reference storage for subscribers (prevent memory leaks)
   - Main-thread-only execution contract (no cross-thread dispatch)

2. **Implement EventBus core**
   - Create `src/core/events/event_bus.h` and `event_bus.cpp`
   - Event type registration with type_index mapping
   - Subscribe/unsubscribe with weak_ptr to handlers
   - Publish method with type-safe dispatch
   - Automatic cleanup of expired weak references

3. **Write unit tests**
   - Create `tests/test_event_bus.cpp`
   - Test: Subscribe and receive events
   - Test: Multiple subscribers for same event
   - Test: Unsubscribe removes handler
   - Test: Weak references prevent memory leaks (handler destroyed)
   - Test: Type safety (compile-time event type checking)

4. **Integration preparation**
   - Document EventBus usage patterns for future subsystems
   - Add EventBus instance to Application (single global instance)
   - Prepare example events (WorkspaceChanged, ImportCompleted)

**Files Affected:**
- NEW: `src/core/events/event_bus.h`
- NEW: `src/core/events/event_bus.cpp`
- NEW: `tests/test_event_bus.cpp`
- MODIFIED: `src/app/application.h` (add EventBus member)
- MODIFIED: `src/app/application.cpp` (initialize EventBus)

**Dependencies:** None

**Success Criteria:**
- EventBus compiles and links successfully
- All EventBus unit tests pass (100% coverage of core API)
- Example events defined and documented
- No memory leaks in subscribe/unsubscribe cycles (verified by Valgrind or sanitizers)

**Risk Flags:**
- LOW: Well-established pattern, no threading complexity at this stage
- Template complexity manageable with C++17 features

---

## Sub-Phase 1.2: SQLite ConnectionPool & WAL Mode

**Goal:** Enable SQLite WAL (Write-Ahead Logging) mode and implement connection pooling to support concurrent read/write from multiple threads without SQLITE_BUSY errors.

**Why Second:** Critical for thread safety, independent of EventBus. Must be in place before ImportQueue uses pooled connections (sub-phase 1.3).

**Plans:** 2 plans

Plans:
- [ ] 1.2-01-PLAN.md — TDD: ConnectionPool and ScopedConnection with Database enhancements
- [ ] 1.2-02-PLAN.md — Integrate ConnectionPool into ImportQueue and Application

**Tasks:**

1. **Enable WAL mode**
   - Modify `src/core/database/database.cpp`
   - Add `PRAGMA journal_mode=WAL` on database open
   - Add `PRAGMA synchronous=NORMAL` for performance
   - Verify WAL mode active with query check

2. **Design ConnectionPool**
   - Pool maintains 2-4 SQLite connections
   - Thread-safe acquire/release semantics (mutex-protected queue)
   - Per-thread connection affinity (connections not shared across threads)
   - RAII wrapper (ScopedConnection) for automatic release
   - Connection validation on acquire (test with simple query)

3. **Implement ConnectionPool**
   - Create `src/core/database/connection_pool.h` and `connection_pool.cpp`
   - Initialize pool with N connections on startup
   - Acquire blocks if pool exhausted (with timeout and error)
   - Release returns connection to available queue
   - Shutdown drains pool and closes all connections

4. **Refactor Database to use pool**
   - Keep existing `Database` class as single-connection wrapper
   - Add `ConnectionPool` as separate class
   - Update `Database::open()` to enable WAL mode
   - Document when to use pool vs single connection

5. **Update ImportQueue to use pool**
   - Modify `src/core/import/import_queue.cpp`
   - Worker thread acquires connection from pool
   - All database operations use pooled connection
   - Connection released back to pool after import task completes

6. **Write unit tests**
   - Create `tests/test_connection_pool.cpp`
   - Test: Acquire and release connection
   - Test: Concurrent access (multiple threads acquire simultaneously)
   - Test: Pool exhaustion handling (timeout, error return)
   - Test: WAL mode allows concurrent read/write
   - Test: Connection validation on acquire
   - Integration test: ImportQueue + UI query simultaneous access (no SQLITE_BUSY)

**Files Affected:**
- NEW: `src/core/database/connection_pool.h`
- NEW: `src/core/database/connection_pool.cpp`
- NEW: `tests/test_connection_pool.cpp`
- MODIFIED: `src/core/database/database.cpp` (enable WAL mode)
- MODIFIED: `src/core/import/import_queue.h` (add ConnectionPool member)
- MODIFIED: `src/core/import/import_queue.cpp` (use pooled connections)
- MODIFIED: `src/app/application.cpp` (initialize ConnectionPool, pass to ImportQueue)

**Dependencies:** None (independent work)

**Success Criteria:**
- WAL mode enabled and verified in database file
- ConnectionPool unit tests pass with concurrent access
- ImportQueue uses pooled connections (no shared single connection)
- Integration test: Concurrent import + UI query executes without SQLITE_BUSY
- No connection leaks (all acquired connections released)

**Risk Flags:**
- MEDIUM: SQLite threading is subtle — WAL mode is critical
- MEDIUM: Connection pool synchronization must be correct (race conditions possible)
- Mitigation: ThreadSanitizer during testing, extensive unit tests

---

## Sub-Phase 1.3: MainThreadQueue & Threading Contracts

**Goal:** Implement thread-safe queue for background workers to post results to main thread, and document threading contracts for ImGui safety.

**Why Third:** Builds on understanding from ConnectionPool (1.2). Critical for preventing ImGui threading violations identified in research (Pitfall 3).

**Tasks:**

1. **Design MainThreadQueue**
   - Lock-free or mutex-protected queue (std::queue with std::mutex)
   - Post from worker threads (enqueue callable or event)
   - Process on main thread during frame update (Application::update())
   - Type-safe task wrappers (std::function<void()>)
   - Bounded queue with overflow handling (drop or block)

2. **Implement MainThreadQueue**
   - Create `src/core/threading/main_thread_queue.h` and `main_thread_queue.cpp`
   - Thread-safe enqueue method (std::lock_guard)
   - Main thread process method (drain queue, execute callables)
   - Clear method for shutdown
   - Size limit and overflow policy

3. **Integrate with Application**
   - Add MainThreadQueue instance to Application
   - Call processQueue() in Application::update() (each frame)
   - Replace any direct cross-thread UI updates with queue posts
   - Update ImportQueue to post completion events to MainThreadQueue

4. **Document threading contracts**
   - Create `docs/THREADING.md` or add to `rulebook/`
   - Contract: All ImGui calls on main thread ONLY
   - Contract: Workers post results to MainThreadQueue, never call UI directly
   - Contract: Database connections are per-thread (never share)
   - Contract: EventBus is main-thread-only
   - Add debug assertions to enforce contracts (check thread ID)

5. **Add thread ownership assertions**
   - Create `src/core/utils/thread_utils.h` with ASSERT_MAIN_THREAD macro
   - Add assertions to critical UI entry points (Panel::render, Workspace setters)
   - Add assertions to EventBus::publish
   - Enable assertions in debug builds only

6. **Write unit tests**
   - Create `tests/test_main_thread_queue.cpp`
   - Test: Post from worker thread, process on main thread
   - Test: Multiple posts maintain ordering
   - Test: Bounded queue handles overflow
   - Test: Clear empties queue
   - Integration test: ImportQueue posts to MainThreadQueue, Application processes

**Files Affected:**
- NEW: `src/core/threading/main_thread_queue.h`
- NEW: `src/core/threading/main_thread_queue.cpp`
- NEW: `src/core/utils/thread_utils.h` (assertions)
- NEW: `docs/THREADING.md` (threading contracts)
- NEW: `tests/test_main_thread_queue.cpp`
- MODIFIED: `src/app/application.h` (add MainThreadQueue member)
- MODIFIED: `src/app/application.cpp` (initialize queue, call processQueue())
- MODIFIED: `src/core/import/import_queue.cpp` (post to MainThreadQueue)
- MODIFIED: `src/ui/panels/panel.h` (add ASSERT_MAIN_THREAD to render())
- MODIFIED: `src/app/workspace.cpp` (add thread assertions)

**Dependencies:**
- Sub-phase 1.2 (understanding of thread-safe patterns from ConnectionPool)

**Success Criteria:**
- MainThreadQueue unit tests pass
- Threading contracts documented in THREADING.md
- Debug assertions added to main-thread-only code paths
- ImportQueue posts completion to MainThreadQueue (not direct UI update)
- No threading violations detected by ThreadSanitizer

**Risk Flags:**
- MEDIUM: Requires discipline to use queue consistently (code review critical)
- LOW: Queue implementation is straightforward (mutex + std::queue)
- Mitigation: Debug assertions catch violations early

---

## Sub-Phase 1.4: Application God Class Decomposition

**Goal:** Extract concerns from Application.cpp (1,071 lines) into focused managers, reducing to ~300 lines (thin coordinator).

**Why Fourth:** This is the highest-risk refactoring. Doing it AFTER infrastructure (EventBus, ConnectionPool, MainThreadQueue) is in place means extracted managers can use those systems immediately.

**Tasks:**

1. **Identify extraction candidates**
   - Audit Application.cpp for distinct responsibilities
   - Candidates from research:
     - UI management (panel/dialog lifecycle, layout persistence)
     - File I/O dispatch (import, export, project open/save)
     - Event callback routing
   - Aim for 3-4 focused managers, each under 800 lines

2. **Extract UIManager**
   - Responsibilities: Panel/dialog lifecycle, ImGui frame setup, layout persistence
   - Create `src/managers/ui_manager.h` and `ui_manager.cpp`
   - Move panel instantiation and rendering from Application
   - Move layout save/restore logic
   - Move theme management
   - UIManager owns panels, exposes render() method called by Application
   - Keep existing `src/ui/ui_manager.h` or consolidate (decide based on overlap)

3. **Extract FileIOManager**
   - Responsibilities: Import/export dispatch, project open/save, file dialog coordination
   - Create `src/managers/file_io_manager.h` and `file_io_manager.cpp`
   - Move import workflow (drag/drop, file dialog, LibraryManager calls)
   - Move export workflow
   - Move project open/save/close logic (ProjectManager calls)
   - FileIOManager orchestrates LibraryManager, ProjectManager, ImportQueue

4. **Extract EventCoordinator (or integrate into Application)**
   - Responsibilities: Route events between subsystems using EventBus
   - Option A: Create EventCoordinator class that subscribes to events and dispatches
   - Option B: Keep event routing in Application but use EventBus (simpler)
   - Decision: Option B — Application subscribes to EventBus, routes to managers
   - Application becomes thin orchestrator: init managers, subscribe to events, delegate

5. **Refactor Application**
   - Reduce Application.cpp to:
     - Manager initialization (UIManager, FileIOManager, LibraryManager, ProjectManager, etc.)
     - Event loop (processEvents, update, render)
     - EventBus subscription and delegation
     - Shutdown and cleanup
   - Target: ~300 lines
   - All panel rendering delegated to UIManager.render()
   - All file operations delegated to FileIOManager
   - Workspace updates use EventBus notifications

6. **Update dependencies**
   - Managers receive Workspace, EventBus, Database via dependency injection
   - No manager directly accesses other managers (use EventBus for cross-manager communication)
   - Application owns all managers, passes references during construction

7. **Verify behavior unchanged**
   - Run all existing tests (should pass)
   - Manual testing: Import, export, project open/save, panel interactions
   - No visible UX changes (NR1: No Behavioral Changes)

**Files Affected:**
- NEW: `src/managers/ui_manager.h` (or consolidate with existing `src/ui/ui_manager.h`)
- NEW: `src/managers/ui_manager.cpp`
- NEW: `src/managers/file_io_manager.h`
- NEW: `src/managers/file_io_manager.cpp`
- MODIFIED: `src/app/application.h` (add manager members, reduce responsibilities)
- MODIFIED: `src/app/application.cpp` (refactor from 1,071 → ~300 lines)
- MODIFIED: All panels (update constructor signatures if manager dependencies change)

**Dependencies:**
- Sub-phase 1.1 (EventBus for decoupled communication)
- Sub-phase 1.3 (MainThreadQueue for worker communication)

**Success Criteria:**
- Application.cpp is under 400 lines (target ~300)
- All existing tests pass
- Manual testing shows no behavioral changes
- UIManager handles all panel/dialog lifecycle
- FileIOManager handles all import/export/project file operations
- Code compiles with no warnings
- clang-tidy reports no new issues

**Risk Flags:**
- HIGH: Large refactoring with many touch points
- HIGH: Risk of introducing subtle bugs (state management, initialization order)
- Mitigation: Small, testable commits; run tests after each extraction; code review
- Mitigation: Extract one manager at a time (UIManager first, then FileIOManager)

---

## Sub-Phase 1.5: Bug Fixes (BUG-01 through BUG-07)

**Goal:** Fix all 7 bugs identified in TODOS.md, improving performance, correctness, and robustness.

**Why Fifth:** Many bugs are independent and can be fixed after infrastructure is stable. BUG-06 (ImportProgress race) is already addressed by MainThreadQueue (1.3).

**Tasks:**

1. **BUG-01: Cache ViewCube geometry**
   - File: `src/ui/panels/viewport_panel.cpp:279-451`
   - Issue: ViewCube recomputed every frame even when camera stationary
   - Fix: Cache geometry, recompute only on camera change
   - Add dirty flag set by camera updates
   - Store cached vertices, faces, depth order

2. **BUG-02: Use fmod for camera angle wrapping**
   - File: `src/render/camera.cpp:38-41`
   - Issue: O(n) loop for angle wrapping
   - Fix: Replace `while (m_yaw > 360) m_yaw -= 360;` with `m_yaw = fmod(m_yaw, 360.0f);`
   - Apply to both yaw and pitch

3. **BUG-03: Zero dimensions in framebuffer move constructor**
   - File: `src/render/framebuffer.cpp:12-21`
   - Issue: Moved-from object retains width/height while GL handles zeroed
   - Fix: Zero width/height in source object
   - Add assertions for moved-from state in debug builds

4. **BUG-04: Use std::optional for shader uniform cache**
   - File: `src/render/shader.cpp:94-103`
   - Issue: -1 cached identically to valid locations
   - Fix: Use `std::optional<GLint>` or don't cache -1
   - Update getUniformLocation to return optional
   - Update callers to check has_value()

5. **BUG-05: Escape LIKE wildcards in database searches**
   - Files: `src/core/database/model_repository.cpp:103,139`, `project_repository.cpp:69`
   - Issue: `%` and `_` in user input not escaped
   - Fix: Implement LIKE escape function
   - Replace `%` with `\%`, `_` with `\_`
   - Use `ESCAPE '\'` in SQL queries
   - Apply to all LIKE queries in repositories

6. **BUG-06: Fix ImportProgress race condition**
   - File: `src/core/import/import_queue.cpp`
   - Issue: `ImportProgress::currentFileName` char[256] written by worker, read by UI without sync
   - Fix: Already addressed by MainThreadQueue (sub-phase 1.3)
   - Verify: ImportQueue posts progress updates to MainThreadQueue
   - UI reads progress from main thread only
   - Remove char[256], use std::string with proper synchronization

7. **BUG-07: Fix normal matrix for non-uniform scaling**
   - File: `src/render/renderer.cpp:108-109`
   - Issue: Normal matrix assumes uniform scaling
   - Fix: Use `transpose(inverse(mat3(modelMatrix)))` for normal matrix
   - Update shader to use corrected normal matrix
   - Test with non-uniformly scaled models

8. **Write/update tests for bug fixes**
   - Add tests for ViewCube caching (no recomputation when camera unchanged)
   - Add tests for camera angle wrapping edge cases
   - Add tests for framebuffer move semantics
   - Add tests for shader uniform cache with missing uniforms
   - Add tests for LIKE wildcard escaping (search for "100%" doesn't match everything)
   - Add tests for non-uniform scaling (verify correct lighting)

**Files Affected:**
- MODIFIED: `src/ui/panels/viewport_panel.cpp` (BUG-01)
- MODIFIED: `src/render/camera.cpp` (BUG-02)
- MODIFIED: `src/render/framebuffer.cpp` (BUG-03)
- MODIFIED: `src/render/shader.h` (BUG-04)
- MODIFIED: `src/render/shader.cpp` (BUG-04)
- MODIFIED: `src/core/database/model_repository.cpp` (BUG-05)
- MODIFIED: `src/core/database/project_repository.cpp` (BUG-05)
- MODIFIED: `src/core/import/import_queue.cpp` (BUG-06)
- MODIFIED: `src/render/renderer.cpp` (BUG-07)
- MODIFIED: `tests/test_camera.cpp` (BUG-02 tests)
- MODIFIED: `tests/test_database.cpp` (BUG-05 tests)
- NEW: `tests/test_viewport_panel.cpp` (BUG-01 tests, if testable)
- MODIFIED: Various test files for shader, renderer

**Dependencies:**
- Sub-phase 1.3 (BUG-06 uses MainThreadQueue)

**Success Criteria:**
- All 7 bugs fixed and verified
- New tests added for each bug fix
- All existing tests still pass
- Manual testing confirms fixes (especially BUG-01 ViewCube, BUG-07 lighting)

**Risk Flags:**
- LOW: Most fixes are localized, low risk
- MEDIUM: BUG-07 (normal matrix) requires shader change, test thoroughly
- MEDIUM: BUG-05 (LIKE escaping) must be applied consistently across all repositories

---

## Sub-Phase 1.6: Dead Code Cleanup & Validation

**Goal:** Resolve all dead code items (DEAD-01 through DEAD-06). Research found DEAD-01 through DEAD-04 already removed, DEAD-06 is a false claim. DEAD-05 (icon system) needs FontAwesome codepoints replaced with text labels.

**Why Last:** Lowest risk, no dependencies. Clean up after all functional work is complete.

**Plans:** 1 plan

Plans:
- [ ] 01.6-01-PLAN.md — Replace FontAwesome icon codepoints with text labels, verify resolved items, update TODOS.md

**Tasks:**

1. **DEAD-01: Remove unused ViewportPanel fields**
   - File: `src/ui/panels/viewport_panel.h:53-56`
   - Remove: `m_isDragging`, `m_isPanning`, `m_lastMouseX`, `m_lastMouseY`
   - Verify: No references in .cpp
   - Test: Existing viewport tests still pass

2. **DEAD-02: Remove unused LibraryPanel field**
   - File: `src/ui/panels/library_panel.h:59`
   - Remove: `m_contextMenuModelIndex`
   - Verify: No references in .cpp
   - Test: Existing library tests still pass

3. **DEAD-03: Remove empty ProjectPanel::refresh()**
   - File: `src/ui/panels/project_panel.cpp:29-31`
   - Remove method definition
   - Remove declaration from header
   - Verify: No callers exist

4. **DEAD-04: Decide on UIManager file dialog stubs**
   - File: `src/ui/ui_manager.cpp:258-268`
   - Option A: Implement file dialogs (use native file picker or ImGui dialog)
   - Option B: Remove stubs if not used
   - Decision: Check if FileIOManager (1.4) uses these — if yes, implement; if no, remove
   - If implementing: Use platform-specific file dialog or ImGui file browser

5. **DEAD-05: Decide on icon font system**
   - File: `src/ui/icons.h` (93 icon constants)
   - Option A: Load FontAwesome icon font and use icons
   - Option B: Remove icon system if not used
   - Decision: Check if any UI code uses icon constants — if yes, load font; if no, remove
   - If loading font: Add FontAwesome TTF to resources, load in UIManager

6. **DEAD-06: Consolidate duplicate theme code**
   - Files: `src/ui/theme.cpp:56-150` vs `src/ui/ui_manager.cpp:159-213`
   - Identify which is canonical (likely `theme.cpp`)
   - Remove duplicate from `ui_manager.cpp`
   - Ensure all theme switching uses single source
   - Test: Theme switching works correctly (dark/light/high-contrast)

7. **Run code quality checks**
   - Run clang-tidy on all modified files
   - Verify no new warnings introduced
   - Run tests to ensure no regressions
   - Check that file size limits are maintained (no .cpp > 800 lines, no .h > 400 lines)

8. **Update documentation**
   - Update TODOS.md: Mark DEAD-01 through DEAD-06 as complete
   - Update any affected documentation in rulebook/

**Files Affected:**
- MODIFIED: `src/ui/panels/viewport_panel.h` (DEAD-01)
- MODIFIED: `src/ui/panels/library_panel.h` (DEAD-02)
- MODIFIED: `src/ui/panels/project_panel.h` (DEAD-03)
- MODIFIED: `src/ui/panels/project_panel.cpp` (DEAD-03)
- MODIFIED: `src/ui/ui_manager.cpp` (DEAD-04, DEAD-06)
- MODIFIED or DELETED: `src/ui/icons.h` (DEAD-05)
- MODIFIED: `src/ui/theme.cpp` (DEAD-06)
- MODIFIED: `.planning/TODOS.md` (mark items complete)

**Dependencies:**
- Sub-phase 1.4 (FileIOManager may clarify DEAD-04 decision)

**Success Criteria:**
- All 6 dead code items resolved (either removed or implemented)
- No references to removed code exist
- All tests pass
- clang-tidy reports no new issues
- File size limits maintained

**Risk Flags:**
- LOW: Dead code removal is low risk (if truly unused)
- LOW: Theme consolidation is straightforward
- MEDIUM: File dialog and icon font decisions require UX consideration

---

## Progress Tracking

| Sub-Phase | Status | Completion Date |
|-----------|--------|-----------------|
| 1.1 - EventBus | ✓ Complete | 2026-02-08 |
| 1.2 - ConnectionPool | Planning Complete | - |
| 1.3 - MainThreadQueue | Not Started | - |
| 1.4 - God Class Decomposition | Not Started | - |
| 1.5 - Bug Fixes | Not Started | - |
| 1.6 - Dead Code Cleanup | Planning Complete | - |

**Overall Progress:** 1/6 sub-phases complete (17%)

---

## Notes

**Dependency Order:**
- 1.1 and 1.2 are independent and can be done in parallel
- 1.3 depends on understanding from 1.2 (thread-safe patterns)
- 1.4 depends on 1.1 and 1.3 (uses EventBus and MainThreadQueue)
- 1.5 depends on 1.3 (BUG-06 uses MainThreadQueue)
- 1.6 depends on 1.4 (FileIOManager may clarify dead code decisions)

**Critical Path:**
1.1 EventBus → 1.3 MainThreadQueue → 1.4 God Class Decomposition → 1.5 Bug Fixes → 1.6 Cleanup

**Parallel Work Opportunities:**
- 1.1 and 1.2 can be done simultaneously (independent)
- 1.5 (most bugs) can be done alongside other work (except BUG-06)

**File Size Compliance:**
- All new files must adhere to limits: .cpp max 800 lines, .h max 400 lines
- Application decomposition (1.4) specifically targets reducing Application.cpp from 1,071 → ~300 lines

**Testing Strategy:**
- Each sub-phase includes unit test creation
- Integration tests added where cross-module interaction occurs
- All existing tests must pass after each sub-phase
- ThreadSanitizer used to detect threading issues

**Weaving Pattern:**
- Bug fixes and dead code cleanup are "woven" throughout but collected into dedicated sub-phases (1.5, 1.6) for clarity
- Alternative: Fix bugs opportunistically during infrastructure work (e.g., BUG-02 when touching camera.cpp)

---

*Roadmap created: 2026-02-08*
*Sub-phase 1.1 planned: 2026-02-08*
*Sub-phase 1.2 planned: 2026-02-08*
*Sub-phase 1.6 planned: 2026-02-08*
