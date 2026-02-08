# Codebase Structure

**Analysis Date:** 2026-02-08

## Directory Layout

```
/data/DW/
├── src/                    # Main application source code
│   ├── main.cpp            # Entry point
│   ├── app/                # Application lifecycle and workspace
│   ├── core/               # Domain logic and data access
│   ├── render/             # OpenGL rendering
│   └── ui/                 # ImGui interface
├── tests/                  # Test suite (CMake-based)
├── settings/               # Separate settings application (Qt/native UI)
├── cmake/                  # CMake modules and macros
├── packaging/              # CPack installer configurations
├── rulebook/               # Documentation and design decisions
├── CMakeLists.txt          # Root build configuration
├── .clang-format           # Code style configuration
├── .clang-tidy             # Static analysis configuration
├── .github/                # GitHub Actions workflows
└── .planning/              # GSD planning documents (this directory)
```

## Directory Purposes

**`src/app/`:**
- Purpose: Application lifecycle, main window, and focused object state management
- Contains: Application main loop, SDL2 window setup, Workspace state holder, top-level panel coordination
- Key files: `application.h/cpp`, `window.h/cpp`, `workspace.h/cpp`

**`src/core/`:**
- Purpose: Domain logic, repositories, managers, and utilities
- Contains: Mesh geometry, database access, file loading, project management, import pipeline, G-code parsing, cut optimization
- Key subdirectories:
  - `archive/` — ZIP/archive handling for project files
  - `config/` — Configuration loading, input binding, file watching
  - `database/` — SQLite wrapper, repositories (Model, Project, Cost), schema
  - `export/` — Mesh export to STL/OBJ/3MF
  - `gcode/` — G-code parsing and analysis
  - `import/` — Background import queue with deduplication
  - `library/` — Library manager for model CRUD
  - `loaders/` — Pluggable mesh format loaders (STL, OBJ, 3MF)
  - `mesh/` — Mesh class, vertex structure, bounds, hash computation
  - `optimizer/` — 2D nesting optimizer (guillotine, bin-packing)
  - `paths/` — Application directory and data folder management
  - `project/` — Project domain model and manager
  - `utils/` — Logging, file utilities, string utilities

**`src/render/`:**
- Purpose: OpenGL 3.3 graphics rendering with GPU mesh cache and offscreen rendering
- Contains: Renderer, Shader, Camera, Framebuffer, ThumbnailGenerator
- Key files: `renderer.h/cpp`, `shader.h/cpp`, `camera.h/cpp`, `framebuffer.h/cpp`, `thumbnail_generator.h/cpp`

**`src/ui/`:**
- Purpose: ImGui-based immediate-mode UI with dockable panels
- Contains: UIManager, base Panel class, concrete panels, dialogs, theme system, icon constants
- Key subdirectories:
  - `dialogs/` — FileDialog, MessageDialog, ConfirmDialog, LightingDialog
  - `panels/` — ViewportPanel, LibraryPanel, ProjectPanel, PropertiesPanel, GCodePanel, CutOptimizerPanel, StartPage
  - `widgets/` — Custom ImGui widgets (BindingRecorder, Canvas2D)
- Key files: `ui_manager.h/cpp`, `theme.h/cpp`, `icons.h`, `panel.h`

**`tests/`:**
- Purpose: Unit and integration test suite
- Contains: Test targets for core modules
- Build: CMake-configured with CTest runner

**`settings/`:**
- Purpose: Separate application for configuration (color themes, input bindings, UI scale)
- Contains: Independent executable that modifies config files; main app watches for changes and reloads

**`cmake/`:**
- Purpose: Reusable CMake modules and macros
- Contains: CompilerFlags.cmake, Dependencies.cmake, GitVersion.cmake, custom build configuration functions

**`packaging/`:**
- Purpose: Installer and package generation configuration
- Contains: CPack templates for NSIS (Windows), makeself (Linux), DMG (macOS)

**`rulebook/`:**
- Purpose: Design documentation, decision records, and development guidelines
- Contains: Architecture decisions, code quality standards, project vision

## Key File Locations

**Entry Points:**
- `src/main.cpp` — Program entry; creates Application and calls init()/run()
- `src/app/application.h/cpp` — Main application class with event loop and panel orchestration
- `CMakeLists.txt` — Root build configuration; defines targets and dependencies

**Configuration:**
- `CMakeLists.txt` — Build system configuration
- `.clang-format` — Code formatting rules (LLVM style with custom modifications)
- `.clang-tidy` — Static analysis configuration for linting warnings
- `cmake/CompilerFlags.cmake` — Warning levels, optimization flags per platform
- `cmake/Dependencies.cmake` — FetchContent declarations for imgui, glad, glm, sqlite3, zlib, sdl2

**Core Logic:**
- `src/core/database/database.h` — SQLite wrapper with RAII statements and transactions
- `src/core/library/library_manager.h` — Model import, search, deduplication, thumbnail generation
- `src/core/project/project.h` — Project lifecycle and model associations
- `src/core/import/import_queue.h` — Background import worker thread with duplicate detection
- `src/core/mesh/mesh.h` — 3D geometry container with operations
- `src/core/loaders/loader_factory.h` — Format-agnostic mesh loading (STL, OBJ, 3MF)

**Rendering:**
- `src/render/renderer.h` — OpenGL context, mesh cache, scene rendering
- `src/render/camera.h` — Orbit camera with target-relative positioning
- `src/render/framebuffer.h` — Render-to-texture for thumbnails and viewport
- `src/render/shader.h` — RAII shader wrapper with uniform location caching

**UI Panels:**
- `src/ui/panels/panel.h` — Abstract base class for all panels
- `src/ui/panels/viewport_panel.h` — 3D visualization with camera control
- `src/ui/panels/library_panel.h` — Model browsing, search, thumbnails
- `src/ui/panels/project_panel.h` — Project tree, model management
- `src/ui/panels/properties_panel.h` — Metadata display for focused object

**UI Support:**
- `src/ui/ui_manager.h` — ImGui context and frame lifecycle
- `src/ui/theme.h` — Dark/light/high-contrast color schemes
- `src/ui/icons.h` — FontAwesome Unicode character constants

## Naming Conventions

**Files:**
- `.h` files (headers) use `snake_case`: `mesh.h`, `library_manager.h`
- `.cpp` files (implementation) match header names: `mesh.cpp`, `library_manager.cpp`
- Subdirectories use `snake_case`: `core/`, `ui/panels/`, `core/database/`
- Panels always end with `_panel`: `viewport_panel.h`, `library_panel.h`
- Repositories always end with `_repository`: `model_repository.h`, `project_repository.h`
- Dialogs always end with `_dialog`: `file_dialog.h`, `message_dialog.h`

**Classes/Types:**
- Classes use `PascalCase`: `Application`, `Mesh`, `Renderer`, `LibraryManager`
- Structs use `PascalCase`: `Vertex`, `Color`, `LoadResult`, `ImportTask`, `ModelRecord`
- Enums use `PascalCase` for type, `UPPER_CASE` for values: `enum class CommandType { RAPID_MOVE, LINEAR_MOVE }`
- Type aliases use `PascalCase`: `MeshPtr` (shared_ptr<Mesh>)
- Custom types in `src/core/types.h`: `Vec3`, `Mat4`, `i64`, `f32`, `Path`, `ByteBuffer`

**Functions/Methods:**
- Functions and methods use `camelCase`: `loadMesh()`, `setFocusedMesh()`, `renderMesh()`
- Getters use `get` prefix or property-style: `getMesh()` or `mesh()`
- Setters use `set` prefix: `setFocusedMesh()`
- Boolean predicates use `is` or `has` prefix: `isValid()`, `hasFocusedMesh()`, `isRunning()`
- Callbacks use `on` prefix: `onModelSelected()`, `onImportModel()`, `onFilesDropped()`

**Variables:**
- Member variables use `m_` prefix: `m_mesh`, `m_camera`, `m_focusedMesh`, `m_running`
- Static variables use `s_` prefix
- Local variables use `snake_case`: `mesh_data`, `vertex_count`
- Constants use `UPPER_CASE`: `DEFAULT_WIDTH`, `WINDOW_TITLE`

**Namespaces:**
- Primary namespace: `dw` (Digital Workshop)
- Subnamespaces: `dw::gcode`, used for domain-specific APIs

## Where to Add New Code

**New Feature (e.g., new model operation):**
- Primary code: Add to appropriate `src/core/` submodule (e.g., `src/core/mesh/` for geometry operations, `src/core/export/` for export formats)
- Manager/orchestration: Update relevant manager if needed (e.g., `LibraryManager` if affecting import workflow)
- UI: Add panel or button to existing panel in `src/ui/panels/`
- Tests: Create `.cpp` test file in `tests/` mirroring source structure
- Example: Adding STL export — implement in `src/core/export/model_exporter.h`, add button in `src/ui/panels/library_panel.cpp`, test in `tests/export_test.cpp`

**New Component/Module (e.g., new 3D format loader):**
- Implementation: Create in appropriate `src/core/` submodule directory
- File naming: Follow `snake_case.h/cpp` pattern, use descriptive names matching class
- Header structure: Include guards, forward declarations, single class definition
- Public API: Only expose what's necessary; keep implementation details private
- Example: New `GltfLoader` — create `src/core/loaders/gltf_loader.h/cpp`, inherit `MeshLoader`, register with `LoaderFactory`

**Utilities and Helpers:**
- Shared helpers: `src/core/utils/` (logging, file I/O, string operations)
- Math utilities: `src/core/types.h` (type aliases, common math functions)
- OpenGL utilities: `src/render/gl_utils.h` (error checking, debug output)
- ImGui utilities: `src/ui/ui_manager.h` or new file in `src/ui/` if substantial
- Example: New validation function — add to `src/core/utils/validation.h`, test in `tests/`

**Tests:**
- Location: Mirror source tree in `tests/` directory
- Naming: `[module]_test.cpp` (e.g., `mesh_test.cpp`, `database_test.cpp`)
- Structure: Use test framework configured in `tests/CMakeLists.txt`
- Coverage: Unit test for individual components; integration tests for manager workflows

**Configuration/Build:**
- CMake: Edit relevant `CMakeLists.txt` (root, `src/`, or submodule)
- Compiler flags: `cmake/CompilerFlags.cmake` (add warnings, optimization changes)
- Dependencies: `cmake/Dependencies.cmake` (add external libraries via FetchContent or find_package)
- Installer: `packaging/` (add files to install, modify NSIS/CPack templates)

## Special Directories

**`.planning/codebase/`:**
- Purpose: GSD-generated codebase analysis documents
- Generated: Yes (by `/gsd:map-codebase` command)
- Committed: Yes (documents are versioned with code)
- Contents: ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, TESTING.md, CONCERNS.md

**`.github/workflows/`:**
- Purpose: GitHub Actions CI/CD pipelines
- Generated: No (manually configured)
- Committed: Yes
- Contents: Build and test workflows for Linux, Windows, macOS

**`build/`:**
- Purpose: Out-of-tree CMake build directory
- Generated: Yes (created by cmake/make build process)
- Committed: No (in .gitignore)
- Contents: Object files, executables, CMakeCache.txt

**`cmake/`:**
- Purpose: Reusable CMake utilities
- Generated: No (manually written)
- Committed: Yes
- Key files: `CompilerFlags.cmake`, `Dependencies.cmake`, `GitVersion.cmake`

**`settings/`:**
- Purpose: Separate Qt/native UI application for configuration
- Generated: No (source code)
- Committed: Yes
- Note: Builds as separate executable; main app watches config files and reloads

**`rulebook/`:**
- Purpose: Design documentation and decision records
- Generated: No (manually written)
- Committed: Yes
- Contents: Architecture decisions, naming guidelines, code quality standards

---

*Structure analysis: 2026-02-08*
