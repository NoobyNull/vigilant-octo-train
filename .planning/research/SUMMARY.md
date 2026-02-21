# Project Research Summary

**Project:** Digital Workshop - CNC Woodworking Desktop App
**Domain:** Desktop Application Enhancement - CNC Control, Business Workflow, Threading
**Researched:** 2026-02-08
**Confidence:** MEDIUM-HIGH

## Executive Summary

Digital Workshop is adding CNC machine control, business workflow features (customer management, quoting, invoicing), and infrastructure improvements (undo/redo, advanced search, filesystem watching) to an existing C++17/SDL2/ImGui desktop application. The research reveals that the most critical challenge is not the new features themselves, but preventing architectural collapse of the existing 1,071-line Application god class when introducing multi-threading for WebSocket connections, background imports, and real-time state synchronization.

The recommended approach is to prioritize **architectural foundation work before feature development**. Phase 1 must decompose the god class, establish thread-safe database access (SQLite WAL mode + connection pooling), implement an EventBus for cross-subsystem communication, and document threading contracts for ImGui. Only after this foundation is solid should features be added in dependency order: foundational databases (tools, materials) before dependent features (feeds/speeds calculator, cut list optimization), and synchronous features (PDF generation, customer management) before asynchronous ones (CNC control, watch folders).

The key risk is attempting to "just add threading" to the existing architecture, which will lead to race conditions, deadlocks, and maintenance nightmares. The research identifies 7 critical pitfalls that must be addressed during Phase 1 foundation work, particularly SQLite single-connection threading issues, ImGui thread-safety violations, and WebSocket event loop integration with SDL. The stack research confirms all required libraries are mature, permissive-licensed, and CMake-compatible, so technology selection is low-risk. The feature research reveals table-stakes expectations (customer management, PDF quotes, tool database, job costing) that users will judge the product by, plus competitive differentiators (CNC machine integration, advanced grain-aware cut optimization).

## Key Findings

### Recommended Stack

The stack research identified battle-tested C++ libraries that meet the project's strict constraints (C++17, permissive licenses, CMake FetchContent, under 25MB total). All core libraries are header-only or lightweight with active maintenance as of 2025-2026.

**Core technologies:**
- **cpp-httplib v0.31.0**: HTTP/REST client for CNC machine communication - MIT license, header-only, zero dependencies, perfect for REST API calls to ncSender service
- **IXWebSocket v11.4.6**: WebSocket client for real-time machine state - BSD-3-Clause, actively maintained (May 2025), simpler than WebSocket++, no Boost dependency
- **nlohmann/json latest**: JSON parsing for REST APIs and config - industry standard, MIT license, header-only, intuitive API used by millions of projects
- **libHaru v2.4.5**: PDF generation for quotes/invoices - zlib/LIBPNG license, cross-platform, mature library with comprehensive PDF features
- **efsw latest**: Filesystem monitoring for watch folders - MIT license, cross-platform (inotify/FSEvents/IOCP/kqueue), asynchronous notifications
- **ExprTk latest**: Expression parser for feeds/speeds calculator - MIT license, header-only, extremely flexible runtime expression evaluation
- **{fmt} v11.x**: String formatting for logging and UI - MIT license, optional header-only mode, type-safe printf replacement (basis for C++20 std::format)

**Total size impact:** ~1.4MB for all new libraries, well under the 25MB budget. Existing stack + new libraries = ~5MB total deployed size.

**Critical architectural decision:** Command Pattern for undo/redo should be implemented in-house (not a library) as most implementations are framework-specific or GPL/LGPL licensed. Custom implementation integrates better with existing architecture.

### Expected Features

The feature landscape research distinguishes table-stakes features (users expect them, missing = product feels incomplete) from competitive differentiators (set product apart but not expected) and anti-features (explicitly avoid building).

**Must have (table stakes):**
- Wood species database with material properties (density, grain, cost/board-foot)
- Tool database with feeds/speeds calculator (RPM, feed rate, chip loads for router bits)
- Customer management with job history and quote tracking
- PDF quote/estimate generation with itemized costs and customizable branding
- Job costing and pricing (material + labor + overhead calculations)
- Enhanced cut list with grain direction, edge banding, 1D optimization for dimensional lumber
- Material inventory tracking (sheet goods and dimensional lumber by species/thickness)
- G-code 3D visualization with stock removal simulation
- Time tracking for accurate labor costing

**Should have (competitive differentiators):**
- CNC machine control integration via REST/WebSocket (control machine from same software as planning)
- Advanced G-code simulation with collision detection (catch crashes before they happen)
- Real-time job costing with profit tracking (know profitability during production)
- Automated tool life management (track usage hours, alert for replacement)
- Watch folder auto-import (drop files, automatically process and catalog)
- Undo/redo system across application

**Defer (v2+ or never):**
- Full CAD/CAM design tools (Fusion 360, VCarve already dominate - don't compete)
- Custom CNC controller firmware (hardware-specific, high complexity, liability)
- Accounting software replacement (QuickBooks/Xero exist - export instead)
- Real-time collaboration (woodworking shops are single-user or small team, local network)
- Cloud-required architecture (many shops have unreliable internet - desktop-first)

**Feature dependencies identified:**
- Material and tool databases are foundational - multiple features reference these, build early
- Customer → Job → Quote → Costing forms a dependency pipeline
- CNC machine control enables monitoring and tool tracking, but is complex - defer until core features are solid
- Real-time costing requires basic costing foundation first

### Architecture Approach

The architecture research focuses on integrating 8 major subsystems into the existing layered C++17 desktop application without collapsing the 1,071-line Application god class. The recommended solution introduces a **Subsystems Layer** between Application and Core layers, with specialized coordinators for each major subsystem.

**Critical architectural changes required:**
1. **Decompose Application god class** into thin coordinator (~300 lines target) that delegates to SubsystemCoordinator
2. **Introduce EventBus** for cross-subsystem communication (Observer Pattern) - publishers/subscribers decouple components
3. **Implement ConnectionPool** for thread-safe database access - SQLite WAL mode + per-thread connections eliminate SQLITE_BUSY errors
4. **Add CommandManager** for undo/redo - two-stack pattern with command compression and delta storage (not snapshots)
5. **Create Service Layer** for business logic (ToolService, QuoteService, MaterialService) - keeps repositories pure data access

**Major subsystems to add:**
1. **CommandManager** - Undo/redo stack management, command registration, cross-subsystem coordination
2. **CNCService** - Machine connection lifecycle, REST API client, WebSocket for real-time state, EventBus integration
3. **QuoteService** - Cost calculation workflow, line item management, PDF generation coordination
4. **ToolService** - Tool database CRUD, feeds/speeds calculation (RPM/feedrate from material+tool+operation)
5. **MaterialService** - Wood species database, material property lookup
6. **SearchService** - Advanced search across models/projects/tools, query parsing, async execution
7. **WatchService** - Filesystem monitoring on watch folders, file event filtering, import queue integration
8. **EventBus** - Cross-subsystem event publication/subscription with weak references to prevent leaks

**Thread model:**
- Main thread: UI rendering (ImGui) + event processing (SDL) + synchronous database operations
- Import workers: 1-2 threads with pooled database connections
- CNC WebSocket thread: Async I/O for real-time state
- Watch folder thread: Filesystem monitoring + queue to main thread
- Search workers: Index building, query execution with pooled connections

**Key architectural constraint:** ImGui is NOT thread-safe - all ImGui calls must happen on main thread. Workers post results to thread-safe queue, main thread processes during frame update.

### Critical Pitfalls

The pitfalls research identified 7 critical failures that occur when adding threading and real-time features to existing codebases. These must be prevented in Phase 1 foundation work.

1. **SQLite Single Connection Shared Across Threads** - Default "serialized" mode causes SQLITE_BUSY errors and timeout failures when multiple threads attempt to write simultaneously. One thread holding a write transaction blocks all others. **Prevention:** Enable WAL mode immediately (writers and readers operate simultaneously), use one connection per thread or queue-based architecture, never pass connection pointers across threads.

2. **God Class Becomes Threading Nightmare** - 1,071-line Application.cpp managing everything must now coordinate thread synchronization for all concerns simultaneously. Every field becomes a potential race condition. "Just add a mutex" creates massive contention and deadlock opportunities. **Prevention:** Refactor BEFORE adding threading - extract concerns into focused classes (DatabaseManager, MachineController, DocumentState) with clear ownership boundaries, use message passing over shared state.

3. **ImGui Single-Context Threading Violations** - ImGui is NOT thread-safe. All ImGui calls must happen on same thread where context was created. Background workers that try to update UI state cause crashes, corruption, or silent desynchronization. **Prevention:** Main thread owns all ImGui calls (zero exceptions), workers post results to thread-safe queue, main thread processes queue during frame update, never pass ImGui IDs or pointers to workers.

4. **WebSocket Thread vs Main Event Loop Mismatch** - WebSocket libraries run I/O event loop that blocks the calling thread. Running on main thread blocks SDL event processing and rendering. Running on background thread is correct, BUT callbacks execute on that background thread and cannot access shared state or call ImGui. **Prevention:** Dedicated WebSocket thread with event loop, callbacks only post to thread-safe queue, main thread polls queue during frame update, graceful shutdown protocol (send close frame, wait for callback, join thread).

5. **Machine State Synchronization Race Conditions** - CNC machine sends rapid updates (position, feed rate, status) at 10-100Hz. Application receives on WebSocket thread, UI renders at 60fps on main thread, user initiates commands on main thread. Without synchronization, UI shows stale position, commands get lost, or duplicate commands sent. **Prevention:** State versioning with monotonic version numbers, lock-protected machine state object (hold lock only for copy, not during rendering), command acknowledgment protocol with unique IDs and timeouts, handle out-of-order messages.

6. **Command Pattern Memory Explosion** - Storing entire object snapshots (e.g., full mesh data) for undo causes memory to balloon. A project with 50 edits × 10MB mesh each = 500MB of undo history. **Prevention:** Store deltas not snapshots (MoveModelCommand stores old/new position, not entire mesh), limit undo stack depth (50-100 commands), collapse consecutive commands (100 micro-moves → single command), use Memento pattern for complex changes with shared data structures.

7. **SDL Event Loop + Background Worker Coordination** - SDL event loop runs on main thread. Background workers need to trigger UI updates when complete but cannot call SDL or ImGui functions. Workers that SDL_PushEvent custom events cause subtle timing bugs - events arrive at unpredictable times, queue can fill up, custom event data lifetime is unclear. **Prevention:** Prefer thread-safe queue over SDL events, if using SDL events allocate with new/free in handler, shutdown protocol sets atomic flag and drains queue, limit event posting rate (batch updates).

**Pitfall-to-phase mapping:** Pitfalls 1, 2, 3, 7 must be addressed in Phase 1 before any new features. Skipping Phase 1 foundation work causes compounding technical debt as features are added.

## Key Findings Synthesis

### Cross-Cutting Insights

**Threading is the fundamental challenge:** All three research documents (STACK, ARCHITECTURE, PITFALLS) converge on threading as the primary technical risk. The existing single-threaded architecture must evolve to support:
- Background file imports (already planned with ImportQueue, but needs connection pooling)
- Real-time WebSocket updates from CNC machine (requires dedicated thread)
- Filesystem watching for auto-import (background monitoring thread)
- PDF generation without blocking UI (background worker)
- Advanced search indexing (background workers)

**The god class is the architectural bottleneck:** ARCHITECTURE research identifies 1,071-line Application.cpp as the central coupling point that will become a threading nightmare if not refactored first. PITFALLS research confirms this with "God Class Becomes Threading Nightmare" as Pitfall 2. The solution (SubsystemCoordinator pattern) must be implemented before adding features.

**Database access is the hidden coupling:** STACK research doesn't flag database as a concern, but PITFALLS research identifies "SQLite Single Connection Shared Across Threads" as Pitfall 1 (critical). ARCHITECTURE research provides the solution: WAL mode + ConnectionPool + per-thread connections. This is invisible to features but foundational.

**Feature priorities drive phase order:** FEATURES research shows tool/material databases are foundational (referenced by cut list, costing, feeds/speeds calculator). ARCHITECTURE research confirms these should be "Phase 4: Domain Services" after foundation work. PITFALLS research adds no specific pitfalls for these databases (they're straightforward CRUD), confirming low risk once foundation is solid.

**CNC integration is high-value but high-risk:** FEATURES research positions CNC machine control as a competitive differentiator. STACK research identifies mature libraries (cpp-httplib, IXWebSocket). But PITFALLS research identifies 3 critical pitfalls (WebSocket threading, machine state synchronization, SDL event coordination) that must be solved first. Phase order: Foundation → Domain Services → CNC Integration.

### Dependency Graph Across Research

```
Foundation (Phase 1)
├─ Thread Safety (PITFALLS 1,2,3,7)
│  ├─ SQLite WAL + ConnectionPool (ARCHITECTURE Pattern 4)
│  ├─ EventBus (ARCHITECTURE Pattern 3)
│  ├─ SubsystemCoordinator (ARCHITECTURE Pattern 1)
│  └─ Threading contracts documented
│
├─ Basic Infrastructure
   ├─ CommandManager for undo/redo (ARCHITECTURE Pattern 2, PITFALLS 6)
   └─ Main thread queue for worker communication

Domain Databases (Phase 2)
├─ MaterialService + MaterialRepository (FEATURES: wood species database)
├─ ToolService + ToolRepository (FEATURES: tool database, feeds/speeds)
│  └─ Uses: ExprTk for calculator (STACK)
└─ No critical pitfalls (standard CRUD)

Business Workflow (Phase 3)
├─ CustomerService + CustomerRepository (FEATURES: customer management)
├─ QuoteService + QuoteRepository (FEATURES: PDF quotes, job costing)
│  ├─ Uses: libHaru for PDF generation (STACK)
│  └─ Pitfall: PDF generation blocking UI (PITFALLS Performance Traps)
└─ Addresses: table-stakes business features

Enhanced Cut List (Phase 4)
├─ Extends existing 2D optimizer (already in codebase)
├─ Uses: MaterialService for grain direction
├─ Adds: Edge banding tracking, 1D optimization
└─ FEATURES: high-value enhancement to existing capability

CNC Integration (Phase 5)
├─ CNCService with REST + WebSocket (STACK: cpp-httplib, IXWebSocket)
├─ Depends on: EventBus, threading foundation
├─ Addresses: PITFALLS 4,5 (WebSocket threading, state sync)
└─ FEATURES: competitive differentiator

Background Processing (Phase 6)
├─ WatchService (STACK: efsw library)
├─ SearchService with FTS5
└─ Uses: ConnectionPool, EventBus (from Phase 1)
```

### Conflicts and Tensions

**No major conflicts identified across research areas.** The research is remarkably consistent:

- STACK and ARCHITECTURE agree on library choices and patterns
- FEATURES and ARCHITECTURE align on foundational databases before dependent features
- PITFALLS and ARCHITECTURE converge on "foundation first, features second" approach

**Minor tension: PDF generation** - STACK research rates libHaru confidence as MEDIUM (maintenance history shows gaps), but ARCHITECTURE doesn't flag this as problematic. Resolution: Abstract behind IPDFGenerator interface during Phase 3 to allow library swapping if needed.

**Minor tension: Search implementation** - FEATURES research suggests advanced search as P2 (should-have), ARCHITECTURE defers to Phase 6 (background processing). Resolution: SQLite FTS5 is already available (no new dependency), so defer to Phase 6 when advanced features are tackled.

## Implications for Roadmap

Based on research, the roadmap must follow strict dependency ordering. Attempting to build features before foundation will trigger the 7 critical pitfalls.

### Phase 1: Architectural Foundation & Thread Safety

**Rationale:** All three research documents converge on this as mandatory first step. PITFALLS research identifies 4 critical failures (SQLite threading, god class, ImGui violations, SDL coordination) that must be prevented before adding any threaded features. ARCHITECTURE research provides the solutions (ConnectionPool, EventBus, SubsystemCoordinator). Without this foundation, every subsequent phase will inherit race conditions, deadlocks, and architectural coupling.

**Delivers:**
- Application.cpp reduced from 1,071 → ~300 lines (thin coordinator)
- SubsystemCoordinator managing all subsystems
- SQLite WAL mode enabled + ConnectionPool with 2-4 worker connections
- EventBus for cross-subsystem communication (Observer Pattern)
- Main thread queue for worker→UI communication
- Threading contracts documented and enforced (ImGui main-thread-only)
- ImportQueue enhanced to use pooled connections

**Addresses:**
- PITFALLS 1, 2, 3, 7 (SQLite, god class, ImGui, SDL coordination)
- ARCHITECTURE Patterns 1, 3, 4 (coordinator, observer, connection pool)

**Avoids:**
- Race conditions when adding WebSocket/workers in later phases
- Architectural collapse from adding features to god class
- SQLITE_BUSY errors from shared connection
- ImGui crashes from background thread calls

**Research flag:** No additional research needed - patterns are well-established. Verify ThreadSanitizer clean during implementation.

### Phase 2: Command Pattern & Undo Infrastructure

**Rationale:** ARCHITECTURE research positions CommandManager as foundational for all future features. PITFALLS research identifies memory explosion risk (Pitfall 6) that must be designed correctly from the start. Building undo infrastructure before domain features means every feature gets undo capability from day one. Command pattern requires discipline (all operations go through commands) - easier to establish early than retrofit.

**Delivers:**
- CommandManager with two-stack undo/redo
- Command interface with execute/undo/redo methods
- Undo stack depth limit (50-100 commands)
- Command compression for consecutive operations
- Keyboard shortcuts (Ctrl+Z, Ctrl+Shift+Z)
- Example commands for existing operations (delete model, rename project)

**Addresses:**
- PITFALLS 6 (command pattern memory explosion)
- ARCHITECTURE Pattern 2 (command pattern)
- FEATURES: Undo/redo system (P2 differentiator)

**Uses:**
- EventBus (from Phase 1) for CommandExecuted notifications

**Avoids:**
- Memory bloat from storing full object snapshots
- Inconsistent undo behavior across features
- Retrofitting undo to existing features later (much harder)

**Research flag:** No additional research needed - pattern is well-documented. Test with heap profiler during implementation.

### Phase 3: Domain Databases (Tools & Materials)

**Rationale:** FEATURES research identifies wood species database and tool database as "foundational" - referenced by multiple downstream features (cut list optimization, feeds/speeds calculator, job costing). ARCHITECTURE research confirms these as "Phase 4: Domain Services" (after foundation phases 1-3). These are straightforward CRUD operations with business logic in service layer. No critical pitfalls (PITFALLS research mentions none), confirming low risk.

**Delivers:**
- MaterialService + MaterialRepository
- Wood species database schema (density, grain characteristics, cost/board-foot, common uses)
- CRUD operations for material management
- ToolService + ToolRepository
- Tool database schema (diameter, flutes, RPM ranges, feed rates, chip loads)
- Feeds & speeds calculator using ExprTk for formula evaluation
- Material-specific cutting parameters
- ToolPanel + MaterialPanel UI with undo/redo integration

**Addresses:**
- FEATURES: Wood species database (P1 table stakes)
- FEATURES: Tool database with feeds/speeds (P1 table stakes)
- ARCHITECTURE: ToolService, MaterialService (service layer pattern)

**Uses:**
- ExprTk (from STACK) for calculator formula engine
- CommandManager (from Phase 2) for undo/redo
- ConnectionPool (from Phase 1) for database access

**Implements:**
- ARCHITECTURE Pattern 5 (service layer for business logic)

**Research flag:** No additional research needed - CRUD operations are standard. May need domain expert review for feeds/speeds formulas.

### Phase 4: Business Workflow (Customer, Quotes, Job Costing)

**Rationale:** FEATURES research identifies customer management, PDF quotes, and job costing as "table stakes" - users expect these for professional credibility. ARCHITECTURE research doesn't flag these as architecturally complex (standard service layer pattern). PITFALLS research warns "PDF generation blocking UI" (Performance Traps) - must run in background worker thread with progress indicator.

**Delivers:**
- CustomerService + CustomerRepository
- Customer management (contact info, job history, quote tracking)
- QuoteService + QuoteRepository
- Cost calculation workflow (material + labor + overhead)
- Line item management with itemized costs
- PDF quote/estimate generation using libHaru
- Professional templates with customizable branding
- Terms, conditions, expiration dates
- Background PDF generation with progress indicator
- CustomerPanel + QuotePanel UI with undo/redo

**Addresses:**
- FEATURES: Customer management (P1 table stakes)
- FEATURES: PDF quote generation (P1 table stakes)
- FEATURES: Job costing & pricing (P1 table stakes)
- PITFALLS: PDF generation blocking UI (Performance Traps)

**Uses:**
- libHaru (from STACK) for PDF generation
- nlohmann/json (from STACK) if quote data exported
- MaterialService (from Phase 3) for material costs
- CommandManager (from Phase 2) for undo/redo
- ConnectionPool (from Phase 1) for database access

**Implements:**
- ARCHITECTURE Pattern 5 (service layer for business logic)

**Research flag:** Test libHaru stability during implementation - STACK research rated confidence as MEDIUM. Consider abstracting behind IPDFGenerator interface to allow library swapping if needed.

### Phase 5: Enhanced Cut List with Grain & Edge Banding

**Rationale:** FEATURES research positions enhanced cut list as "P2 - leverages existing optimizer" - builds on existing 2D bin-packing and guillotine code already in codebase. ARCHITECTURE research confirms this as incremental enhancement (no new subsystems). Dependencies: Requires MaterialService (Phase 3) for grain direction and wood species properties.

**Delivers:**
- Grain direction tracking in cut list
- Grain direction optimization (minimize waste while respecting aesthetics)
- Edge banding specification (species, color, which edges)
- Auto-calculate linear footage for edge banding
- 1D optimization for dimensional lumber (extends existing 2D optimizer)
- Enhanced cut list UI with grain visualization

**Addresses:**
- FEATURES: Enhanced cut list (P2 differentiator)
- Significant material cost savings through better optimization

**Uses:**
- MaterialService (from Phase 3) for wood species properties and grain characteristics
- Existing 2D optimizer (already in codebase)
- CommandManager (from Phase 2) for undo/redo

**Avoids:**
- No critical pitfalls (PITFALLS research mentions none)

**Research flag:** May need domain expert review for grain matching algorithms (book matching, sequential matching). Standard optimization patterns apply.

### Phase 6: CNC Machine Control Integration

**Rationale:** FEATURES research identifies CNC machine control as "competitive differentiator" but complex - should defer until core features solid. ARCHITECTURE research confirms as "Phase 5: Advanced Features" requiring EventBus and threading foundation. PITFALLS research identifies 2 critical failures (Pitfall 4: WebSocket threading, Pitfall 5: machine state sync) that require Phase 1 foundation to solve.

**Delivers:**
- CNCService with machine connection lifecycle
- REST API client using cpp-httplib for control commands (start job, pause, resume, MDI)
- WebSocket client using IXWebSocket for real-time state monitoring
- State cache + EventBus integration for UI notifications
- Command acknowledgment protocol with timeouts
- Connection status indicator (green/yellow/red)
- Reconnection logic with exponential backoff
- CNCPanel UI for machine monitoring and control

**Addresses:**
- FEATURES: CNC machine control integration (P2 differentiator)
- PITFALLS 4 (WebSocket threading)
- PITFALLS 5 (machine state synchronization race conditions)

**Uses:**
- cpp-httplib (from STACK) for REST API
- IXWebSocket (from STACK) for WebSocket
- nlohmann/json (from STACK) for message parsing
- EventBus (from Phase 1) for state notifications
- Main thread queue (from Phase 1) for WebSocket callbacks

**Implements:**
- ARCHITECTURE: CNCService subsystem
- ARCHITECTURE Pattern 3 (Observer for real-time state)
- ARCHITECTURE Pattern 7 (async REST client)

**Avoids:**
- Blocking main thread on WebSocket connect/disconnect
- Race conditions from shared machine state
- ImGui crashes from WebSocket callbacks
- Lost commands from missing acknowledgment

**Research flag:** CRITICAL - ncSender protocol documentation needed. STACK research notes "ncSender is not a widely documented standard" with LOW confidence on specifics. During phase planning, research ncSender's exact WebSocket message format, authentication requirements, message rate, and connection loss handling. May need vendor documentation.

### Phase 7: Background Processing (Watch Folders & Advanced Search)

**Rationale:** FEATURES research positions watch folder auto-import as "P2 differentiator" (low complexity) and advanced search as "P2 should-have" for large libraries. ARCHITECTURE research defers both to "Phase 6: Background Processing" as they require threading foundation (Phase 1) but don't have feature dependencies. PITFALLS research doesn't flag specific failures for these (general threading pitfalls already solved in Phase 1).

**Delivers:**
- WatchService using efsw for filesystem monitoring
- Background thread watches configured folders for new files
- File event filtering (only importable formats: .stl, .obj, .3mf, .gcode)
- Debouncing logic (wait for file to be fully written)
- Integration with ImportQueue (from Phase 1)
- Settings UI for managing watch folders
- SearchService with SQLite FTS5 full-text search
- Search indexing across models, projects, customers, tools
- Query parsing for advanced syntax (field:value, AND/OR/NOT)
- Async search execution with progress
- SearchPanel UI with results ranking

**Addresses:**
- FEATURES: Watch folder auto-import (P2 differentiator)
- FEATURES: Advanced search (P2 should-have)

**Uses:**
- efsw (from STACK) for filesystem watching
- SQLite FTS5 (already available, no new dependency)
- ConnectionPool (from Phase 1) for worker thread database access
- EventBus (from Phase 1) for notifications
- Main thread queue (from Phase 1) for watch thread communication

**Implements:**
- ARCHITECTURE: WatchService subsystem
- ARCHITECTURE: SearchService subsystem
- ARCHITECTURE Pattern 6 (filesystem watcher with event queue)

**Avoids:**
- File system polling (inefficient)
- Race conditions when file still being written
- Overwhelming main thread with events

**Research flag:** No additional research needed for watch folders (efsw is well-documented). Search query syntax may need UX design during phase planning.

### Phase 8: Advanced Features (Material Inventory, Time Tracking, PBR Rendering)

**Rationale:** FEATURES research identifies material inventory tracking and time tracking as "P2" features that enable better costing. PBR rendering is "P2" for professional client presentations. These are standard features with no architectural complexity (ARCHITECTURE research doesn't mention) and no critical pitfalls (PITFALLS research mentions none).

**Delivers:**
- InventoryService + InventoryRepository
- Material inventory tracking (sheet goods: species, thickness, quantity)
- Dimensional lumber tracking
- Low-stock alerts and reorder suggestions
- InventoryPanel UI
- TimeTrackingService + TimeTrackingRepository
- Start/stop timers per job/task
- Actual vs. estimated time reporting
- TimeTrackingPanel UI
- PBR material rendering for viewport (realistic wood grain visualization)
- Texture loading and shader enhancements

**Addresses:**
- FEATURES: Material inventory tracking (P2)
- FEATURES: Time tracking (P2)
- FEATURES: PBR material rendering (P2)

**Uses:**
- MaterialService (from Phase 3) for wood species
- QuoteService (from Phase 4) for cost integration
- Existing 3D viewport (already in codebase)
- CommandManager (from Phase 2) for undo/redo

**Research flag:** PBR rendering may need graphics programming research during phase planning (shader implementation, texture formats). Inventory and time tracking are standard CRUD.

### Phase Ordering Rationale

**Foundation-first is non-negotiable:** PITFALLS research identifies 7 critical failures, 4 of which (1,2,3,7) must be solved before any threaded features. ARCHITECTURE research provides solutions (ConnectionPool, EventBus, SubsystemCoordinator) that must be built first. Attempting to add CNC control (Phase 6) or watch folders (Phase 7) before Phase 1 will trigger race conditions, deadlocks, and ImGui crashes.

**Dependencies drive sequence:**
- Tool/material databases (Phase 3) before enhanced cut list (Phase 5) - cut list needs grain direction from materials
- Tool/material databases (Phase 3) before job costing (Phase 4) - costing needs material costs
- EventBus (Phase 1) before CNC control (Phase 6) - machine state updates require pub/sub
- Command infrastructure (Phase 2) before all features - undo/redo is easier to build in than retrofit

**Risk-based deferral:**
- CNC control (Phase 6) deferred until Phase 1 foundation solid - PITFALLS research identifies WebSocket threading and state sync as critical, requires robust threading foundation
- Background processing (Phase 7) deferred - watch folders and search are lower priority than business workflow (Phase 4)
- Advanced features (Phase 8) lowest priority - nice-to-have enhancements after core functionality delivered

**Grouping by architectural pattern:**
- Phase 1: Infrastructure (threading, events, database)
- Phase 2: Cross-cutting concern (undo/redo)
- Phase 3: Domain databases (CRUD with business logic)
- Phase 4: Business workflow (service layer pattern)
- Phase 5: Algorithm enhancement (optimize existing code)
- Phase 6: Real-time integration (async I/O, state sync)
- Phase 7: Background processing (workers with queues)
- Phase 8: Polish and enhancements

### Research Flags

**Phases needing deeper research during planning:**

- **Phase 6 (CNC control):** CRITICAL - ncSender protocol documentation required. STACK research notes LOW confidence on ncSender specifics (message format, authentication, rates). Must research ncSender WebSocket API, authentication requirements, message schemas, connection loss behavior, and error codes before implementation. Consider reaching out to ncSender vendor for documentation.

- **Phase 7 (Search query syntax):** MEDIUM - UX design needed for advanced search syntax. ARCHITECTURE research suggests simple recursive descent parser or ExprTk for boolean expressions. Need to define user-facing query syntax (field:value, comparisons, boolean logic) before implementation.

- **Phase 8 (PBR rendering):** MEDIUM - Graphics programming research needed if team lacks OpenGL shader experience. Existing codebase has 3D viewport with OpenGL 3.3, but PBR requires physically-based shading (metallic/roughness workflow), texture sampling, and IBL (image-based lighting). May need shader development research.

**Phases with standard patterns (skip research-phase):**

- **Phase 1 (Foundation):** Well-documented patterns - ARCHITECTURE research provides complete implementation guidance for ConnectionPool, EventBus, SubsystemCoordinator. PITFALLS research identifies exact failures to avoid. No additional research needed beyond codebase review.

- **Phase 2 (Undo/Redo):** Command Pattern is textbook - ARCHITECTURE research provides implementation, PITFALLS research identifies memory explosion prevention. No additional research needed.

- **Phase 3 (Databases):** Standard CRUD operations - service layer pattern is well-established. Feeds/speeds calculator formulas may need domain expert validation, but no research needed for implementation approach.

- **Phase 4 (Business workflow):** Standard service layer pattern - customer management and job costing are straightforward. PDF generation uses libHaru (STACK research confirmed library choice). No additional research needed.

- **Phase 5 (Enhanced cut list):** Algorithm enhancement - extends existing 2D optimizer. Grain matching algorithms may need domain expert review, but implementation pattern is clear.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All core libraries verified from official GitHub releases (cpp-httplib v0.31.0, IXWebSocket v11.4.6, nlohmann/json, ExprTk). Active maintenance confirmed for 2025-2026. License compliance verified (all permissive). Size budget confirmed (~1.4MB new libraries, ~5MB total). Lower confidence (MEDIUM) on libHaru due to maintenance gaps, but mitigated by interface abstraction. |
| Features | MEDIUM | Feature expectations validated across multiple commercial products (Microvellum, CabinetShop, VCarve). Table-stakes vs differentiators clearly identified. Competitive positioning sound. No official domain standards available (all WebSearch results), hence MEDIUM not HIGH. Feature dependencies validated across multiple sources. |
| Architecture | HIGH | Patterns are industry-standard and well-documented (Command, Observer, Service Layer, Connection Pool). Implementation guidance detailed and specific to C++17/ImGui/SDL2/SQLite context. Thread safety approaches validated by multiple authoritative sources (SQLite official docs, ImGui maintainer GitHub issues). SubsystemCoordinator pattern adapted from established practices. |
| Pitfalls | MEDIUM-HIGH | Critical pitfalls (SQLite threading, ImGui violations, WebSocket threading) validated by official documentation and multiple practitioner reports. Recovery strategies confirmed across sources. Confidence is HIGH for pitfalls with official documentation (SQLite, ImGui), MEDIUM for pitfalls based on practitioner experience (god class refactoring, command pattern memory). All pitfalls have clear prevention strategies. |

**Overall confidence:** MEDIUM-HIGH

The research provides a solid foundation for roadmap creation. Stack and architecture decisions are well-supported by authoritative sources. Feature expectations are validated across industry examples but lack official standards (inherent to domain). Pitfalls are thoroughly documented with prevention strategies. The main uncertainty is ncSender-specific protocol details, which requires phase-specific research before Phase 6.

### Gaps to Address

**ncSender protocol specifics (CRITICAL gap):**
- STACK research identifies ncSender as "not a widely documented standard" with LOW confidence
- Unknown: Message format, authentication requirements, connection loss behavior, error codes
- Impact: Phase 6 (CNC control) cannot be planned in detail without this information
- **Resolution:** During Phase 6 planning, use /gsd:research-phase to investigate ncSender API. If documentation unavailable, consider contacting vendor or reverse-engineering from packet capture. Fallback: Abstract behind ICNCController interface to support multiple controller types.

**Domain expert validation for formulas (MEDIUM gap):**
- Feeds/speeds calculator (Phase 3) requires correct formulas for RPM, feed rate, chip loads
- STACK research identifies ExprTk for formula evaluation, but formulas themselves need domain validation
- **Resolution:** During Phase 3 planning, consult CNC woodworking expert or reference materials (Tooling Guide, manufacturer recommendations). Formula correctness is critical for safe machine operation.

**Grain matching algorithm design (LOW gap):**
- Enhanced cut list (Phase 5) includes grain direction optimization
- FEATURES research mentions book matching and sequential matching but doesn't detail algorithms
- **Resolution:** During Phase 5 planning, research wood grain matching techniques. This is aesthetic optimization (not safety-critical), so can iterate based on user feedback.

**PBR rendering implementation (LOW gap):**
- Phase 8 includes PBR material rendering for wood grain visualization
- FEATURES research identifies as "P2" feature but ARCHITECTURE/STACK research don't address implementation
- **Resolution:** During Phase 8 planning, research PBR shader implementation for OpenGL 3.3. Existing codebase has 3D viewport, so extend with PBR shaders. Not critical for MVP - can defer to v2 if needed.

**libHaru stability (LOW gap):**
- STACK research rates libHaru confidence as MEDIUM due to maintenance gaps
- PDF generation is table-stakes feature (Phase 4), so library stability matters
- **Resolution:** During Phase 4 implementation, abstract PDF generation behind IPDFGenerator interface. Test libHaru thoroughly with real invoices. If instability found, can swap to PDFWriter or other library without refactoring.

## Sources

### Primary (HIGH confidence)

**Stack Research:**
- [cpp-httplib GitHub releases](https://github.com/yhirose/cpp-httplib/releases) - Version v0.31.0 confirmed Feb 2025
- [IXWebSocket GitHub](https://github.com/machinezone/IXWebSocket) - Version v11.4.6 confirmed May 2025
- [nlohmann/json GitHub](https://github.com/nlohmann/json) - Industry standard, millions of projects
- [ExprTk Official](http://www.partow.net/programming/exprtk/) - Documentation and MIT license verified
- [{fmt} GitHub](https://github.com/fmtlib/fmt) - C++20 std::format basis, v11.x stable

**Architecture Research:**
- [SQLite Official: Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html) - Thread safety modes
- [Qt Undo Framework](https://doc.qt.io/qt-6/qundo.html) - Command pattern reference (concept only, not using Qt)
- [Implementing Undo/Redo with Command Pattern](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/) - Implementation guide
- [Service Layer Pattern](https://martinfowler.com/eaaCatalog/serviceLayer.html) - Martin Fowler's catalog

**Pitfalls Research:**
- [ImGui Issue #6597, #221, #9136](https://github.com/ocornut/imgui) - Threading violations confirmed by maintainer
- [Multi-threaded SQLite without OperationalErrors](https://charlesleifer.com/blog/multi-threaded-sqlite-without-the-operationalerrors/) - WAL mode best practices
- [WebSocket++ Utility Client Tutorial](https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html) - Threading patterns

### Secondary (MEDIUM confidence)

**Feature Research:**
- [Microvellum Manufacturing Software](https://www.microvellum.com/solutions/manufacturing) - High-end competitor feature analysis
- [CabinetShop Maestro](https://www.cabinetshopsoftware.com/) - Mid-tier competitor
- [VCarve/Aspire](https://www.vectric.com/) - CAM-focused competitor
- [CNC Feeds and Speeds Calculator](https://www.cncoptimization.com/calculators/feeds-speeds/) - Formula validation
- [Wood Database](https://www.wood-database.com/) - Material properties
- [5 Best OEE Monitoring Software](https://www.fabrico.io/blog/best-oee-software-cnc-machine-shops/) - Production monitoring patterns

**Architecture Research:**
- [Observer Pattern in C++](https://medium.com/@chetanp.verma98/c-observer-design-pattern-a-comprehensive-deep-dive-50101a2a8ee1) - EventBus pattern
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) - C++17 thread pool library (v5.1.0 Jan 2026)
- [efsw Filesystem Watcher](https://github.com/SpartanJ/efsw) - Cross-platform file monitoring

**Pitfalls Research:**
- [Multithreading: Race Conditions and Deadlocks in C++](https://dev.to/shreyosghosh/multithreading-handling-race-conditions-and-deadlocks-in-c-4ae4) - General threading advice
- [WebSocket architecture best practices](https://ably.com/topic/websocket-architecture-best-practices) - State synchronization patterns
- [Command Pattern (Game Programming Patterns)](https://gameprogrammingpatterns.com/command.html) - Memory optimization

### Tertiary (LOW confidence)

**Stack Research:**
- [libHaru GitHub](https://github.com/libharu/libharu) - v2.4.5 March 2025, but maintenance gaps noted
- [CNCjs Controller API](https://github.com/cncjs/cncjs) - WebSocket + REST example (not ncSender specific)
- [Buildbotics CNC API](https://buildbotics.com/introduction-to-the-buildbotics-cnc-controller-api/) - Industry patterns (not ncSender specific)

**Feature Research:**
- Various WebSearch results on CNC shop management features - no authoritative domain standards available

**Pitfalls Research:**
- [SDL2 common mistakes](https://nullprogram.com/blog/2023/01/08/) - Blog post, not official docs

---
*Research completed: 2026-02-08*
*Ready for roadmap: yes*
