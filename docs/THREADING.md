# Threading Contracts -- Digital Workshop

## Overview
Document the threading model, who owns what thread, lock hierarchy, and contracts for each subsystem. Debug assertions (ASSERT_MAIN_THREAD) enforce these contracts at runtime in debug builds.

## Thread Inventory
1. **Main Thread** -- SDL event loop, ImGui rendering, EventBus dispatch, Workspace state, MainThreadQueue processing. Identified by threading::initMainThread() called in Application::init().
2. **Import Worker Thread** -- Spawned by ImportQueue::enqueue(), runs ImportQueue::workerLoop(). Reads files, parses meshes, writes to database. Posts completed tasks to m_completed vector (currently) or MainThreadQueue (future).
3. **Future worker threads** -- Any additional background work (e.g., optimizer, batch export) must follow the same pattern: do work off-thread, post results to MainThreadQueue.

## Threading Rules
1. **ImGui is NOT thread-safe.** All ImGui calls must happen on the main thread. Panel::render() is always called from Application::render() on the main thread.
2. **EventBus is main-thread-only.** No internal synchronization. publish() includes ASSERT_MAIN_THREAD(). Workers must not publish events directly -- post a lambda to MainThreadQueue that publishes.
3. **Workspace is main-thread-only.** All setters (setFocusedMesh, etc.) include ASSERT_MAIN_THREAD(). Workers must not modify workspace state directly.
4. **MainThreadQueue is the bridge.** Thread-safe enqueue from any thread, processAll() on main thread only. This is the ONLY safe way for workers to affect UI state.
5. **Database connections are per-thread** (future -- ConnectionPool in Phase 1.2). Never share a sqlite3* handle across threads.
6. **Log is thread-safe.** log::g_logMutex protects all log writes. Safe to call from any thread.

## Lock Hierarchy
To prevent deadlocks, always acquire locks in this order:
1. MainThreadQueue::m_mutex
2. ImportQueue::m_mutex
3. log::g_logMutex

Never hold a higher-numbered lock when acquiring a lower-numbered one.

## ASSERT_MAIN_THREAD Usage
- Defined in src/core/utils/thread_utils.h
- Active in debug builds (NDEBUG not defined), no-op in release
- Call threading::initMainThread() once at startup (Application::init)
- Add ASSERT_MAIN_THREAD() to any function that must only run on main thread
- Currently enforced at: EventBus::publish(), Workspace setters, Application::renderPanels(), MainThreadQueue::processAll()

## Pattern: Worker-to-UI Communication
```cpp
// WRONG: Worker thread calling UI directly
void ImportQueue::workerLoop() {
    // ... process task ...
    workspace->setFocusedMesh(mesh);  // CRASH in debug, UB in release
}

// RIGHT: Worker posts to MainThreadQueue
void ImportQueue::workerLoop() {
    // ... process task ...
    mainThreadQueue->enqueue([mesh]() {
        workspace->setFocusedMesh(mesh);  // Runs on main thread
    });
}
```

## Subsystem Threading Summary
| Subsystem | Thread Safety | Assertions |
|-----------|--------------|------------|
| ImGui | NOT safe | Via renderPanels() |
| EventBus | Main-thread-only | publish() |
| Workspace | Main-thread-only | All setters |
| MainThreadQueue | Thread-safe (enqueue any, process main) | processAll() |
| ImportQueue | Internal mutex | None (worker is private) |
| Log | Thread-safe (g_logMutex) | None needed |
| Database | NOT safe (single connection) | None yet (Phase 1.2) |
