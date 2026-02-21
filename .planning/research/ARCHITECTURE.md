# Architecture Research: CNC Workshop Management Subsystem Integration

**Domain:** Desktop application subsystem integration (C++17)
**Researched:** 2026-02-08
**Confidence:** HIGH

## Executive Summary

Integrating 8 major subsystems (CNC machine control, cost estimation, tool database, material database, undo/redo, advanced search, watch folders, thread safety) into an existing layered C++17 desktop application requires decomposing the 1,071-line Application god class and establishing clear architectural boundaries.

The recommended approach uses **Application Coordinator Pattern** to decompose the god class into specialized subsystem coordinators, **Command Pattern** for undo/redo with cross-subsystem support, **Observer Pattern** for real-time WebSocket state propagation, **Connection Pool Pattern** for thread-safe database access, and **Service Layer Pattern** to encapsulate business logic away from UI concerns.

The key architectural decision is to introduce a **Subsystems Layer** between Application and Core, where each major subsystem gets its own coordinator/service managing lifecycle, state, and integration with existing layers.

## Current Architecture Analysis

### Existing Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         Application.cpp (1,071 lines)                 │   │
│  │  • SDL/OpenGL/ImGui initialization                    │   │
│  │  • Event loop orchestration                           │   │
│  │  • Menu/panel rendering                               │   │
│  │  • All callbacks (import, export, project ops)        │   │
│  │  • Config watching & restart logic                    │   │
│  │  • Owns all core systems & UI panels                  │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                        Workspace Layer                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Workspace (focused object pattern)                   │   │
│  │  • Active project tracking                            │   │
│  │  • Selection state management                         │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                          UI Layer                            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Viewport │ │ Library  │ │Properties│ │ Project  │       │
│  │  Panel   │ │  Panel   │ │  Panel   │ │  Panel   │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  (ImGui immediate-mode rendering)                            │
├─────────────────────────────────────────────────────────────┤
│                    Manager/Service Layer                     │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │ LibraryManager │  │ ProjectManager │  │ ImportQueue  │  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                         Core Layer                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   Loaders   │  │   Optimizer │  │   GCode     │         │
│  │  (Factory)  │  │  (Bin Pack) │  │  (Parser)   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│                     Repository Layer                         │
│  ┌──────────────────┐  ┌──────────────────┐                 │
│  │ ModelRepository  │  │ ProjectRepository│                 │
│  └──────────────────┘  └──────────────────┘                 │
├─────────────────────────────────────────────────────────────┤
│                        Data Layer                            │
│  ┌─────────────────────────────────────────────────────┐    │
│  │          Database (SQLite with WAL mode)             │    │
│  │          Statement (RAII prepared statements)        │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘

Thread Model:
• Main thread: UI rendering (ImGui) + event processing
• Background worker: ImportQueue (single worker thread)
```

### Problems with Current Architecture

1. **God Class Syndrome:** Application.cpp owns everything (core systems, UI panels, dialogs, config watching) and orchestrates all interactions
2. **No Subsystem Boundaries:** No clear place to add complex subsystems like CNC control or undo/redo
3. **Thread Safety Gaps:** Single SQLite connection, no connection pooling for multi-threaded access
4. **Tight Coupling:** Application directly manages panel lifecycle and inter-panel communication
5. **No Real-Time State:** Architecture assumes synchronous operations, no WebSocket integration point

## Recommended Target Architecture

### Decomposed Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Application (Thin Coordinator - ~300 lines target)   │   │
│  │  • SDL/OpenGL/ImGui initialization                    │   │
│  │  • Event loop delegation                              │   │
│  │  • High-level menu coordination                       │   │
│  │  • Subsystem lifecycle orchestration                  │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                     Subsystems Layer (NEW)                   │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │ CommandManager │  │  CNCService    │  │ QuoteService │  │
│  │ (Undo/Redo)    │  │ (REST+WS)      │  │ (PDF Gen)    │  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │   ToolService  │  │ MaterialService│  │ SearchService│  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
│  ┌────────────────┐  ┌────────────────┐                     │
│  │  WatchService  │  │  EventBus      │                     │
│  │ (Filesystem)   │  │ (Observer)     │                     │
│  └────────────────┘  └────────────────┘                     │
├─────────────────────────────────────────────────────────────┤
│                        Workspace Layer                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Workspace (Enhanced with event subscription)         │   │
│  │  • Active project + selection state                   │   │
│  │  • Subscribes to EventBus for cross-subsystem events  │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                          UI Layer                            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Existing │ │   CNC    │ │  Quote   │ │   Tool   │       │
│  │  Panels  │ │  Panel   │ │  Panel   │ │  Panel   │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  • Panels call subsystem services                            │
│  • Panels subscribe to EventBus for state updates            │
├─────────────────────────────────────────────────────────────┤
│                    Manager/Service Layer                     │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │ Existing       │  │   Enhanced     │  │ New Workers  │  │
│  │ Managers       │  │  ImportQueue   │  │ (ThreadPool) │  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                         Core Layer                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ Existing    │  │  Commands   │  │ Calculator  │         │
│  │ Components  │  │ (for undo)  │  │ (feeds/sp.) │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│                     Repository Layer                         │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────┐  │
│  │ Existing Repos   │  │   ToolRepository │  │ Material │  │
│  │ (Thread-aware)   │  │ QuoteRepository  │  │   Repo   │  │
│  └──────────────────┘  └──────────────────┘  └──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                        Data Layer                            │
│  ┌─────────────────────────────────────────────────────┐    │
│  │    Database (Enhanced with Connection Pool)          │    │
│  │    • Main connection (UI thread)                     │    │
│  │    • Pool of 2-4 connections (worker threads)        │    │
│  │    • Thread-safe connection acquisition              │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘

Enhanced Thread Model:
• Main thread: UI + render + synchronous DB operations
• Import workers: 1-2 threads with pooled DB connections
• CNC WebSocket thread: Async I/O for real-time state
• Watch folder thread: Filesystem monitoring + queue to main thread
```

## Component Responsibilities

### New Subsystems Layer

| Component | Responsibility | Implementation |
|-----------|----------------|----------------|
| **CommandManager** | Undo/redo stack management, command registration, cross-subsystem command coordination | Two-stack pattern (undo/redo), command interface with execute/undo/redo, command compression for text edits |
| **CNCService** | Machine connection lifecycle, REST API client for control commands, WebSocket client for real-time state, state cache + observer notifications | Async HTTP client (cpprestsdk or restclient-cpp), WebSocket client (websocketpp), EventBus integration for state broadcasts |
| **QuoteService** | Cost calculation workflow, line item management, PDF generation coordination | Business logic layer, delegates to QuoteRepository for persistence, uses external PDF library |
| **ToolService** | Tool database CRUD, feeds/speeds calculation, tool recommendations | Service wraps ToolRepository, calculator logic for RPM/feedrate from material+tool+operation |
| **MaterialService** | Wood species database, material property lookup | Service wraps MaterialRepository |
| **SearchService** | Advanced search across models/projects/tools, search index management, query parsing | Full-text search indexing (SQLite FTS5 or in-memory index), async search execution |
| **WatchService** | Filesystem monitoring on watch folders, file event filtering, import queue integration | Platform-specific watchers (inotify/kqueue/ReadDirectoryChangesW) or cross-platform library (efsw), posts import tasks to main thread |
| **EventBus** | Cross-subsystem event publication/subscription, type-safe event routing | Observer pattern implementation, event type registry, subscription management with weak references to prevent leaks |

### Data Flow Patterns

#### Request Flow (User Action → Subsystem → Repository → Database)

```
User clicks "Add Tool"
    ↓
ToolPanel::onAddTool()
    ↓
ToolService::addTool(ToolData)
    ↓
CommandManager::execute(AddToolCommand)  ← Wrapped for undo
    ↓
ToolRepository::insert(Tool)
    ↓
Database::prepare() → Statement::execute()
    ↓
EventBus::publish(ToolAddedEvent)  ← Notify observers
    ↓
[All subscribers receive event]
    • ToolPanel refreshes list
    • SearchService updates index
```

#### Real-Time State Flow (CNC Machine → WebSocket → UI)

```
CNC Machine sends state update
    ↓
CNCService WebSocket callback (background thread)
    ↓
CNCService updates state cache (mutex-protected)
    ↓
EventBus::publish(MachineStateChanged) ← Thread-safe post to main thread
    ↓
Main thread processes events (next frame)
    ↓
CNCPanel::onMachineStateChanged()
    ↓
ImGui renders updated state (next frame)
```

#### Background Import Flow (Watch Folder → Queue → Worker → Database)

```
WatchService detects new file (watch thread)
    ↓
WatchService::onFileCreated() posts to main thread queue
    ↓
Main thread: ImportQueue::enqueue(path)
    ↓
Worker thread picks up task
    ↓
ImportTask executes:
    • LoaderFactory::createLoader()
    • Loader::load() → Mesh
    • Acquire DB connection from pool
    • ModelRepository::insert(Mesh)
    • Release DB connection
    ↓
Worker posts completion event to main thread
    ↓
Main thread: EventBus::publish(ModelImported)
    ↓
LibraryPanel::onModelImported() refreshes display
```

## Architectural Patterns

### Pattern 1: Application Coordinator Decomposition

**What:** Break Application god class into thin coordinator that delegates to subsystem services

**When to use:** When a single class exceeds 500-800 lines and manages multiple unrelated concerns

**Trade-offs:**
- **Pro:** Clear separation of concerns, easier testing, parallel development
- **Pro:** Subsystems become reusable (e.g., CommandManager can be unit tested independently)
- **Con:** More indirection, more files to navigate
- **Con:** Initial refactoring effort is significant

**Implementation approach:**

```cpp
// Before: Application owns everything
class Application {
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<ImportQueue> m_importQueue;
    // ... 20+ more members

    void onImportModel(); // 50 lines of logic
    void onExportModel(); // 40 lines of logic
    // ... 30+ callbacks
};

// After: Application delegates to subsystem coordinators
class Application {
    std::unique_ptr<SubsystemCoordinator> m_subsystems;

    void onImportModel() {
        m_subsystems->import()->startImport(path);
    }
};

class SubsystemCoordinator {
    std::unique_ptr<CommandManager> m_commands;
    std::unique_ptr<CNCService> m_cnc;
    std::unique_ptr<QuoteService> m_quotes;
    std::unique_ptr<ToolService> m_tools;
    std::unique_ptr<MaterialService> m_materials;
    std::unique_ptr<SearchService> m_search;
    std::unique_ptr<WatchService> m_watch;
    std::unique_ptr<EventBus> m_events;

public:
    // Subsystem accessors
    CommandManager* commands() { return m_commands.get(); }
    CNCService* cnc() { return m_cnc.get(); }
    // ...

    // Lifecycle
    void initialize(Database* db, Workspace* workspace);
    void shutdown();
};
```

### Pattern 2: Command Pattern for Undo/Redo

**What:** Encapsulate operations as command objects with execute(), undo(), redo() methods

**When to use:** When implementing undo/redo that spans multiple subsystems (tool edits, project changes, model operations)

**Trade-offs:**
- **Pro:** Cross-subsystem undo support, command history for debugging
- **Pro:** Command compression for repeated operations (e.g., dragging a slider)
- **Con:** Not every operation can be undone (e.g., file imports, external API calls)
- **Con:** Commands must store enough state to reverse operations (memory overhead)

**Implementation approach:**

```cpp
// Command interface
class Command {
public:
    virtual ~Command() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool redo() { return execute(); }
    virtual bool canCompress(const Command* other) const { return false; }
    virtual void compress(Command* other) {}
    virtual std::string description() const = 0;
};

// Example: AddToolCommand
class AddToolCommand : public Command {
    ToolService* m_service;
    Tool m_tool;
    int64_t m_insertedId = -1;

public:
    AddToolCommand(ToolService* svc, Tool tool)
        : m_service(svc), m_tool(std::move(tool)) {}

    bool execute() override {
        m_insertedId = m_service->insertTool(m_tool);
        return m_insertedId > 0;
    }

    bool undo() override {
        return m_service->deleteTool(m_insertedId);
    }

    std::string description() const override {
        return "Add tool: " + m_tool.name;
    }
};

// CommandManager with two-stack pattern
class CommandManager {
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;

public:
    bool execute(std::unique_ptr<Command> cmd) {
        if (cmd->execute()) {
            m_undoStack.push_back(std::move(cmd));
            m_redoStack.clear(); // Clear redo on new action
            return true;
        }
        return false;
    }

    bool undo() {
        if (m_undoStack.empty()) return false;
        auto cmd = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        if (cmd->undo()) {
            m_redoStack.push_back(std::move(cmd));
            return true;
        }
        return false;
    }

    bool redo() {
        if (m_redoStack.empty()) return false;
        auto cmd = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        if (cmd->redo()) {
            m_undoStack.push_back(std::move(cmd));
            return true;
        }
        return false;
    }
};
```

**Source:** [Command Pattern for Undo/Redo](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/), [Qt Undo Framework](https://doc.qt.io/qt-6/qundo.html)

### Pattern 3: Observer Pattern for Real-Time State (EventBus)

**What:** EventBus with type-safe event publication/subscription for cross-component communication

**When to use:** When real-time state changes (CNC machine updates, import completion) need to notify multiple UI panels without tight coupling

**Trade-offs:**
- **Pro:** Decouples publishers from subscribers, dynamic subscription
- **Pro:** Single broadcast reaches all interested parties
- **Con:** Potential memory leaks if observers aren't unsubscribed (use weak_ptr)
- **Con:** Harder to trace event flow in debugger

**Implementation approach:**

```cpp
// Event base class (for type safety)
class Event {
public:
    virtual ~Event() = default;
};

// Specific event types
class MachineStateChangedEvent : public Event {
public:
    std::string machineId;
    MachineState newState;
};

class ModelImportedEvent : public Event {
public:
    int64_t modelId;
    std::string filename;
};

// EventBus with type-safe subscriptions
class EventBus {
public:
    template<typename EventType>
    using EventHandler = std::function<void(const EventType&)>;

    template<typename EventType>
    void subscribe(std::weak_ptr<void> owner, EventHandler<EventType> handler) {
        auto& handlers = m_handlers[typeid(EventType)];
        handlers.push_back({owner, [handler](const Event& e) {
            handler(static_cast<const EventType&>(e));
        }});
    }

    template<typename EventType>
    void publish(const EventType& event) {
        auto it = m_handlers.find(typeid(EventType));
        if (it == m_handlers.end()) return;

        // Clean up dead subscriptions
        auto& handlers = it->second;
        handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
            [](auto& h) { return h.owner.expired(); }), handlers.end());

        // Notify active subscribers
        for (auto& h : handlers) {
            if (!h.owner.expired()) {
                h.callback(event);
            }
        }
    }

private:
    struct Subscription {
        std::weak_ptr<void> owner;
        std::function<void(const Event&)> callback;
    };

    std::unordered_map<std::type_index, std::vector<Subscription>> m_handlers;
};

// Usage in CNCService (publisher)
void CNCService::onWebSocketMessage(const std::string& json) {
    MachineState state = parseState(json);

    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_currentState = state;

    // Post to main thread (EventBus is not thread-safe)
    m_mainThreadQueue.enqueue([this, state]() {
        MachineStateChangedEvent event{m_machineId, state};
        m_eventBus->publish(event);
    });
}

// Usage in CNCPanel (subscriber)
void CNCPanel::initialize(EventBus* eventBus) {
    eventBus->subscribe<MachineStateChangedEvent>(
        shared_from_this(),
        [this](const MachineStateChangedEvent& e) {
            if (e.machineId == m_selectedMachine) {
                updateStateDisplay(e.newState);
            }
        }
    );
}
```

**Source:** [Observer Pattern in C++](https://medium.com/@chetanp.verma98/c-observer-design-pattern-a-comprehensive-deep-dive-50101a2a8ee1), [Event-Driven Architecture](https://www.geeksforgeeks.org/system-design/event-driven-architecture-system-design/)

### Pattern 4: Connection Pool for Thread-Safe Database Access

**What:** Maintain pool of 2-4 SQLite connections for worker threads, with main thread using dedicated connection

**When to use:** When background workers (import, search indexing) need database access without blocking UI thread

**Trade-offs:**
- **Pro:** Eliminates database lock contention between UI and workers
- **Pro:** SQLite's serialized mode allows concurrent reads from different connections
- **Con:** Each connection has memory overhead (cache, page buffers)
- **Con:** Must handle connection lifecycle carefully

**Implementation approach:**

```cpp
class ConnectionPool {
public:
    ConnectionPool(const std::string& dbPath, size_t poolSize = 3)
        : m_dbPath(dbPath) {
        for (size_t i = 0; i < poolSize; ++i) {
            auto conn = std::make_unique<Database>();
            if (conn->open(dbPath)) {
                m_connections.push(std::move(conn));
            }
        }
    }

    // RAII connection acquisition
    class ScopedConnection {
        ConnectionPool* m_pool;
        std::unique_ptr<Database> m_conn;

    public:
        ScopedConnection(ConnectionPool* pool) : m_pool(pool) {
            std::unique_lock<std::mutex> lock(pool->m_mutex);
            pool->m_condition.wait(lock, [pool] {
                return !pool->m_connections.empty();
            });

            m_conn = std::move(pool->m_connections.front());
            pool->m_connections.pop();
        }

        ~ScopedConnection() {
            if (m_conn) {
                std::lock_guard<std::mutex> lock(m_pool->m_mutex);
                m_pool->m_connections.push(std::move(m_conn));
                m_pool->m_condition.notify_one();
            }
        }

        Database* get() { return m_conn.get(); }
        Database* operator->() { return m_conn.get(); }
    };

    ScopedConnection acquire() {
        return ScopedConnection(this);
    }

private:
    std::string m_dbPath;
    std::queue<std::unique_ptr<Database>> m_connections;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

// Usage in worker thread
void ImportWorker::processTask(ImportTask task) {
    auto conn = m_connectionPool->acquire(); // Blocks if pool empty

    auto mesh = loadMesh(task.path);

    ModelRepository repo(conn.get());
    int64_t modelId = repo.insert(mesh);

    // Connection automatically returned to pool when conn goes out of scope
}
```

**Why not SQLite's built-in threading modes?**
- **Multi-threaded mode:** Requires one connection per thread but no synchronization between connections. Good for our use case.
- **Serialized mode:** SQLite serializes all access internally. Simpler but may bottleneck.
- **Recommendation:** Use multi-threaded mode with connection pool for maximum throughput.

**Source:** [SQLite Threading Modes](https://www.sqlite.org/threadsafe.html), [Libzdb Connection Pooling](https://www.tildeslash.com/libzdb/)

### Pattern 5: Service Layer for Business Logic

**What:** Introduce service classes (ToolService, QuoteService, etc.) that encapsulate business logic, keeping repositories pure data access

**When to use:** When domain logic (cost calculations, feeds/speeds) shouldn't live in UI or repositories

**Trade-offs:**
- **Pro:** Clean separation: UI → Service → Repository → Database
- **Pro:** Services are easily unit testable without UI
- **Con:** Additional layer of indirection

**Implementation approach:**

```cpp
// ToolRepository: Pure data access
class ToolRepository {
    Database* m_db;

public:
    int64_t insert(const Tool& tool);
    std::optional<Tool> findById(int64_t id);
    std::vector<Tool> findByType(ToolType type);
    bool update(const Tool& tool);
    bool remove(int64_t id);
};

// ToolService: Business logic layer
class ToolService {
    ToolRepository* m_repo;
    MaterialService* m_materials;

public:
    // High-level operations
    int64_t addTool(const Tool& tool) {
        // Validation logic
        if (tool.diameter <= 0) {
            throw std::invalid_argument("Diameter must be positive");
        }

        return m_repo->insert(tool);
    }

    // Feeds & speeds calculation (business logic)
    FeedsAndSpeeds calculate(int64_t toolId, int64_t materialId,
                              OperationType operation) {
        auto tool = m_repo->findById(toolId);
        if (!tool) throw std::runtime_error("Tool not found");

        auto material = m_materials->findById(materialId);
        if (!material) throw std::runtime_error("Material not found");

        // Domain logic
        float chipLoad = getRecommendedChipLoad(tool->type, material->hardness);
        float surfaceSpeed = getSurfaceSpeed(tool->material, material->type);

        float rpm = (surfaceSpeed * 1000.0f) / (M_PI * tool->diameter);
        float feedRate = rpm * tool->flutes * chipLoad;

        return {rpm, feedRate, calculateDepthOfCut(tool->type)};
    }

    // Tool recommendations
    std::vector<Tool> recommendToolsForOperation(OperationType op,
                                                   int64_t materialId) {
        auto allTools = m_repo->findByType(getToolTypeForOperation(op));

        // Filter based on material compatibility
        std::vector<Tool> compatible;
        for (const auto& tool : allTools) {
            if (isCompatible(tool, materialId)) {
                compatible.push_back(tool);
            }
        }

        // Sort by recommendation score
        std::sort(compatible.begin(), compatible.end(),
                  [](const Tool& a, const Tool& b) {
                      return a.recommendationScore > b.recommendationScore;
                  });

        return compatible;
    }
};

// UI Panel: Thin presentation layer
class ToolPanel : public Panel {
    ToolService* m_toolService;

    void render() override {
        if (ImGui::Button("Calculate")) {
            try {
                auto fs = m_toolService->calculate(
                    m_selectedTool, m_selectedMaterial, m_operation);
                displayFeedsAndSpeeds(fs);
            } catch (const std::exception& e) {
                showError(e.what());
            }
        }
    }
};
```

**Source:** [Service Layer Pattern](https://martinfowler.com/eaaCatalog/serviceLayer.html), [Understanding Service Layer](https://medium.com/@navroops38/understanding-the-service-layer-in-software-architecture-df9b676b3a16)

### Pattern 6: Filesystem Watcher with Event Queue

**What:** Background thread monitors watch folders using platform-specific APIs, posts events to main thread queue for processing

**When to use:** Automatic import when files are dropped into watched folders

**Trade-offs:**
- **Pro:** Responsive to file changes without polling
- **Pro:** Cross-platform libraries available (efsw, filewatch)
- **Con:** Must debounce events (file creates can trigger multiple notifications)
- **Con:** Must handle race conditions (file may not be fully written)

**Implementation approach:**

```cpp
// WatchService: Manages filesystem monitoring
class WatchService {
public:
    using FileEventCallback = std::function<void(const std::string& path)>;

    void addWatchFolder(const std::string& path) {
        m_watcher->addWatch(path, this, true); // recursive
    }

    void setFileCreatedCallback(FileEventCallback cb) {
        m_fileCreatedCallback = std::move(cb);
    }

private:
    // Called from watch thread
    void handleFileAction(efsw::WatchID watchId, const std::string& dir,
                          const std::string& filename, efsw::Action action) {
        if (action != efsw::Actions::Add) return;

        std::string fullPath = dir + "/" + filename;

        // Filter by extension
        if (!isImportableFormat(fullPath)) return;

        // Debounce: Only process if file stable for 500ms
        m_pendingFiles[fullPath] = std::chrono::steady_clock::now();

        // Post to main thread queue (thread-safe)
        m_mainThreadQueue->enqueue([this, fullPath]() {
            if (m_fileCreatedCallback) {
                m_fileCreatedCallback(fullPath);
            }
        });
    }

    bool isImportableFormat(const std::string& path) {
        static const std::vector<std::string> extensions = {
            ".stl", ".obj", ".3mf", ".gcode"
        };
        auto ext = getExtension(path);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return std::find(extensions.begin(), extensions.end(), ext)
               != extensions.end();
    }

    std::unique_ptr<efsw::FileWatcher> m_watcher;
    FileEventCallback m_fileCreatedCallback;
    MainThreadQueue* m_mainThreadQueue;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_pendingFiles;
};

// Integration with ImportQueue
void Application::setupWatchService() {
    m_watchService->setFileCreatedCallback([this](const std::string& path) {
        // Called on main thread
        m_importQueue->enqueue(path);
    });

    // Load watch folders from config
    auto config = Config::load();
    for (const auto& folder : config.watchFolders) {
        m_watchService->addWatchFolder(folder);
    }
}
```

**Recommended library:** [efsw](https://github.com/SpartanJ/efsw) (cross-platform, C++, active development)

**Alternative:** [FileWatch](https://github.com/ThomasMonkman/filewatch) (header-only, simpler)

**Source:** [efsw Documentation](https://github.com/SpartanJ/efsw), [C++17 Filesystem Watcher](https://solarianprogrammer.com/2019/01/13/cpp-17-filesystem-write-file-watcher-monitor/)

### Pattern 7: Async REST Client with Callback Queue

**What:** CNCService manages HTTP REST client for control commands, queues responses to main thread for UI updates

**When to use:** Sending commands to CNC machine without blocking UI (start job, pause, resume, set parameters)

**Trade-offs:**
- **Pro:** Non-blocking UI during network operations
- **Pro:** Can show progress/spinner while request in flight
- **Con:** Must handle timeouts and network failures gracefully
- **Con:** Callback-based async can get complex (consider std::future or coroutines in future)

**Implementation approach:**

```cpp
class CNCService {
public:
    // Async command: returns immediately, callback invoked on main thread later
    void sendCommand(const std::string& machineId, CNCCommand cmd,
                     std::function<void(bool success, std::string error)> callback) {

        // Execute HTTP request on background thread
        std::thread([this, machineId, cmd, callback]() {
            try {
                auto response = m_httpClient->post(
                    buildUrl(machineId, cmd.endpoint),
                    cmd.toJson()
                );

                bool success = (response.status == 200);
                std::string error = success ? "" : response.body;

                // Post result to main thread
                m_mainThreadQueue->enqueue([callback, success, error]() {
                    callback(success, error);
                });

            } catch (const std::exception& e) {
                m_mainThreadQueue->enqueue([callback, e]() {
                    callback(false, e.what());
                });
            }
        }).detach();
    }

    // Synchronous query (for immediate state checks)
    std::optional<MachineState> getState(const std::string& machineId) {
        try {
            auto response = m_httpClient->get(buildUrl(machineId, "/state"));
            if (response.status == 200) {
                return parseMachineState(response.body);
            }
        } catch (const std::exception& e) {
            logError("Failed to query machine state: {}", e.what());
        }
        return std::nullopt;
    }

private:
    std::unique_ptr<RestClient> m_httpClient;
    MainThreadQueue* m_mainThreadQueue;
};

// Usage in UI panel
void CNCPanel::onStartJobClicked() {
    m_showSpinner = true; // Show loading indicator

    m_cncService->sendCommand(m_machineId, CNCCommand::startJob(m_jobId),
        [this](bool success, std::string error) {
            m_showSpinner = false;

            if (success) {
                showNotification("Job started successfully");
            } else {
                showError("Failed to start job: " + error);
            }
        });
}
```

**Recommended library:** [cpprestsdk](https://github.com/microsoft/cpprestsdk) (Microsoft, mature, async-first) or [restclient-cpp](https://github.com/mrtazz/restclient-cpp) (simpler, less features)

**Source:** [C++ REST SDK](https://github.com/microsoft/cpprestsdk), [Modern C++ REST API](https://medium.com/@ivan.mejia/modern-c-micro-service-implementation-rest-api-b499ffeaf898)

## Anti-Patterns to Avoid

### Anti-Pattern 1: Service Locator for Subsystem Access

**What people do:** Create global ServiceLocator::get<T>() for accessing subsystems from anywhere

**Why it's wrong:**
- Hides dependencies (makes testing harder)
- Global state (initialization order issues)
- Tight coupling through the back door

**Do this instead:** Constructor injection or pass subsystem pointers explicitly

```cpp
// BAD: Service locator
class ToolPanel {
    void render() {
        auto tools = ServiceLocator::get<ToolService>()->getAllTools();
    }
};

// GOOD: Explicit dependency injection
class ToolPanel {
    ToolService* m_toolService; // Injected via constructor

public:
    ToolPanel(ToolService* toolService) : m_toolService(toolService) {}

    void render() {
        auto tools = m_toolService->getAllTools();
    }
};
```

### Anti-Pattern 2: Synchronous Blocking in UI Thread

**What people do:** Make synchronous HTTP requests or long-running calculations directly in UI rendering code

**Why it's wrong:**
- Freezes UI (ImGui stops responding)
- Poor user experience
- Can cause watchdog timeouts on some platforms

**Do this instead:** Async operations with callbacks or show modal progress dialog with event processing

```cpp
// BAD: Blocks UI thread
void CNCPanel::render() {
    if (ImGui::Button("Get Status")) {
        auto state = m_cncService->getState(m_machineId); // BLOCKS!
        displayState(state);
    }
}

// GOOD: Async with loading state
void CNCPanel::render() {
    if (ImGui::Button("Get Status")) {
        m_loading = true;
        m_cncService->getStateAsync(m_machineId, [this](auto state) {
            m_loading = false;
            m_cachedState = state;
        });
    }

    if (m_loading) {
        ImGui::Spinner(); // Show progress
    } else {
        displayState(m_cachedState);
    }
}
```

### Anti-Pattern 3: Database Access from UI Panels

**What people do:** UI panels directly call repositories or execute SQL

**Why it's wrong:**
- Breaks layering (UI depends on data layer directly)
- Business logic scattered across UI code
- Hard to test UI without database

**Do this instead:** UI → Service → Repository → Database

```cpp
// BAD: Panel directly accesses repository
class QuotePanel {
    QuoteRepository* m_repo;

    void render() {
        if (ImGui::Button("Save")) {
            Quote quote = buildQuoteFromUI();
            float total = 0.0f;
            for (const auto& item : quote.lineItems) {
                total += item.quantity * item.unitPrice;
            }
            quote.total = total;
            m_repo->insert(quote); // Direct DB access!
        }
    }
};

// GOOD: Panel calls service, service handles business logic
class QuotePanel {
    QuoteService* m_service;

    void render() {
        if (ImGui::Button("Save")) {
            Quote quote = buildQuoteFromUI();
            m_service->saveQuote(quote); // Service calculates total, validates, persists
        }
    }
};
```

### Anti-Pattern 4: Command Objects Storing Too Much State

**What people do:** Commands capture entire object graphs for undo, leading to memory bloat

**Why it's wrong:**
- Memory usage grows with undo stack depth
- Copying large objects (meshes, images) repeatedly
- May capture stale pointers

**Do this instead:** Store minimal state (IDs, deltas) and reconstruct from database on undo

```cpp
// BAD: Stores entire mesh
class TransformModelCommand : public Command {
    Mesh m_beforeMesh; // Could be 10MB+!
    Mesh m_afterMesh;
    // ...
};

// GOOD: Stores only transform delta
class TransformModelCommand : public Command {
    int64_t m_modelId;
    glm::mat4 m_oldTransform;
    glm::mat4 m_newTransform;

    bool execute() override {
        return m_modelRepo->setTransform(m_modelId, m_newTransform);
    }

    bool undo() override {
        return m_modelRepo->setTransform(m_modelId, m_oldTransform);
    }
};
```

### Anti-Pattern 5: EventBus with Strong References

**What people do:** EventBus holds strong references (shared_ptr) to subscribers

**Why it's wrong:**
- Memory leaks: subscribers never destroyed even after panel closed
- Events delivered to dead UI components
- Circular dependencies between EventBus and subscribers

**Do this instead:** Use weak_ptr for subscribers, clean up expired subscriptions on publish

```cpp
// BAD: Strong references
class EventBus {
    std::vector<std::shared_ptr<ISubscriber>> m_subscribers; // Leak!
};

// GOOD: Weak references (shown in Pattern 3)
class EventBus {
    struct Subscription {
        std::weak_ptr<void> owner; // Automatically null when owner destroyed
        std::function<void(const Event&)> callback;
    };

    void publish(const Event& event) {
        // Clean up dead subscriptions first
        m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(),
            [](auto& h) { return h.owner.expired(); }), m_handlers.end());

        // Then notify alive subscribers
        for (auto& h : m_handlers) {
            if (!h.owner.expired()) {
                h.callback(event);
            }
        }
    }
};
```

## Integration Strategy (Build Order Dependencies)

### Phase 1 Foundation: EventBus + ConnectionPool

**Build these first (no dependencies on new subsystems):**

1. **EventBus** - Core pub/sub infrastructure
   - Zero dependencies beyond std::
   - Enables loose coupling for all future subsystems
   - Test with simple events before integrating

2. **ConnectionPool** - Thread-safe database access
   - Depends on existing Database class
   - Enables parallel work before subsystems need it
   - Test with ImportQueue worker threads

**Deliverable:** EventBus + ConnectionPool integrated into Application, ImportQueue uses pooled connections

### Phase 2 Decomposition: SubsystemCoordinator

**Decompose Application god class:**

3. **SubsystemCoordinator** - Facade for subsystem access
   - Move subsystem ownership from Application to SubsystemCoordinator
   - Application becomes thin lifecycle coordinator
   - Panels receive subsystem pointers via coordinator

**Deliverable:** Application.cpp reduced from 1,071 → ~300 lines, existing functionality preserved

### Phase 3 Commands: CommandManager + Example Commands

**Add undo/redo infrastructure:**

4. **CommandManager** - Undo/redo stack management
   - Depends on EventBus (publishes CommandExecuted events)
   - Start with simple commands (edit project name, delete model)
   - Integrate keyboard shortcuts (Ctrl+Z, Ctrl+Shift+Z)

**Deliverable:** Basic undo/redo working for existing operations

### Phase 4 Domain Services: ToolService + MaterialService

**Add domain-specific business logic layers:**

5. **ToolService** + **ToolRepository**
   - Database schema for tools (diameter, flutes, material, feeds/speeds parameters)
   - CRUD operations + feeds/speeds calculator
   - Command wrappers (AddToolCommand, EditToolCommand)

6. **MaterialService** + **MaterialRepository**
   - Database schema for wood species (hardness, density, grain direction)
   - Material property lookup

7. **ToolPanel** + **MaterialPanel** UI
   - Subscribe to EventBus for tool/material changes
   - Call services for operations

**Deliverable:** Tool and material databases functional with undo/redo

### Phase 5 Advanced Features: CNCService + QuoteService

**Add complex subsystems with external dependencies:**

8. **CNCService**
   - REST client for control commands
   - WebSocket client for real-time state
   - State cache + EventBus integration
   - CNCPanel UI for machine monitoring

9. **QuoteService** + **QuoteRepository**
   - Cost calculation workflow
   - Line item management
   - PDF generation (integrate library: libharu or PDFWriter)
   - QuotePanel UI

**Deliverable:** CNC machine control + cost estimation functional

### Phase 6 Background Processing: SearchService + WatchService

**Add background worker subsystems:**

10. **SearchService**
    - Search indexing (SQLite FTS5 or in-memory)
    - Query parsing + ranking
    - Async search execution with progress
    - SearchPanel UI

11. **WatchService**
    - Integrate efsw or filewatch library
    - Connect to ImportQueue
    - Settings UI for managing watch folders

**Deliverable:** Advanced search + automatic import from watch folders

### Phase 7 Refinement: Command Coverage + Thread Pool Enhancement

**Complete the architecture:**

12. Expand CommandManager coverage to all subsystems
13. Replace single worker thread with thread pool (BS::thread_pool or std::thread pool)
14. Performance profiling and optimization

**Deliverable:** All operations undoable, parallel background processing

## Dependency Graph

```
EventBus ←─┐
           │
ConnectionPool ←─┐
                 │
    SubsystemCoordinator
         │
         ├──→ CommandManager (depends on EventBus)
         │
         ├──→ ToolService ───→ ToolRepository (depends on ConnectionPool)
         │       │
         │       └──→ FeedsSpeedsCalculator
         │
         ├──→ MaterialService ───→ MaterialRepository
         │
         ├──→ CNCService (depends on EventBus, REST client, WebSocket client)
         │
         ├──→ QuoteService ───→ QuoteRepository (depends on ConnectionPool)
         │                   └──→ PDF generation library
         │
         ├──→ SearchService ───→ SearchIndex (SQLite FTS5 or custom)
         │                   └──→ Thread pool
         │
         └──→ WatchService (depends on efsw, EventBus)
```

## Thread Safety Considerations

### Thread Model

| Thread | Responsibilities | Database Access | EventBus Access |
|--------|------------------|-----------------|-----------------|
| **Main** | UI rendering (ImGui), event loop, panel updates | Main connection (non-pooled) | Publish events (main thread only) |
| **Import workers** | File loading, mesh processing | Pooled connections | Post events to main thread queue |
| **CNC WebSocket** | WebSocket I/O, state updates | Read-only queries (pooled) | Post events to main thread queue |
| **Watch folder** | Filesystem monitoring | None (posts to main thread) | Post events to main thread queue |
| **Search workers** | Index building, query execution | Pooled connections | Post results to main thread queue |

### Thread Safety Rules

1. **EventBus is NOT thread-safe:** Only publish from main thread. Background threads must enqueue events.

2. **ImGui is NOT thread-safe:** Only call ImGui functions from main thread.

3. **Database connections are thread-isolated:** Each thread uses separate connection from pool.

4. **Repositories are NOT thread-safe:** Create repository instance per thread (they're lightweight, hold only Database*).

5. **Services CAN be shared:** If service state is immutable or protected by mutexes.

### Main Thread Queue Pattern

```cpp
class MainThreadQueue {
public:
    void enqueue(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(std::move(task));
    }

    void processAll() {
        std::queue<std::function<void()>> tasks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            tasks.swap(m_tasks); // Move all tasks out
        }

        while (!tasks.empty()) {
            tasks.front()();
            tasks.pop();
        }
    }

private:
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
};

// In Application::update() (called every frame on main thread)
void Application::update() {
    m_mainThreadQueue.processAll(); // Execute all pending tasks
    // ... rest of update logic
}
```

## Scaling Considerations

This is a desktop application, not a web service. Scaling concerns are limited:

| Scale | Approach |
|-------|----------|
| **10-50 models** | Current single-threaded import sufficient |
| **100-500 models** | Connection pool + 2 import workers (already planned) |
| **1,000+ models** | Increase worker threads to 4, add progress batching, thumbnail generation optimization |
| **10,000+ models** | Paginate library view, lazy-load thumbnails, optimize search indexing (FTS5 with incremental updates) |

**First bottleneck:** Thumbnail generation (OpenGL context per thread is expensive)
- **Solution:** Generate thumbnails on demand, cache aggressively, limit concurrent generation to 2

**Second bottleneck:** Search index size (FTS5 index grows with model count)
- **Solution:** Incremental indexing, compression, consider external search engine (unlikely to be needed)

## Technology Stack for New Subsystems

| Subsystem | Required Libraries | Why |
|-----------|-------------------|-----|
| **CNCService (REST)** | [cpprestsdk](https://github.com/microsoft/cpprestsdk) or [restclient-cpp](https://github.com/mrtazz/restclient-cpp) | Async HTTP client, mature, cross-platform |
| **CNCService (WebSocket)** | [websocketpp](https://github.com/zaphoyd/websocketpp) | Header-only, stable, widely used |
| **QuoteService (PDF)** | [libharu](http://libharu.org/) or [PDFWriter](https://github.com/galkahana/PDF-Writer) | C++ PDF generation, active maintenance |
| **WatchService** | [efsw](https://github.com/SpartanJ/efsw) | Cross-platform filesystem watcher, C++, fast |
| **SearchService** | SQLite FTS5 (already using SQLite) | Full-text search built into SQLite, zero new deps |
| **Thread Pool** | [BS::thread_pool](https://github.com/bshoshany/thread-pool) | Modern C++17, lightweight, well-tested (v5.1.0 released Jan 2026) |

**Principle:** Prefer header-only or CMake-integrated libraries to minimize build complexity.

## Sources

### Architectural Patterns
- [Layered Architecture in C++](https://www.modernescpp.com/index.php/layers/)
- [Service Layer Pattern](https://martinfowler.com/eaaCatalog/serviceLayer.html)
- [Understanding Service Layer](https://medium.com/@navroops38/understanding-the-service-layer-in-software-architecture-df9b676b3a16)
- [Software Architecture with C++](https://www.oreilly.com/library/view/software-architecture-with/9781838554590/)

### Design Patterns
- [Command Pattern for Undo/Redo](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/)
- [Qt Undo Framework](https://doc.qt.io/qt-6/qundo.html)
- [Observer Pattern in C++](https://medium.com/@chetanp.verma98/c-observer-design-pattern-a-comprehensive-deep-dive-50101a2a8ee1)
- [Event-Driven Architecture](https://www.geeksforgeeks.org/system-design/event-driven-architecture-system-design/)
- [Facade Pattern in C++](https://refactoring.guru/design-patterns/facade/cpp/example)

### Thread Safety & Database
- [SQLite Threading Modes](https://www.sqlite.org/threadsafe.html)
- [Libzdb Connection Pooling](https://www.tildeslash.com/libzdb/)
- [Multi-threaded SQLite Access](https://dev.yorhel.nl/doc/sqlaccess)

### Concurrency
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) (C++17/20/23 thread pool library)
- [Background Threads in C++](https://cloud.google.com/cpp/docs/background-threads)
- [Multithreaded Work Queue in C++](https://vichargrave.github.io/programming/multithreaded-work-queue-in-cpp/)

### Filesystem Watching
- [efsw Filesystem Watcher](https://github.com/SpartanJ/efsw)
- [C++17 Filesystem Watcher](https://solarianprogrammer.com/2019/01/13/cpp-17-filesystem-write-file-watcher-monitor/)

### WebSocket & REST
- [WebSocket++ Documentation](https://docs.websocketpp.org/)
- [WebSocket Architecture Best Practices](https://ably.com/topic/websocket-architecture-best-practices)
- [C++ REST SDK](https://github.com/microsoft/cpprestsdk)
- [Modern C++ REST API](https://medium.com/@ivan.mejia/modern-c-micro-service-implementation-rest-api-b499ffeaf898)

### ImGui Integration
- [About the IMGUI Paradigm](https://github.com/ocornut/imgui/wiki/About-the-IMGUI-paradigm)
- [ImGui State Management](https://www.rfleury.com/p/ui-part-2-build-it-every-frame-immediate)

---

*Architecture research for: CNC Workshop Management Desktop Software*
*Researched: 2026-02-08*
*Confidence: HIGH (patterns are well-established, libraries verified to be active)*
