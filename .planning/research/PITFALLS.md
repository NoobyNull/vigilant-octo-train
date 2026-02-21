# Pitfalls Research

**Domain:** C++ Desktop Application - Adding Real-time Communication, Threading, and Business Workflow to Existing Codebase
**Researched:** 2026-02-08
**Confidence:** MEDIUM-HIGH

---

## Critical Pitfalls

### Pitfall 1: SQLite Single Connection Shared Across Threads

**What goes wrong:**
SQLite's default "serialized" mode allows multi-threaded access but causes SQLITE_BUSY errors and timeout failures when multiple threads attempt to write simultaneously. The entire database locks during write operations, blocking all other threads. With a single connection shared across threads (as currently exists in the codebase), the write lock and transaction state machine become toxic—one thread can accidentally hold a write transaction open, causing other threads to timeout.

**Why it happens:**
Developers assume SQLite's thread-safe mode means "safe to share one connection everywhere." The existing codebase has a single Database instance, and adding background import workers will immediately create write contention. The natural reflex is to just "use the existing connection" from worker threads.

**How to avoid:**
1. **Enable WAL mode immediately** (Write-Ahead Logging): Writers and readers operate simultaneously—writers add changes to WAL file while readers read from main database file. This is the single most important fix for mixed read/write operations.
2. **One connection per thread**: Each worker thread gets its own connection. Never pass connection pointers across threads.
3. **OR use queue-based architecture**: Single database thread handles all operations, workers send requests via thread-safe queue.

**Warning signs:**
- "SQLITE_BUSY" errors appearing in logs
- Import operations hanging or timing out
- UI freezes when background imports run
- Intermittent database corruption on application crash

**Phase to address:**
Phase 1: Thread Safety Foundation (before adding any new threaded features)

**Sources:**
- [SQLite Official: Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html)
- [Multi-threaded SQLite without the OperationalErrors](https://charlesleifer.com/blog/multi-threaded-sqlite-without-the-operationalerrors/)
- [vadosware: SQLite is threadsafe and concurrent-access safe](https://vadosware.io/post/sqlite-is-threadsafe-and-concurrent-access-safe-but)

---

### Pitfall 2: God Class Becomes Threading Nightmare

**What goes wrong:**
The existing 1,071-line Application.cpp god class manages initialization, event handling, UI state, database access, rendering, and business logic. Adding threading to this architecture means the god class must now coordinate thread synchronization for all these concerns simultaneously. Every field becomes a potential race condition. Refactoring becomes impossible because everything is entangled.

**Why it happens:**
"Just add a mutex" seems easier than restructuring. Developers add `std::mutex m_appMutex;` and wrap every method with locks, creating massive contention and deadlock opportunities. The real problem: god classes have unclear boundaries for what needs thread safety.

**How to avoid:**
1. **Refactor BEFORE adding threading**: Extract concerns into focused classes (DatabaseManager, MachineController, DocumentState) with clear ownership boundaries.
2. **Identify shared mutable state**: What actually needs synchronization? UI state (main thread only), document state (command pattern with locking), machine state (dedicated thread), database (per above).
3. **Use message passing over shared state**: Workers send results to main thread via thread-safe queue instead of modifying shared objects.

**Warning signs:**
- Adding mutexes to more than 3 methods in same class
- Nested lock acquisition (lock A, then lock B)
- Methods that "sometimes" need locking depending on caller
- Temptation to make everything `mutable` to lock in const methods

**Phase to address:**
Phase 1: Architecture Cleanup (before threading changes)

**Sources:**
- [Multithreading: Handling Race Conditions and Deadlocks in C++](https://dev.to/shreyosghosh/multithreading-handling-race-conditions-and-deadlocks-in-c-4ae4)
- [Finding Race Conditions in C++](https://symbolicdebugger.com/modern-programming/finding-race-conditions/)

---

### Pitfall 3: ImGui Single-Context Threading Violations

**What goes wrong:**
ImGui is fundamentally thread-agnostic and NOT thread-safe. All ImGui calls must happen on the same thread where the context was created (typically main thread). Adding background workers that try to update UI state, trigger ImGui refreshes, or modify ImGui data structures causes crashes, corruption, or silent state desynchronization. The crash manifests as segfaults in `ImGui::Render()` or garbled UI that "sometimes" works.

**Why it happens:**
Background worker completes task and wants to update UI immediately. Natural pattern: call `m_statusText = result;` from worker thread, where `m_statusText` is rendered by ImGui on main thread. Or worse: worker calls ImGui functions directly. ImGui's internal structures (draw lists, input state) are not protected by mutexes.

**How to avoid:**
1. **Main thread owns all ImGui calls**: Zero exceptions. Not a single `ImGui::Text()` from worker threads.
2. **Workers post results to thread-safe queue**: Main thread polls queue during frame update and applies results.
3. **Double-buffering pattern for shared state**: Worker writes to `m_workerState`, main thread copies to `m_uiState` during frame, ImGui renders `m_uiState`.
4. **Never pass ImGui IDs or pointers to workers**: Workers don't know UI exists.

**Warning signs:**
- Crashes in `ImGui::Render()` or `ImGui::EndFrame()`
- "Invalid ID" assertions from ImGui
- UI shows stale data despite worker updating variables
- Race condition crashes that disappear with logging/debugging

**Phase to address:**
Phase 1: Threading Contract Establishment (document and enforce before WebSocket/workers)

**Sources:**
- [ImGui Issue #6597: Multithreading lag](https://github.com/ocornut/imgui/issues/6597)
- [ImGui Issue #221: Multithreading discussion](https://github.com/ocornut/imgui/issues/221)
- [ImGui Issue #9136: Crash when rendering in separate thread](https://github.com/ocornut/imgui/issues/9136)

---

### Pitfall 4: WebSocket Thread vs Main Event Loop Mismatch

**What goes wrong:**
WebSocket libraries (like WebSocket++) run an I/O event loop that blocks the calling thread. Running this on the main thread blocks SDL event processing and rendering, freezing the application. Running on background thread is correct, BUT callbacks from WebSocket events (on_message, on_open, on_close) execute on that background thread—so accessing shared state or calling ImGui causes race conditions (see Pitfall 3).

**Why it happens:**
Developers follow WebSocket tutorial examples that run in console applications with no GUI, where blocking the main thread is fine. Or they correctly put WebSocket on background thread but forget callbacks execute on that thread, not main thread.

**How to avoid:**
1. **Dedicated WebSocket thread with event loop**: Create thread, run `client.run()`, keep thread alive for connection lifetime.
2. **Callbacks only post to thread-safe queue**: `on_message` receives data, validates, pushes to queue, returns immediately. Never access shared state.
3. **Main thread polls queue**: During frame update, drain queue and apply machine state updates.
4. **Graceful shutdown protocol**: Send close frame, wait for `on_close` callback, join thread. Don't force-kill thread while I/O pending.

**Warning signs:**
- Application freezes when connecting to machine
- "Broken pipe" or socket errors on shutdown
- Machine state updates arrive but UI doesn't refresh
- Deadlocks during connection loss/reconnection

**Phase to address:**
Phase 2: WebSocket Infrastructure (after thread safety foundation)

**Sources:**
- [WebSocket++ Utility Client Tutorial](https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html)
- [websocket-client threading documentation](https://websocket-client.readthedocs.io/en/latest/threading.html)
- [WebSocket++ Issue #62: Threaded server](https://github.com/zaphoyd/websocketpp/issues/62)

---

### Pitfall 5: Machine State Synchronization Race Conditions

**What goes wrong:**
CNC machine sends rapid WebSocket updates (position, feed rate, status) at 10-100Hz. Application receives updates on WebSocket thread, UI renders at 60fps on main thread, user initiates commands on main thread. Without proper synchronization, UI shows stale position (read doesn't see latest write), commands get lost (overwrite between read-modify-write), or application sends duplicate commands because it didn't see acknowledgment yet.

**Why it happens:**
Developers think "just copy the data" is safe because it's a simple struct. But without atomics or locks, multi-word updates (position X, Y, Z) are not atomic—UI might read X from one update and Y from another, showing physically impossible position. Version numbers/timestamps are forgotten.

**How to avoid:**
1. **State versioning**: Every machine state update includes monotonic version number. UI only applies updates with version > current.
2. **Lock-protected machine state object**: `MachineState` with internal mutex. `getPosition()` and `updatePosition()` both lock. Hold lock only for copy, not during UI rendering.
3. **Command acknowledgment protocol**: Every command gets unique ID. Application waits for ACK before considering command applied. Timeout triggers error state.
4. **Handle out-of-order messages**: Network can reorder packets. Timestamp or sequence number allows discarding stale updates.

**Warning signs:**
- Machine position "jumps" backwards in UI
- Send command twice, machine executes once (or vice versa)
- "Lost connection" when network is fine (missed heartbeats)
- Occasional state corruption during rapid movements

**Phase to address:**
Phase 2: Machine Control Layer (same phase as WebSocket, but separate concerns)

**Sources:**
- [Synchronizing state with Websockets and JSON Patch](https://cetra3.github.io/blog/synchronising-with-websocket/)
- [Writing a WebSocket-Controlled State Machine](https://www.endpointdev.com/blog/2024/07/websocket-controlled-state-machine/)
- [WebSocket architecture best practices](https://ably.com/topic/websocket-architecture-best-practices)

---

### Pitfall 6: Command Pattern Memory Explosion

**What goes wrong:**
Implementing undo/redo with command pattern seems clean: every edit becomes a Command object pushed onto undo stack. But storing entire object snapshots (e.g., full mesh data) for every edit causes memory to balloon. A project with 50 edits × 10MB mesh each = 500MB of undo history. Application runs out of memory or becomes sluggish due to memory pressure.

**Why it happens:**
Following textbook command pattern examples that show `SaveCommand` storing entire document. Developers don't realize they must store deltas (what changed) not full state snapshots. Seems easier to "just clone the object."

**How to avoid:**
1. **Store deltas, not snapshots**: `MoveModelCommand` stores old position and new position (48 bytes), not entire mesh (megabytes).
2. **Limit undo stack depth**: Default to 50-100 commands. Provide user preference. Clear undo history on project close.
3. **Collapse consecutive commands**: User drags model, generates 100 micro-moves. Collapse into single command with start/end position.
4. **Memento pattern for complex changes**: When delta is impractical (e.g., mesh decimation), store before/after snapshots but share unchanged data structures.
5. **Manual memory management**: Commands must clean up resources. Use smart pointers (`unique_ptr<Command>`) to avoid leaks.

**Warning signs:**
- Memory usage grows without bound during editing session
- Undo becomes slower as session progresses
- Out-of-memory crashes after extended use
- Undo stack cleared only on app restart

**Phase to address:**
Phase 3: Undo/Redo System (after threading stable)

**Sources:**
- [C++ Undo Redo Frameworks Part 1](https://blog.meldstudio.co/c-undo-redo-frameworks-part-1/)
- [Implementing undo/redo with Command Pattern](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/)
- [Command Pattern (Game Programming Patterns)](https://gameprogrammingpatterns.com/command.html)

---

### Pitfall 7: SDL Event Loop + Background Worker Coordination

**What goes wrong:**
SDL event loop (`SDL_PollEvent`) runs on main thread. Background workers (import, WebSocket, PDF generation) need to trigger UI updates when complete, but cannot safely call SDL or ImGui functions. Workers that `SDL_PushEvent()` custom events seem to work initially but cause subtle timing bugs—events arrive at unpredictable times, SDL event queue can fill up, custom event data lifetime is unclear (who frees it?).

**Why it happens:**
SDL's custom events (`SDL_RegisterEvents()`) seem designed for this, and tutorials show it working. But tutorials don't cover edge cases: event queue overflow, proper memory management for event data, shutdown race conditions (worker pushes event after SDL quit).

**How to avoid:**
1. **Prefer thread-safe queue over SDL events**: Workers push to `std::queue` protected by mutex + condition variable. Main thread polls queue during frame update, before `SDL_PollEvent()`.
2. **If using SDL events, allocate event data with `new`, free in event handler**: Document ownership clearly. Use smart pointers in event data struct.
3. **Shutdown protocol**: Set `m_shuttingDown` flag (atomic), workers check before posting events, main thread drains queue after signaling shutdown.
4. **Limit event posting rate**: Worker that posts 1000 events/sec will overwhelm main thread. Batch updates or rate-limit.

**Warning signs:**
- Custom SDL events "sometimes" don't arrive
- Memory leaks in custom event data
- Crash on shutdown: worker posts event after SDL_Quit
- UI becomes unresponsive when worker is busy

**Phase to address:**
Phase 1: Worker Communication Protocol (before adding background workers)

**Sources:**
- [SDL2 common mistakes](https://nullprogram.com/blog/2023/01/08/)
- [C++ Background Worker issues](https://www.codeproject.com/Questions/1076065/Cplusplus-Background-Worker)

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Single global mutex for all threading | Quick to implement, "obviously safe" | Massive contention, deadlocks, no concurrency benefit | Never—defeats purpose of threading |
| Disable SQLite mutexes for "performance" | Marginal speed gain | Undefined behavior, crashes, corruption in multi-threaded code | Only if single-threaded forever (not this project) |
| Store full snapshots in undo commands | Simple to implement | Memory explosion, slow undo/redo | MVP with undo depth limit of 10 |
| Block main thread on WebSocket connect | Avoid threading complexity | UI freeze for 5-30 seconds on connection | Never in production |
| Poll machine state instead of WebSocket | Avoids WebSocket complexity | 1-2 second latency, server load, missed events | Early prototype/demo only |
| Hardcode PDF layout instead of template engine | Fast initial implementation | Maintenance nightmare when adding fields | Acceptable for first invoice iteration, must refactor in Phase 4 |
| Skip command pattern, implement undo ad-hoc per feature | Faster per-feature development | Inconsistent undo behavior, duplication, bugs | Never—consistency is critical |

---

## Integration Gotchas

Common mistakes when connecting to external services.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| WebSocket (ncSender) | Assume instant connection, no retry logic | Connection can take 1-10s, fail due to network/firewall. Implement exponential backoff, show "connecting..." UI, timeout after 30s |
| WebSocket message parsing | Assume all messages are well-formed JSON | Validate JSON schema, handle parse errors gracefully, log malformed messages for debugging |
| PDF generation (libharu/PoDoFo) | Include library headers in main code, tight coupling | Abstract behind `IPDFGenerator` interface, isolate library includes in .cpp file, allows switching libraries |
| PDF fonts | Assume system fonts are available | Bundle fonts with application, use relative paths, test on fresh VM with no fonts installed |
| CNC machine connection | Hardcode localhost URL | Allow user configuration, support DNS names + IP addresses, validate URL format |
| G-code analysis | Parse entire file synchronously | Stream parse in background worker, show progress, allow cancellation, handle malformed G-code |
| Thread pool (import) | Create threads on-demand | Pre-create fixed pool (std::thread::hardware_concurrency()), reuse threads, join on shutdown |

---

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| N+1 queries in import | Import becomes slower with more models | Batch queries: fetch all tags in one query, update in transaction | 100+ models in library |
| Full G-code reparse on edit | Edit latency grows with file size | Incremental parse: only reparse modified section + dependents | Files > 10,000 lines |
| Unbounded WebSocket message queue | Memory grows during rapid machine updates | Fixed-size ring buffer, drop oldest messages when full | Machine sending > 100 msg/sec |
| Render entire model list every frame | Frame rate drops with large library | Virtual scrolling: only render visible rows | Library > 500 models |
| PDF generation on main thread | UI freezes during invoice generation | Background thread + progress indicator | Invoices > 10 pages |
| Undo stack never compacted | Memory grows indefinitely | Limit depth (50-100), clear on project close | Long editing sessions (> 1 hour) |
| Lock entire database during import | UI freezes during background import | WAL mode + per-thread connections, imports don't block reads | Importing multiple files simultaneously |

---

## Security Mistakes

Domain-specific security issues beyond general web security.

| Mistake | Risk | Prevention |
|---------|------|------------|
| Trust machine state updates without validation | Malicious/buggy machine sends invalid coordinates, app crashes or corrupts project | Validate ranges (X/Y/Z within machine bounds), reject malformed messages |
| Execute user-provided G-code without sanitization | If app controls machine directly in future, malicious G-code could damage machine/material | Whitelist allowed G-codes, reject unknown commands, implement dry-run mode |
| Store database unencrypted with customer PII | Customer names, addresses, pricing in plaintext—data breach risk | SQLite encryption extension (SQLCipher) for customer data, document what's encrypted |
| Log sensitive data (prices, customer info) | Log files leak business data | Scrub logs: replace customer names with IDs, omit prices from debug logs |
| No authentication on WebSocket connection | Anyone on network can control machine | Validate API key/token in WebSocket handshake (if ncSender supports), document security model |
| PDF contains hidden metadata (file paths, usernames) | Leaks internal information to customers | Strip metadata from generated PDFs, test with PDF analyzer |

---

## UX Pitfalls

Common user experience mistakes in this domain.

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No indication WebSocket is disconnected | User changes settings, thinks machine updated, machine ignores commands | Connection status indicator (green/yellow/red), disable machine controls when disconnected |
| No feedback during long operations | User clicks "Import", nothing happens for 30s, clicks again (duplicate import) | Immediate progress dialog, show file being processed, allow cancel |
| Undo/redo with no description | User sees "Undo" but doesn't know what it will undo, afraid to click | Show action name: "Undo Move Model" / "Redo Rotate 90°" |
| Machine coordinates update too fast | Position display shows "blur" of numbers changing 60 times/sec, unreadable | Throttle UI updates to 10Hz, use smooth interpolation for visual feedback |
| Error messages with library internals | "SQLite error: SQLITE_BUSY (5) at connection 0x7f3a4c0" | User-friendly: "Database is busy, please try again" + log technical details |
| PDF generation with no preview | User prints invoice, sees layout is wrong, wastes paper | Show PDF preview before printing, allow tweaking layout |
| Background import with no cancellation | User accidentally imports 1000 files, must force-quit to stop | Cancel button stops current file, clears queue, shows "Cancelling..." |

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **WebSocket Connection:** Works in demo, but missing—reconnection logic after disconnect, error handling for malformed messages, graceful shutdown without hanging, timeout when server doesn't respond
- [ ] **Threading Implementation:** Compiles without errors, but missing—proper shutdown (join all threads), race condition testing (run with ThreadSanitizer), deadlock prevention (lock ordering documented), clean separation between UI thread and worker threads
- [ ] **SQLite WAL Mode:** Enabled in code, but missing—verify PRAGMA wal_checkpoint runs periodically, test that -wal and -shm files are cleaned up on close, confirm backups include WAL file, verify cross-platform behavior (Windows file locking)
- [ ] **Command Pattern:** Basic commands work, but missing—command coalescing (group micro-edits), undo stack depth limit, memory profiling (verify deltas not snapshots), undo/redo work after load from disk
- [ ] **PDF Generation:** Produces PDF, but missing—font embedding (works without system fonts), page breaks for long invoices, proper error handling (disk full, permission denied), metadata stripping, preview functionality
- [ ] **G-code Analysis:** Parses file, but missing—handling of malformed input (partial lines, invalid commands), cancellation support (stop mid-parse), memory limits (reject 1GB files), incremental parse for edits
- [ ] **Machine Control:** Sends commands successfully, but missing—command acknowledgment tracking, timeout handling, queue management (max pending commands), state recovery after connection loss
- [ ] **Import Queue:** Imports files, but missing—duplicate detection (skip already imported), proper transaction handling (rollback on error), thumbnail generation for all formats, consistent error reporting

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Shared SQLite connection causing SQLITE_BUSY | LOW | 1. Enable WAL mode via PRAGMA, 2. Add per-thread connections for workers, 3. Test with concurrent operations |
| God class threading deadlock | HIGH | 1. Identify minimal refactoring (extract DatabaseManager, MachineController), 2. Add interfaces, 3. Incremental extraction with tests at each step, 4. Add threading afterward |
| ImGui called from worker thread (crashes) | MEDIUM | 1. Add thread ID assertions to detect violations, 2. Implement result queue, 3. Audit codebase for ImGui calls (grep for ImGui::), 4. Add threading documentation |
| WebSocket blocking main thread | LOW | 1. Move WebSocket to dedicated thread, 2. Add message queue, 3. Test connection/disconnection, 4. Add timeout handling |
| Command pattern memory explosion | MEDIUM | 1. Profile memory usage with heap analyzer, 2. Convert snapshot commands to delta commands, 3. Add undo stack depth limit, 4. Implement command coalescing |
| Race condition in machine state | MEDIUM | 1. Add state versioning/timestamps, 2. Add mutex to MachineState class, 3. Run with ThreadSanitizer, 4. Add integration tests for concurrent access |
| PDF generation missing fonts | LOW | 1. Bundle required fonts, 2. Use relative paths, 3. Test on fresh OS install, 4. Add font fallback logic |
| Background import never completes | MEDIUM | 1. Add per-operation timeout, 2. Add cancellation token, 3. Wrap in transaction with rollback, 4. Log progress for debugging |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| SQLite single connection (Pitfall 1) | Phase 1: Thread Safety Foundation | Run concurrent import + UI operations, no SQLITE_BUSY errors |
| God class threading (Pitfall 2) | Phase 1: Architecture Cleanup | Separate DatabaseManager, MachineController, DocumentState classes with tests |
| ImGui threading violations (Pitfall 3) | Phase 1: Threading Contract | Add thread ID assertions, run with sanitizers, document contract in code comments |
| WebSocket thread mismatch (Pitfall 4) | Phase 2: WebSocket Infrastructure | Connect/disconnect/reconnect without UI freeze, graceful shutdown |
| Machine state race conditions (Pitfall 5) | Phase 2: Machine Control Layer | Stress test: 100Hz updates + rapid commands, verify state consistency |
| Command pattern memory (Pitfall 6) | Phase 3: Undo/Redo System | Profile 1-hour editing session, memory stays < 100MB, undo/redo instant |
| SDL event coordination (Pitfall 7) | Phase 1: Worker Communication | Background operations complete without SDL event issues, clean shutdown |
| PDF generation blocking UI | Phase 4: PDF/Invoicing | Generate 50-page invoice, UI remains responsive, progress indicator |
| Import queue contention | Phase 1: Thread Safety + Phase 5 | Import 100 files concurrently, no database locks, progress updates smooth |
| G-code parse performance | Phase 2 or later (if analysis needed for machine control) | Parse 100,000-line file in < 2s, cancellable, handles malformed input |

---

## Open Research Questions

Areas that need phase-specific research before implementation.

**For Phase 2 (WebSocket/Machine Control):**
- What is ncSender's exact WebSocket message format? (Need API documentation)
- Does ncSender support authentication/API keys?
- What is the message rate during typical operation? (Affects queue sizing)
- How does ncSender handle connection loss/recovery?

**For Phase 3 (Undo/Redo):**
- Which operations need undo? (All edits, or subset?)
- Should undo persist across app restart? (Affects serialization)
- How to handle undo for operations that affect multiple entities?

**For Phase 4 (PDF/Invoicing):**
- Exact layout requirements for invoices/quotes?
- Must PDFs be PDF/A compliant for archival?
- What PDF library has best C++17 support + cross-platform?

**For Phase 5 (Tool Database/Feeds & Speeds):**
- What is data model for tools? (Affects database schema)
- Are feeds/speeds calculated or looked up?
- Integration with G-code analysis?

---

## Confidence Assessment

| Topic | Level | Rationale |
|-------|-------|-----------|
| SQLite threading | HIGH | Official documentation + multiple practitioner sources confirm WAL mode + per-thread connections |
| ImGui threading | HIGH | Official GitHub issues from maintainer confirm thread-agnostic design, multiple crash reports validate pitfall |
| WebSocket threading | MEDIUM-HIGH | Official library docs + GitHub issues, but specific to WebSocket++ (library choice affects details) |
| Command pattern memory | MEDIUM | Multiple sources agree on delta storage, but specific to application domain (mesh data vs simple properties) |
| God class refactoring | MEDIUM | General C++ knowledge, but specific refactoring strategy depends on application architecture review |
| PDF integration | MEDIUM-LOW | Library options identified, but specific pitfalls depend on library choice (need phase-specific research) |
| Machine control protocols | LOW | No access to ncSender API docs, message format/rate assumptions need verification |

---

## Summary: Critical Pitfalls by Phase

**Phase 1 (Foundation):**
- SQLite threading → Must fix before any background workers
- God class refactoring → Blocks clean threading architecture
- ImGui contract → Prevents entire class of crashes

**Phase 2 (Machine Control):**
- WebSocket threading → Core feature enablement
- Machine state synchronization → Data integrity for control

**Phase 3 (Undo/Redo):**
- Command pattern memory → Usability and stability

**Phase 4 (PDF/Invoicing):**
- PDF generation blocking → UX quality

**Phase 5 (Advanced Features):**
- Import queue performance → Scalability

The most critical pitfalls (1, 2, 3, 7) must be addressed in Phase 1 before any new features. Skipping Phase 1 foundation work will cause compounding technical debt as features are added.

---

## Sources

### SQLite & Threading
- [SQLite Official: Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html)
- [Multi-threaded SQLite without the OperationalErrors](https://charlesleifer.com/blog/multi-threaded-sqlite-without-the-operationalerrors/)
- [vadosware: SQLite is threadsafe and concurrent-access safe](https://vadosware.io/post/sqlite-is-threadsafe-and-concurrent-access-safe-but)

### C++ Multithreading
- [Multithreading: Handling Race Conditions and Deadlocks in C++](https://dev.to/shreyosghosh/multithreading-handling-race-conditions-and-deadlocks-in-c-4ae4)
- [Finding Race Conditions in C++](https://symbolicdebugger.com/modern-programming/finding-race-conditions/)
- [How to debug race conditions in C/C++](https://undo.io/resources/debugging-race-conditions-cpp/)

### ImGui & Threading
- [ImGui Issue #6597: Multithreading lag](https://github.com/ocornut/imgui/issues/6597)
- [ImGui Issue #221: Multithreading discussion](https://github.com/ocornut/imgui/issues/221)
- [ImGui Issue #9136: Crash when rendering in separate thread](https://github.com/ocornut/imgui/issues/9136)

### WebSocket
- [WebSocket++ Utility Client Tutorial](https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html)
- [websocket-client threading documentation](https://websocket-client.readthedocs.io/en/latest/threading.html)
- [WebSocket++ Issue #62: Threaded server](https://github.com/zaphoyd/websocketpp/issues/62)

### Machine Control & State Synchronization
- [Synchronizing state with Websockets and JSON Patch](https://cetra3.github.io/blog/synchronising-with-websocket/)
- [Writing a WebSocket-Controlled State Machine](https://www.endpointdev.com/blog/2024/07/websocket-controlled-state-machine/)
- [WebSocket architecture best practices](https://ably.com/topic/websocket-architecture-best-practices)

### Command Pattern & Undo/Redo
- [C++ Undo Redo Frameworks Part 1](https://blog.meldstudio.co/c-undo-redo-frameworks-part-1/)
- [Implementing undo/redo with Command Pattern](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/)
- [Command Pattern (Game Programming Patterns)](https://gameprogrammingpatterns.com/command.html)

### SDL & Event Handling
- [SDL2 common mistakes](https://nullprogram.com/blog/2023/01/08/)

### PDF Generation
- [Apryse C++ SDK Documentation](https://docs.apryse.com/core/guides/get-started/cpp)
- [PoDoFo: C++17 PDF manipulation library](https://github.com/podofo/podofo)

### CNC & G-code
- [Grbl: G-code parser and CNC controller](https://github.com/gnea/grbl)
- [LinuxCNC G-Codes Documentation](https://linuxcnc.org/docs/html/gcode/g-code.html)

---

*Pitfalls research for: CNC Workshop Management Desktop Software (C++17, SDL2, ImGui, SQLite)*
*Researched: 2026-02-08*
