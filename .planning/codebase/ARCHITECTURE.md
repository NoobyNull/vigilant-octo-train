# Architecture

**Analysis Date:** 2026-02-08

## Pattern Overview

**Overall:** Layered N-tier architecture with a focused object state management pattern

**Key Characteristics:**
- **Separation of concerns** with distinct layers: UI (ImGui), Render (OpenGL), Core (domain logic), and Data (SQLite)
- **Workspace-centric state management** — panels observe and act on a single focused object (mesh, G-code, or cut plan) held in the Workspace
- **Repository pattern** for data access — repositories mediate all database operations through a SQLite wrapper
- **Factory pattern** for extensible file format loading (STL, OBJ, 3MF)
- **Thread-safe import pipeline** with background worker for processing large files
- **Render-to-texture** architecture for thumbnail generation and viewport rendering

## Layers

**Application (UI Orchestration):**
- Purpose: Main application lifecycle, event loop, menu dispatch, panel lifecycle
- Location: `src/app/application.h`, `src/app/application.cpp`
- Contains: Application class managing SDL2 window, ImGui frame lifecycle, panel instantiation
- Depends on: Workspace, Database, LibraryManager, ProjectManager, all UI panels, Renderer
- Used by: `src/main.cpp` (entry point)

**Workspace (Focused Object State):**
- Purpose: Central state holder for "what is currently being worked on" — maintains references to the currently focused Mesh, GCodeFile, and CutPlan
- Location: `src/app/workspace.h`, `src/app/workspace.cpp`
- Contains: Shared pointers to focused mesh, G-code, and cut plan; setters and getters
- Depends on: Core types (Mesh, GCodeFile, CutPlan)
- Used by: All UI panels read workspace state; Application updates workspace on user actions

**UI Layer (ImGui-based):**
- Purpose: Immediate-mode GUI rendering with dockable panels
- Location: `src/ui/`, `src/ui/panels/`, `src/ui/dialogs/`, `src/ui/widgets/`
- Contains:
  - UIManager (`src/ui/ui_manager.h`) — ImGui context and frame management
  - Base Panel class (`src/ui/panels/panel.h`) — abstract base for all panels
  - Concrete panels: ViewportPanel, LibraryPanel, PropertiesPanel, ProjectPanel, GCodePanel, CutOptimizerPanel, StartPage
  - Dialogs: FileDialog, MessageDialog, ConfirmDialog, LightingDialog
  - Theme system (`src/ui/theme.h`) — dark/light/high-contrast themes
  - Icon system (`src/ui/icons.h`) — FontAwesome Unicode constants
- Depends on: Workspace, render system, library/project managers
- Used by: Application renders panels each frame; panels query workspace and invoke manager methods

**Render Layer (OpenGL 3.3):**
- Purpose: 3D visualization with GPU mesh upload, camera control, and offscreen rendering
- Location: `src/render/`
- Contains:
  - Renderer (`src/render/renderer.h`) — main OpenGL context, mesh cache, shader programs
  - Camera (`src/render/camera.h`) — orbit camera with target-relative positioning
  - Shader (`src/render/shader.h`) — RAII shader program wrapper with uniform caching
  - Framebuffer (`src/render/framebuffer.h`) — render-to-texture for thumbnails and viewport
  - ThumbnailGenerator (`src/render/thumbnail_generator.h`) — offscreen rendering with TGA export
  - GLUtils (`src/render/gl_utils.h`) — error checking and debug utilities
- Depends on: Core types (Mesh, Vec3, Mat4), GLAD (OpenGL bindings)
- Used by: ViewportPanel, LibraryPanel (thumbnails), ThumbnailGenerator

**Core Domain Layer:**
- Purpose: Business logic and domain models independent of UI or storage
- Location: `src/core/`
- Contains:
  - **Types** (`src/core/types.h`) — common type aliases (Vec3, Mat4, i64, etc.) and Color struct
  - **Mesh** (`src/core/mesh/mesh.h`) — 3D geometry with vertices, indices, bounds; auto-orientation for relief models
  - **Vertex** (`src/core/mesh/vertex.h`) — vertex structure with position, normal, texcoord, custom hash for deduplication
  - **Bounds** (`src/core/mesh/bounds.h`) — axis-aligned bounding box (AABB) with spatial queries
  - **GCode** (`src/core/gcode/`) — parser and analyzer for CNC toolpath files
  - **CutOptimizer** (`src/core/optimizer/`) — 2D nesting optimizer with guillotine and bin-packing algorithms
  - **Project** (`src/core/project/project.h`) — project domain model with model associations
  - **Utils** (`src/core/utils/`) — logging, file I/O, string utilities
  - **Config** (`src/core/config/`) — configuration loading, input binding, file watching
  - **Paths** (`src/core/paths/`) — application directory management
- Depends on: Standard library, GLM (math)
- Used by: Application, managers, UI panels

**Data Layer (Repositories):**
- Purpose: Encapsulate database access patterns and SQLite transactions
- Location: `src/core/database/`
- Contains:
  - Database (`src/core/database/database.h`) — RAII SQLite wrapper with prepared statements and transactions
  - Statement (`src/core/database/database.h`) — prepared statement wrapper with bind/step/get operations
  - Transaction (`src/core/database/database.h`) — RAII transaction wrapper with rollback on destruction
  - Schema (`src/core/database/schema.h`) — schema versioning and creation
  - ModelRepository (`src/core/database/model_repository.h`) — CRUD for models with tags, hash-based deduplication
  - ProjectRepository (`src/core/database/project_repository.h`) — CRUD for projects and model associations
  - CostRepository (`src/core/database/cost_repository.h`) — CRUD for cost estimates with line items
- Depends on: SQLite3, core types
- Used by: LibraryManager, ProjectManager, ImportQueue

**Manager Layer (Business Logic Coordinators):**
- Purpose: Coordinate across multiple layers and implement high-level workflows
- Location: `src/core/library/`, `src/core/project/`, `src/core/import/`, `src/core/export/`
- Contains:
  - LibraryManager (`src/core/library/library_manager.h`) — import, search, tag, delete models; hash-based duplicate detection
  - ProjectManager (`src/core/project/project.h`) — create/open/save projects; manage model membership
  - ImportQueue (`src/core/import/import_queue.h`) — background thread worker for multi-stage import pipeline (file → hash → parse → DB → thumbnail)
  - ModelExporter (`src/core/export/model_exporter.h`) — export mesh to STL, OBJ, 3MF formats
- Depends on: Repositories, Loaders, Renderer
- Used by: Application, UI panels

**Loader Layer (File Format Support):**
- Purpose: Pluggable format-specific mesh loading
- Location: `src/core/loaders/`
- Contains:
  - MeshLoader interface (`src/core/loaders/loader.h`) — abstract base with load() and supports()
  - LoaderFactory (`src/core/loaders/loader_factory.h`) — static factory methods for format selection
  - STLLoader, OBJLoader, ThreeMFLoader — concrete implementations
- Depends on: Mesh, types
- Used by: LibraryManager, ImportQueue

## Data Flow

**Model Import (User drags files → Database + Thumbnail):**

1. User drops files or clicks Import
2. Application calls LibraryManager.importModel(path) for each file
3. LibraryManager enqueues ImportTask to ImportQueue
4. Background ImportQueue worker thread:
   - Reads file into ByteBuffer
   - Computes hash for deduplication
   - Calls LoaderFactory.loadFromBuffer() → gets Mesh
   - Stores Mesh and metadata in database via ModelRepository
   - Enqueues task to completed queue
5. Application polls ImportQueue.pollCompleted() each frame
6. For each completed task:
   - Application.processCompletedImports() → ThumbnailGenerator.generate()
   - Renderer uploads mesh to GPU, renders to Framebuffer
   - ThumbnailGenerator encodes to TGA and writes to library folder
   - ModelRepository updates thumbnail path in database
7. Workspace remains unchanged; LibraryPanel refreshes via LibraryManager.getAllModels()

**Model Focus (User selects model in library → Viewport displays it):**

1. User single-clicks model in LibraryPanel
2. LibraryPanel invokes m_onModelSelected callback → Application.onModelSelected()
3. Application:
   - Calls LibraryManager.loadMesh(modelId) → queries ModelRepository, calls LoaderFactory
   - Mesh is cached in LibraryManager's internal storage
   - Calls Workspace.setFocusedMesh(mesh)
4. ViewportPanel observes Workspace (queried in render())
5. ViewportPanel:
   - Calls Renderer.uploadMesh(mesh) → caches GPU mesh
   - Each frame: Renderer.renderMesh(m_mesh, modelMatrix)
6. PropertiesPanel also reads Workspace.getFocusedMesh() and displays metadata

**Project Save (User adds models to project → Database persists associations):**

1. User drags models from LibraryPanel to ProjectPanel
2. Application calls ProjectManager.addModelToProject(modelId)
3. ProjectManager:
   - Updates in-memory Project.m_modelIds
   - Marks project as modified
4. User saves project (Ctrl+S)
5. Application calls ProjectManager.save(project)
6. ProjectManager invokes ProjectRepository.update() within a Transaction
7. ProjectRepository writes project metadata and model associations to database

**G-code Analysis (User loads G-code file → Analyzer extracts statistics):**

1. User imports G-code file (treated as model with .gcode extension)
2. LoaderFactory creates special handler that:
   - Reads G-code text
   - Calls GCodeParser.parse()
   - Creates virtual Mesh representation (for consistency)
3. GCode stored in database
4. User clicks "Analyze" button in GCodePanel
5. GCodePanel calls GCodeAnalyzer.analyze(gcodeFile)
6. Analyzer extracts move commands, compute toolpath length, estimate time
7. Results displayed in GCodePanel

## Key Abstractions

**Mesh (Vertex/Index Container):**
- Purpose: Unified representation for 3D geometry from any source
- Examples: `src/core/mesh/mesh.h`, loaded by any MeshLoader subclass
- Pattern: Owner of vertex/index arrays; provides geometry operations (recalculate bounds/normals, transform, auto-orient)

**Panel (Abstract UI Component):**
- Purpose: Composable UI element with frame-based rendering
- Examples: `src/ui/panels/viewport_panel.h`, `src/ui/panels/library_panel.h`
- Pattern: Virtual render() method called by Application each frame; manages own state and visuals

**Repository (Data Access):**
- Purpose: Encapsulate SQL queries and return domain objects
- Examples: `src/core/database/model_repository.h`, `src/core/database/project_repository.h`
- Pattern: Typically owns a Database reference; wraps prepared statements; returns std::optional<Record> or std::vector<Record>

**Loader (Format Extension):**
- Purpose: Implement loading for a specific file format
- Examples: `src/core/loaders/stl_loader.h`, `src/core/loaders/obj_loader.h`
- Pattern: Inherit MeshLoader interface; implement load(path) → LoadResult; declare supports(ext)

**Manager (Workflow Orchestrator):**
- Purpose: High-level operations spanning multiple layers
- Examples: `src/core/library/library_manager.h`, `src/core/project/project.h`
- Pattern: Owns repositories and other managers; provides public API for common workflows (import, search, save)

## Entry Points

**main() - Application Launch:**
- Location: `src/main.cpp`
- Triggers: Program execution
- Responsibilities: Create Application, call init() and run(), return exit code

**Application::init() - Initialization Sequence:**
- Location: `src/app/application.h`, `src/app/application.cpp`
- Triggers: Called by main()
- Responsibilities:
  - Initialize SDL2 window and OpenGL context
  - Initialize ImGui with docking backend
  - Create and initialize all managers (Database, LibraryManager, ProjectManager, ImportQueue)
  - Create and initialize all panels and dialogs
  - Set up menu bar
  - Restore workspace state from config

**Application::run() - Main Event Loop:**
- Location: `src/app/application.h`, `src/app/application.cpp`
- Triggers: Called by main() after init()
- Responsibilities:
  - Poll SDL events → processEvents()
  - Update (import progress, config changes) → update()
  - Render (menu bar, all panels, dialogs) → render()
  - Return exit code when m_running becomes false

**Panel::render() - Frame-Based UI Rendering:**
- Location: `src/ui/panels/panel.h`, each concrete panel subclass
- Triggers: Called by Application each frame for visible panels
- Responsibilities: ImGui calls to render UI and handle interaction; query Workspace and managers; invoke callbacks

## Error Handling

**Strategy:** Optional return types (std::optional<T>) and Result<T> type alias for operations that may fail

**Patterns:**
- Database operations return bool: `Database::open()`, `Database::execute()`
- Load operations return LoadResult struct: `{ MeshPtr mesh; std::string error; }`
- Manager operations return std::optional<T>: `LibraryManager::getModel(modelId) -> std::optional<ModelRecord>`
- Import workflow tracks ImportTask state with enum flags (pending, hashing, loading, storing, thumbnail)
- File I/O wrapped in FileUtils with error strings propagated to UI dialogs

## Cross-Cutting Concerns

**Logging:**
- Utility: `src/core/utils/log.h`
- Approach: Centralized log functions (Log, LogWarning, LogError) writing to stdout and rotating log files
- Usage: Every manager, loader, and database operation logs significant events

**Validation:**
- Mesh validation: `src/core/mesh/mesh.h::validate()` checks for NaN, degenerate triangles, out-of-bounds indices
- File validation: Loaders check file magic bytes and structure before parsing
- Database validation: Schema version checked on open; transactions used for atomic operations

**Authentication:**
- Not applicable — single-user desktop application

**Threading:**
- ImportQueue runs background worker thread for I/O and parsing
- All main-thread work (GL uploads, UI rendering) happens in Application::run()
- Mutex protects ImportQueue's task queues between worker and main threads
- ThumbnailGenerator uses Framebuffer created in worker context but rendered in main context

---

*Architecture analysis: 2026-02-08*
