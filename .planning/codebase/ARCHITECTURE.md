# Architecture

**Analysis Date:** 2026-02-19

## Pattern Overview

**Overall:** Layered Application with Separation of Concerns (Thin Application Coordinator + Core Services + UI Framework)

**Key Characteristics:**
- **Thin Application Coordinator**: `Application` class handles SDL2/OpenGL lifecycle and event loop, delegates business logic to managers
- **Manager-Based Abstraction**: Three extracted managers (`UIManager`, `FileIOManager`, `ConfigManager`) implement god class decomposition
- **Type-Safe Event Bus**: Main-thread-only publish-subscribe system for decoupled subsystem communication
- **Focus-Based UI Model**: `Workspace` class tracks "focused object" (mesh, g-code, cut plan) that drives panel updates
- **Multi-Stage Import Pipeline**: Background thread pool with thread-safe staging and main-thread completion handling
- **Repository Pattern**: Database access abstracted through specialized repositories (`ModelRepository`, `ProjectRepository`, `MaterialRepository`, `GCodeRepository`)

## Layers

**Application Layer:**
- Purpose: SDL2/OpenGL window management, frame loop coordination, manager initialization
- Location: `src/app/application.cpp`, `src/app/application.h`
- Contains: Window lifecycle, render loop (processEvents → update → render), manager ownership
- Depends on: EventBus, MainThreadQueue, all core systems and managers
- Used by: main.cpp entry point

**UI Layer:**
- Purpose: ImGui-based user interface panels, dialogs, menu bar, keyboard shortcuts
- Location: `src/ui/`, `src/managers/ui_manager.h`
- Contains: Panels (Viewport, Library, Properties, Project, GCode, CutOptimizer, StartPage), Dialogs (File, Lighting, ImportSummary, Message), Widgets (StatusBar, Toast, BindingRecorder)
- Depends on: Workspace (for focused object), LibraryManager, ProjectManager, ImportQueue, EventBus
- Used by: UIManager for centralized rendering and state management

**Manager Layer (Business Logic):**
- Purpose: Orchestration of file I/O, configuration, and UI state
- Location: `src/managers/`
- Contains: UIManager (panel/dialog ownership), FileIOManager (import/export/project orchestration), ConfigManager (settings, hot-reload, workspace state)
- Depends on: EventBus, Database, LibraryManager, ProjectManager, ImportQueue
- Used by: Application for delegating business logic

**Core Domain Layer:**
- Purpose: Business domain models and operations
- Location: `src/core/`
- Contains:
  - **Database**: SQLite wrapper, connection pool, schema, repositories
  - **Library**: LibraryManager (import, search, tag management, duplicate detection)
  - **Project**: ProjectManager, Project domain model
  - **Import**: ImportQueue with multi-stage pipeline, thread-safe progress tracking
  - **Materials**: MaterialRepository for material records with pricing
  - **Threading**: ThreadPool, MainThreadQueue, thread utilities
  - **Config**: Configuration singleton with hot-reload via file watching
- Depends on: SQLite3, thread library, filesystem utilities
- Used by: Manager layer for domain operations

**Render Layer:**
- Purpose: OpenGL 3.3 rendering, GPU mesh management, camera control
- Location: `src/render/`
- Contains: Renderer (mesh/grid/axis rendering), Camera (orbit-based 3D navigation), Shader (GLSL wrapper), Framebuffer (offscreen rendering), ThumbnailGenerator (offline model thumbnails)
- Depends on: GLAD (GL loader), GLM (math), stb_image (texture loading)
- Used by: ViewportPanel for 3D visualization, ThumbnailGenerator for library thumbnails

**Loader Layer:**
- Purpose: Multi-format mesh and g-code file loading
- Location: `src/core/loaders/`
- Contains: LoaderFactory, MeshLoader interface, format-specific loaders (STL, OBJ, 3MF, GCode)
- Depends on: Mesh domain model, ByteBuffer utilities
- Used by: ImportQueue during file parsing stage

**Mesh/Geometry Layer:**
- Purpose: In-memory 3D geometry representation and operations
- Location: `src/core/mesh/`
- Contains: Mesh class (vertices, triangles, normals, bounds), auto-orientation for relief models, spatial bounds checking, content-based hashing for deduplication
- Depends on: GLM math, file utilities
- Used by: Loaders, Renderer, GCode analyzer, library operations

**G-Code Layer:**
- Purpose: G-code file parsing, analysis, metadata extraction
- Location: `src/core/gcode/`
- Contains: GCodeParser (line-by-line parsing), GCodeAnalyzer (statistics, move types, time estimation)
- Depends on: String utilities, math
- Used by: ImportQueue, GCodePanel for visualization and analysis

**Optimization Layer:**
- Purpose: 2D cutting plan generation and material layout optimization
- Location: `src/core/optimizer/`
- Contains: CutOptimizer (orchestration), BinPacker (placement algorithm), Guillotine (rectangle packing)
- Depends on: Mesh geometry, math
- Used by: CutOptimizerPanel for interactive layout

## Data Flow

**Model Import Pipeline:**

1. User selects files → FileIOManager.importModel() → FileDialog collects paths
2. FileIOManager.onFilesDropped() or importModel() enqueues ImportTasks
3. ImportQueue (worker threads):
   - Read file → Hash → Check duplicate → Parse → Insert DB → WaitingForThumbnail
4. Main thread (processCompletedImports):
   - Render thumbnail → Upload to DB
   - Update UI panels (Library, Properties)
   - Emit ModelSelected event → Updates Workspace.focusedMesh
5. Panels observe Workspace changes and re-render

**Project Workflow:**

1. User creates/opens project → FileIOManager.newProject() or openProject()
2. ProjectManager creates Project domain object, loads model associations
3. Project models added to workspace → Panels display model list
4. User exports → FileIOManager.exportModel() → Loaders serialize format
5. Project saved to ProjectRepository (SQLite) with modified timestamp

**G-Code Workflow:**

1. User imports G-code file → ImportQueue treats as ImportType::GCode
2. GCodeParser extracts metadata (tool changes, move types, estimates)
3. GCodeRepository stores file record with metadata
4. User selects g-code → Workspace.focusedGCode set
5. GCodePanel displays analysis; user can link to model or operation group
6. On render: GCodeAnalyzer generates toolpath mesh (visual preview)

**Configuration and State:**

1. Application startup → Config::instance().load() reads INI file
2. Config change (UI or external file) → ConfigWatcher detects via polling
3. ConfigWatcher notifies ConfigManager → hot-reload applied
4. Workspace state (window size, panel visibility, recent projects) persisted to Config
5. Application restart restores previous session state

**State Management:**

- **EventBus**: Type-safe publish/subscribe for model selection, import completion, config changes
- **Workspace**: Tracks "focused" objects (mesh, g-code, toolpath, cut plan) — panels observe
- **MainThreadQueue**: Workers enqueue UI updates (thumbnail completion) for main thread execution
- **Atomic Flags**: ImportProgress uses atomics for thread-safe progress updates (no locks)

## Key Abstractions

**LibraryManager:**
- Purpose: Unified interface for model library operations
- Examples: `src/core/library/library_manager.h`, `src/core/database/model_repository.h`
- Pattern: Facade over ModelRepository with deduplication logic, thumbnail generation, metadata management
- Key methods: importModel(), getAllModels(), searchModels(), filterByTag(), generateThumbnail()

**Repository Pattern:**
- Purpose: Abstract database access behind domain-specific queries
- Examples: `ModelRepository`, `ProjectRepository`, `MaterialRepository`, `GCodeRepository`
- Pattern: CRUD + domain-specific methods (e.g., getAllModels, filterByTag)
- Storage: All backed by SQLite with prepared statements

**Panel Pattern:**
- Purpose: Reusable UI component framework
- Base class: `src/ui/panels/panel.h` (virtual render(), title, visibility state)
- Subclasses: ViewportPanel, LibraryPanel, PropertiesPanel, ProjectPanel, GCodePanel, CutOptimizerPanel, StartPage
- Pattern: Each panel observes Workspace and updates on focus changes

**Dialog Pattern:**
- Purpose: Modal dialog framework
- Base class: `src/ui/dialogs/dialog.h` (virtual render(), open/close state, title)
- Subclasses: FileDialog, LightingDialog, ImportSummaryDialog, MessageDialog
- Pattern: Caller triggers dialog.open(), dialog renders modally, caller checks result

**ImportTask Pipeline:**
- Purpose: Staged processing of import files across threads
- Stages: Pending → Reading → Hashing → CheckingDuplicate → Parsing → Inserting → WaitingForThumbnail → Done/Failed
- Pattern: Struct carries data through stages; ImportQueue advances stage; Main thread handles thumbnail stage
- Thread-safety: Atomic stage enum, mutex-protected filename for progress display

**Loader Factory:**
- Purpose: Polymorphic file format support
- Pattern: getLoader(path) → format-specific MeshLoader subclass → LoadResult
- Supported formats: STL, OBJ, 3MF (mesh); GCode, NC, NGC, TAP (g-code)
- Extensibility: New loaders added as MeshLoader subclasses

## Entry Points

**main():**
- Location: `src/main.cpp`
- Triggers: Application startup
- Responsibilities: Create Application instance, call init(), run(), return exit code

**Application::init():**
- Location: `src/app/application.cpp`
- Triggers: On startup before main loop
- Responsibilities:
  - Initialize paths, config, logging
  - Create SDL2 window, OpenGL context
  - Initialize ImGui with docking
  - Create core systems (EventBus, Database, LibraryManager, ProjectManager, Workspace)
  - Create managers (UIManager, FileIOManager, ConfigManager)
  - Initialize thread pool, import queue

**Application::run():**
- Location: `src/app/application.cpp`
- Triggers: Main loop entry
- Responsibilities: Frame loop (processEvents → update → render → swap)
  - ProcessEvents: SDL_PollEvent, keyboard input, drag-and-drop
  - Update: Update panels, process MainThreadQueue, advance ImportQueue
  - Render: UIManager.renderMenuBar() + renderPanels(), ImGui render

**Application::processEvents():**
- Location: `src/app/application.cpp`
- Triggers: Each frame
- Responsibilities: Handle SDL events (window close, window resize, input), fire EventBus events for custom shortcuts

**UIManager::renderPanels():**
- Location: `src/managers/ui_manager.cpp`
- Triggers: Each frame in Application::render()
- Responsibilities: Call render() on all panels and dialogs, manage ImGui docking layout

**ImportQueue (worker threads):**
- Location: `src/core/import/import_queue.h`
- Triggers: FileIOManager.importModel() or onFilesDropped()
- Responsibilities: Advance ImportTasks through pipeline stages on background ThreadPool

## Error Handling

**Strategy:** Early validation with optional-based returns; exceptions caught at system boundaries; error propagation via struct fields

**Patterns:**
- **Result<T>** (alias for std::optional<T>): Used for file operations, DB queries, loader results
- **ImportTask::error** field: Populated on failure; displayed in ImportSummaryDialog
- **try-catch at EventBus boundary**: Handler exceptions logged but don't propagate to other handlers
- **File I/O**: FileIOManager wraps file operations with error reporting to user via MessageDialog or toast
- **Database**: Prepared statement execution checked; transaction rollback on failure
- **GL Errors**: gl_utils.cpp provides checkGLError() for shader compilation and GL state validation

## Cross-Cutting Concerns

**Logging:**
- Framework: Custom log::errorf(), log::warnf(), log::infof() in `src/core/utils/log.h`
- Configuration: Log level set from Config, applied via log::setLevel()
- Pattern: Namespace-based categorization (e.g., "event_bus", "import_queue", "database")

**Validation:**
- Input: File format validation via LoaderFactory.isSupported(); project name non-empty
- State: ImportTask stage transitions validated implicitly (stages are sequential)
- Database: Foreign key constraints enabled in schema; unique hash constraints for deduplication

**Authentication:**
- Not applicable (single-user desktop application)

**Threading:**
- Main thread assertion: ASSERT_MAIN_THREAD() macro in thread_utils.h
- Worker threads: ThreadPool and ImportQueue dequeue from global queue
- Cross-thread communication: MainThreadQueue for workers → main, EventBus for main → main
- Synchronization: Mutex for database connection pool; atomics for progress tracking

**Configuration:**
- Storage: INI file in config directory
- Hot-reload: ConfigWatcher polls file modification time every 100ms
- Propagation: ConfigManager.applyConfigChanges() called on change; ApplicationConfig events published

---

*Architecture analysis: 2026-02-19*
