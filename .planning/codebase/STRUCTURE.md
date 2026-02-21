# Codebase Structure

**Analysis Date:** 2026-02-19

## Directory Layout

```
digital_workshop/
├── src/                          # Main source code
│   ├── main.cpp                  # Application entry point
│   ├── app/                      # Application lifecycle and coordination
│   │   ├── application.h         # Main Application class
│   │   ├── application.cpp       # Application implementation
│   │   ├── workspace.h           # Focused object state model
│   │   ├── workspace.cpp
│   │   ├── window.h              # SDL window configuration
│   │   └── window.cpp
│   ├── managers/                 # Business logic coordinators
│   │   ├── ui_manager.h          # UI panel/dialog ownership and rendering
│   │   ├── ui_manager.cpp
│   │   ├── file_io_manager.h     # Import/export/project orchestration
│   │   ├── file_io_manager.cpp
│   │   ├── config_manager.h      # Configuration management and hot-reload
│   │   └── config_manager.cpp
│   ├── core/                     # Core business domain and services
│   │   ├── types.h               # Type aliases (i64, Vec3, Mat4, etc.)
│   │   ├── types.cpp
│   │   ├── database/             # SQLite database layer
│   │   │   ├── database.h        # RAII SQLite wrapper
│   │   │   ├── database.cpp
│   │   │   ├── connection_pool.h # Thread-safe DB connection pooling
│   │   │   ├── connection_pool.cpp
│   │   │   ├── schema.h          # Database schema initialization
│   │   │   ├── schema.cpp
│   │   │   ├── model_repository.h    # Model CRUD and search
│   │   │   ├── model_repository.cpp
│   │   │   ├── project_repository.h  # Project CRUD and model associations
│   │   │   ├── project_repository.cpp
│   │   │   ├── material_repository.h # Material record management
│   │   │   ├── material_repository.cpp
│   │   │   ├── cost_repository.h     # Cost estimate storage
│   │   │   ├── cost_repository.cpp
│   │   │   ├── gcode_repository.h    # G-code file records
│   │   │   └── gcode_repository.cpp
│   │   ├── library/              # Model library operations
│   │   │   ├── library_manager.h
│   │   │   └── library_manager.cpp
│   │   ├── project/              # Project domain model
│   │   │   ├── project.h         # Project class and ProjectManager
│   │   │   └── project.cpp
│   │   ├── import/               # Multi-stage file import pipeline
│   │   │   ├── import_task.h     # ImportTask struct, stages, progress
│   │   │   ├── import_queue.h    # Background import worker
│   │   │   ├── import_queue.cpp
│   │   │   ├── file_handler.h    # File reading utilities
│   │   │   └── file_handler.cpp
│   │   ├── mesh/                 # 3D geometry representation
│   │   │   ├── mesh.h            # Mesh class with vertices/triangles
│   │   │   ├── mesh.cpp
│   │   │   ├── vertex.h          # Vertex struct with custom hash
│   │   │   ├── bounds.h          # Bounding box and spatial queries
│   │   │   ├── hash.h            # Content-based mesh hashing
│   │   │   └── hash.cpp
│   │   ├── gcode/                # G-code parsing and analysis
│   │   │   ├── gcode_parser.h    # Line-by-line G-code parser
│   │   │   ├── gcode_parser.cpp
│   │   │   ├── gcode_analyzer.h  # Statistics, move type classification
│   │   │   └── gcode_analyzer.cpp
│   │   ├── loaders/              # Multi-format file loaders
│   │   │   ├── loader_factory.h  # Loader selection and dispatch
│   │   │   ├── loader_factory.cpp
│   │   │   ├── loader.h          # MeshLoader interface
│   │   │   ├── stl_loader.h      # STL format loader
│   │   │   ├── stl_loader.cpp
│   │   │   ├── obj_loader.h      # OBJ format loader
│   │   │   ├── obj_loader.cpp
│   │   │   ├── threemf_loader.h  # 3MF format loader
│   │   │   ├── threemf_loader.cpp
│   │   │   ├── gcode_loader.h    # G-code loader (specialized)
│   │   │   └── gcode_loader.cpp
│   │   ├── config/               # Configuration system
│   │   │   ├── config.h          # Configuration singleton
│   │   │   ├── config.cpp
│   │   │   ├── config_watcher.h  # File change detection
│   │   │   ├── config_watcher.cpp
│   │   │   ├── input_binding.h   # Keyboard binding definitions
│   │   │   └── input_binding.cpp
│   │   ├── events/               # Event bus for decoupled communication
│   │   │   └── event_bus.h       # Type-safe event bus (main thread only)
│   │   ├── threading/            # Multi-threading utilities
│   │   │   ├── thread_pool.h     # Generic bounded thread pool
│   │   │   ├── thread_pool.cpp
│   │   │   ├── main_thread_queue.h   # FIFO queue for main thread callbacks
│   │   │   ├── main_thread_queue.cpp
│   │   │   ├── thread_utils.h    # Main thread assertions, helper functions
│   │   │   ├── thread_utils.cpp
│   │   │   └── loading_state.h   # Async model loading state (no .cpp)
│   │   ├── optimizer/            # 2D cutting plan optimization
│   │   │   ├── cut_optimizer.h   # Optimizer orchestration
│   │   │   ├── cut_optimizer.cpp
│   │   │   ├── bin_packer.h      # Guillotine-based bin packing
│   │   │   ├── bin_packer.cpp
│   │   │   ├── guillotine.h      # Rectangle packing algorithm
│   │   │   └── guillotine.cpp
│   │   ├── paths/                # Cross-platform application paths
│   │   │   ├── app_paths.h
│   │   │   └── app_paths.cpp
│   │   ├── utils/                # Utility functions and helpers
│   │   │   ├── log.h             # Logging framework
│   │   │   ├── log.cpp
│   │   │   ├── file_utils.h      # File I/O helpers
│   │   │   ├── file_utils.cpp
│   │   │   ├── string_utils.h    # String parsing/manipulation
│   │   │   └── string_utils.cpp
│   │   ├── archive/              # Archive file handling
│   │   │   ├── archive.h         # ZIP/TAR extraction utilities
│   │   │   └── archive.cpp
│   │   ├── export/               # Model export functionality
│   │   │   ├── model_exporter.h
│   │   │   └── model_exporter.cpp
│   ├── render/                   # OpenGL 3.3 rendering system
│   │   ├── renderer.h            # Main renderer class
│   │   ├── renderer.cpp
│   │   ├── camera.h              # Orbit-based 3D camera
│   │   ├── camera.cpp
│   │   ├── shader.h              # GLSL shader wrapper
│   │   ├── shader.cpp
│   │   ├── gl_utils.h            # GL error checking, context info
│   │   ├── gl_utils.cpp
│   │   ├── framebuffer.h         # Offscreen rendering (FBO)
│   │   ├── framebuffer.cpp
│   │   ├── thumbnail_generator.h # Offline model thumbnail rendering
│   │   └── thumbnail_generator.cpp
│   ├── ui/                       # ImGui-based user interface
│   │   ├── theme.h               # ImGui theme colors and variants
│   │   ├── theme.cpp
│   │   ├── panels/               # Main UI panels
│   │   │   ├── panel.h           # Panel base class
│   │   │   ├── viewport_panel.h  # 3D model visualization
│   │   │   ├── viewport_panel.cpp
│   │   │   ├── library_panel.h   # Model library browser
│   │   │   ├── library_panel.cpp
│   │   │   ├── properties_panel.h # Model metadata editor
│   │   │   ├── properties_panel.cpp
│   │   │   ├── project_panel.h   # Project model list
│   │   │   ├── project_panel.cpp
│   │   │   ├── gcode_panel.h     # G-code analysis and visualization
│   │   │   ├── gcode_panel.cpp
│   │   │   ├── cut_optimizer_panel.h # 2D optimization interface
│   │   │   ├── cut_optimizer_panel.cpp
│   │   │   ├── start_page.h      # Welcome/quick start panel
│   │   │   └── start_page.cpp
│   │   ├── dialogs/              # Modal dialog windows
│   │   │   ├── dialog.h          # Dialog base class
│   │   │   ├── file_dialog.h     # File open/save dialogs
│   │   │   ├── file_dialog.cpp
│   │   │   ├── lighting_dialog.h # Light configuration
│   │   │   ├── lighting_dialog.cpp
│   │   │   ├── import_summary_dialog.h # Import results summary
│   │   │   ├── import_summary_dialog.cpp
│   │   │   ├── message_dialog.h  # Simple message boxes
│   │   │   └── message_dialog.cpp
│   │   └── widgets/              # Reusable UI components
│   │       ├── binding_recorder.h    # Interactive key binding capture
│   │       ├── binding_recorder.cpp
│   │       ├── status_bar.h      # Bottom status bar (import progress, info)
│   │       ├── status_bar.cpp
│   │       ├── toast.h           # Floating notification messages
│   │       ├── toast.cpp
│   │       ├── canvas_2d.h       # 2D drawing widget
│   │       └── canvas_2d.cpp
│   └── CMakeLists.txt            # Main build configuration
├── tests/                        # Test suite
│   ├── CMakeLists.txt
│   └── test_*.cpp                # Individual test files
├── build/                        # Build output (generated)
├── CMakeLists.txt                # Root CMake configuration
└── README.md                     # Project documentation
```

## Directory Purposes

**src/:**
- Purpose: All source code for the application
- Contains: Headers (.h), implementations (.cpp)
- Key files: `main.cpp` (entry), `CMakeLists.txt` (build configuration)

**src/app/:**
- Purpose: Application lifecycle coordination and workspace state
- Contains: Application class (init, run loop, shutdown), Workspace (focused object tracking)
- Key files: `application.h`, `application.cpp`, `workspace.h`

**src/managers/:**
- Purpose: Business logic orchestrators (god class decomposition)
- Contains: UIManager (UI coordination), FileIOManager (import/export/project), ConfigManager (settings)
- Key files: `ui_manager.h`, `file_io_manager.h`, `config_manager.h`

**src/core/:**
- Purpose: Domain models and core services
- Contains: Database layer, LibraryManager, Project system, Import pipeline, Mesh geometry, G-code parser, Loaders, Configuration, Threading, Events
- Key files: Type system (`types.h`), core abstractions

**src/core/database/:**
- Purpose: SQLite database abstraction layer
- Contains: RAII Database wrapper, connection pooling, schema initialization, specialized repositories (Model, Project, Material, GCode, Cost)
- Key files: `database.h`, `schema.h`, repository headers

**src/core/library/:**
- Purpose: Model library operations (unified interface)
- Contains: LibraryManager with import, search, tag management, duplicate detection, thumbnail generation
- Key files: `library_manager.h`, `library_manager.cpp`

**src/core/project/:**
- Purpose: Project domain model and lifecycle
- Contains: Project class (metadata, model associations), ProjectManager (CRUD)
- Key files: `project.h`, `project.cpp`

**src/core/import/:**
- Purpose: Multi-stage file import pipeline
- Contains: ImportQueue (background worker), ImportTask struct (pipeline stages), progress tracking, file handler utilities
- Key files: `import_task.h`, `import_queue.h`, `import_queue.cpp`

**src/core/mesh/:**
- Purpose: 3D geometry representation and operations
- Contains: Mesh class (vertices, triangles, normals, bounds), Vertex struct, auto-orientation for relief models, content-based hashing
- Key files: `mesh.h`, `vertex.h`, `hash.h`

**src/core/gcode/:**
- Purpose: G-code parsing and analysis
- Contains: GCodeParser (line-by-line), GCodeAnalyzer (statistics, time estimation, move classification)
- Key files: `gcode_parser.h`, `gcode_analyzer.h`

**src/core/loaders/:**
- Purpose: Multi-format file loading (strategy pattern)
- Contains: LoaderFactory (dispatch), MeshLoader interface, format-specific loaders (STL, OBJ, 3MF, GCode)
- Key files: `loader_factory.h`, `loader.h`

**src/core/config/:**
- Purpose: Configuration system with persistence and hot-reload
- Contains: Config singleton (INI-based), ConfigWatcher (file change detection), InputBinding (keyboard configuration)
- Key files: `config.h`, `config_watcher.h`

**src/core/events/:**
- Purpose: Decoupled subsystem communication
- Contains: EventBus (type-safe publish/subscribe, main thread only)
- Key files: `event_bus.h`

**src/core/threading/:**
- Purpose: Multi-threading infrastructure
- Contains: ThreadPool (generic worker pool), MainThreadQueue (callback queue), thread utilities (main thread assertions)
- Key files: `thread_pool.h`, `main_thread_queue.h`, `thread_utils.h`

**src/core/optimizer/:**
- Purpose: 2D cutting plan optimization
- Contains: CutOptimizer, BinPacker (guillotine algorithm), placement logic
- Key files: `cut_optimizer.h`, `bin_packer.h`

**src/core/paths/:**
- Purpose: Cross-platform application directory management
- Contains: Functions for config dir, data dir, cache dir, database path, etc.
- Key files: `app_paths.h`

**src/core/utils/:**
- Purpose: Shared utility functions
- Contains: Logging framework, file I/O helpers, string parsing
- Key files: `log.h`, `file_utils.h`, `string_utils.h`

**src/render/:**
- Purpose: OpenGL 3.3 rendering system
- Contains: Renderer (mesh/grid/axis), Camera (orbit navigation), Shader (GLSL wrapper), Framebuffer (FBO), ThumbnailGenerator (offscreen rendering)
- Key files: `renderer.h`, `camera.h`, `thumbnail_generator.h`

**src/ui/:**
- Purpose: ImGui-based user interface
- Contains: Panels (viewport, library, properties, project, gcode, optimizer, start page), Dialogs (file, lighting, import summary, message), Widgets (status bar, toast, binding recorder), Theme system
- Key files: `theme.h` (color palette), `panels/`, `dialogs/`, `widgets/`

**tests/:**
- Purpose: Unit and integration tests
- Contains: Test files matching source files (e.g., `test_material_repository.cpp`)
- Key files: `CMakeLists.txt` (test build config)

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Application entry point; creates Application instance
- `src/app/application.h`: Application class definition with init() and run() methods
- `src/app/application.cpp`: Application initialization, main loop (processEvents → update → render)

**Configuration:**
- `src/CMakeLists.txt`: Main executable build configuration
- `CMakeLists.txt`: Root CMake with dependencies (SDL2, OpenGL, SQLite3, GLM, etc.)

**Core Logic:**
- `src/app/workspace.h`: Focused object state (mesh, g-code, toolpath, cut plan)
- `src/managers/ui_manager.h`: UI panel/dialog ownership and frame rendering
- `src/managers/file_io_manager.h`: Import/export/project orchestration
- `src/core/library/library_manager.h`: Model library operations
- `src/core/import/import_queue.h`: Multi-stage import pipeline
- `src/core/database/schema.h`: Database schema initialization and versioning

**Testing:**
- `tests/test_material_repository.cpp`: Material repository tests
- `tests/CMakeLists.txt`: Test build configuration (GoogleTest framework)

## Naming Conventions

**Files:**
- `.h` for headers, `.cpp` for implementations
- Snake_case for filenames (e.g., `viewport_panel.h`, `config_watcher.cpp`)
- Grouped logically by directory (e.g., all panel headers in `ui/panels/`)

**Directories:**
- Lowercase, descriptive names
- One concept per directory (e.g., `database/`, `library/`, `loaders/`)
- Nested by layer (e.g., `core/threading/`, `core/database/`)

**Classes:**
- PascalCase (e.g., `ViewportPanel`, `ImportQueue`, `ModelRepository`)
- Manager classes suffixed with "Manager" (e.g., `UIManager`, `FileIOManager`, `ConfigManager`)
- Repository classes suffixed with "Repository" (e.g., `ModelRepository`, `ProjectRepository`)
- Panel classes suffixed with "Panel" (e.g., `ViewportPanel`, `LibraryPanel`)
- Dialog classes suffixed with "Dialog" (e.g., `FileDialog`, `ImportSummaryDialog`)

**Functions:**
- camelCase (e.g., `importModel()`, `getAllModels()`, `setFocusedMesh()`)
- Getter methods follow pattern `get*()` (e.g., `getModel()`, `getFocusedMesh()`)
- Boolean accessors use `is*()` or `has*()` (e.g., `isOpen()`, `hasFocusedMesh()`)

**Namespaces:**
- Single namespace `dw` (Digital Workshop)
- Sub-namespaces for utilities (e.g., `dw::paths`, `dw::log`, `dw::threading`)

## Where to Add New Code

**New Feature (e.g., Materials Mapping):**
- Primary code: `src/core/materials/` (MaterialRepository already exists at `src/core/database/material_repository.h`)
- Domain model: `src/core/database/model_repository.h` (extend ModelRecord, add material associations)
- UI Panel: `src/ui/panels/materials_panel.h` (new panel inheriting from Panel base class)
- Manager integration: Update `src/managers/ui_manager.h` to own new panel
- Tests: `tests/test_materials_*.cpp`

**New Database Repository:**
- Implementation: `src/core/database/{name}_repository.h` and `.cpp`
- Schema: Add table definition to `src/core/database/schema.cpp` (Schema::initialize)
- Usage: Instantiate in relevant manager or Application class

**New File Format Loader:**
- Implementation: `src/core/loaders/{format}_loader.h` and `.cpp`
- Interface: Inherit from MeshLoader (pure virtual load())
- Registration: Update LoaderFactory.getLoaderByExtension() to return new loader

**New Threading Utility:**
- Implementation: Add to `src/core/threading/thread_utils.h`
- Usage: Include thread_utils.h in file needing thread assertion

**New UI Component:**
- Base class inheritance: Panel for main panels, Dialog for modals, or Widget for reusable components
- Location: `src/ui/panels/`, `src/ui/dialogs/`, or `src/ui/widgets/`
- Ownership: Register in UIManager constructor

**New Configuration Option:**
- Definition: Add to `Config` class in `src/core/config/config.h`
- Persistence: Add getter/setter, handle serialization in load()/save()
- Hot-reload: ConfigWatcher automatically notifies on file change

## Special Directories

**build/:**
- Purpose: Build output directory (generated by CMake)
- Generated: Yes
- Committed: No (included in .gitignore)

**.planning/:**
- Purpose: GSD planning documents (codebase maps, phase plans, execution logs)
- Generated: Yes (by GSD orchestrator)
- Committed: Yes (tracked in git for context)

**src/ui/dialogs/ and src/ui/panels/:**
- Purpose: ImGui UI components (not a web framework)
- Committed: Yes
- Special note: All rendering done via ImGui API; no retained-mode scene graph

---

*Structure analysis: 2026-02-19*
