# Digital Workshop - Comprehensive Development Plan

## Current Status

**Scaffold: ~60% Complete**
- Build system: Done
- Core app structure: Done
- Rendering: Not started
- UI: Not started
- Core modules: Not started

---

## Task Summary (20 Tasks with Dependencies)

| ID | Task | Blocked By |
|----|------|------------|
| 1 | Core utilities and path management | - |
| 2 | Database layer (library backbone) | #1 |
| 3 | Mesh data structures | #1 |
| 4 | File loaders (STL, OBJ, 3MF) | #1, #3 |
| 5 | Rendering system | #1, #3 |
| 6 | Project system | #1, #2 |
| 7 | Library management system | #2, #4, #6 |
| 8 | Archive export/import | #6, #7 |
| 9 | Model export | #3, #4 |
| 10 | UI framework | #1, #5 |
| 11 | Polished ImGui theme | #10 |
| 12 | UI panels - Library | #7, #10 |
| 13 | UI panels - Viewport | #5, #10 |
| 14 | UI panels - Properties | #3, #10 |
| 15 | UI panels - Project | #6, #10 |
| 16 | UI dialogs | #10, #11 |
| 17 | G-code parser and analyzer | #1 |
| 18 | 2D cut optimizer | #1 |
| 19 | Integration and workflow | #7-18 (all features) |
| 20 | Git repository setup | #19 |

---

## Phase 1: Foundation (Task #1)

### 1.1 Core Utilities
- [ ] src/core/types.h - Vec3, Mat4, Path aliases
- [ ] src/core/utils/log.h/cpp - Logging
- [ ] src/core/utils/file_utils.h/cpp - File I/O
- [ ] src/core/utils/string_utils.h/cpp - String helpers
- [ ] src/core/utils/paths.h/cpp - Path management

### 1.2 Path Management
Standard XDG-style paths:
- Config: `~/.config/digitalworkshop/` - App settings
- Data: `~/.local/share/digitalworkshop/` - Database, thumbnails, cache
- User: `~/Documents/DigitalWorkshop/` - User projects

---

## Phase 2: Database Layer (Task #2)

### 2.1 SQLite Wrapper
- [ ] src/core/database/database.h/cpp - Connection, transactions
- [ ] src/core/database/schema.h/cpp - Tables (no migrations needed)

### 2.2 Repositories
- [ ] src/core/database/model_repository.h/cpp - Model CRUD
- [ ] src/core/database/project_repository.h/cpp - Project CRUD

### 2.3 Schema
```sql
-- Models table (library backbone)
CREATE TABLE models (
    id INTEGER PRIMARY KEY,
    hash TEXT UNIQUE NOT NULL,    -- xxHash for deduplication
    name TEXT NOT NULL,
    file_path TEXT NOT NULL,      -- Source file (original import)
    file_format TEXT NOT NULL,
    vertex_count INTEGER,
    triangle_count INTEGER,
    bounds_min_x, bounds_min_y, bounds_min_z REAL,
    bounds_max_x, bounds_max_y, bounds_max_z REAL,
    thumbnail_path TEXT,
    imported_at TEXT DEFAULT CURRENT_TIMESTAMP,
    tags TEXT                     -- JSON array
);

-- Projects table
CREATE TABLE projects (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    modified_at TEXT DEFAULT CURRENT_TIMESTAMP
);

-- Project-Model junction
CREATE TABLE project_models (
    project_id INTEGER REFERENCES projects(id),
    model_id INTEGER REFERENCES models(id),
    sort_order INTEGER DEFAULT 0,
    PRIMARY KEY (project_id, model_id)
);
```

---

## Phase 3: Mesh System (Tasks #3, #4)

### 3.1 Data Structures
- [ ] src/core/mesh/vertex.h - Vertex struct
- [ ] src/core/mesh/mesh.h/cpp - Mesh class
- [ ] src/core/mesh/bounds.h/cpp - AABB
- [ ] src/core/mesh/mesh_utils.h/cpp - Utilities
- [ ] src/core/mesh/hash.h/cpp - xxHash wrapper

### 3.2 File Loaders
- [ ] src/core/loaders/loader.h - Interface
- [ ] src/core/loaders/loader_factory.h/cpp
- [ ] src/core/loaders/stl_loader.h/cpp
  - Binary STL parsing
  - ASCII STL parsing
  - Auto-detect format
- [ ] src/core/loaders/obj_loader.h/cpp
  - Vertex parsing (v, vt, vn)
  - Face parsing with triangulation
  - MTL file parsing
- [ ] src/core/loaders/threemf_loader.h/cpp
  - ZIP extraction
  - XML parsing

---

## Phase 4: Rendering System (Task #5)

- [ ] src/render/renderer.h/cpp - OpenGL state, mesh rendering
- [ ] src/render/shader.h/cpp - Compilation, uniforms
- [ ] src/render/shader_sources.h - Embedded GLSL
- [ ] src/render/camera.h/cpp - Orbit camera, pan/zoom
- [ ] src/render/grid_renderer.h/cpp - Workspace grid
- [ ] src/render/framebuffer.h/cpp - Offscreen rendering (thumbnails)
- [ ] src/render/gl_utils.h - Error checking

---

## Phase 5: Project System (Task #6)

- [ ] src/core/project/project.h/cpp - Project data model
- [ ] src/core/project/project_manager.h/cpp - CRUD operations

Features:
- Create/Open/Save/Close projects
- Add/Remove models from project
- Project metadata (name, description)
- Model ordering within project

---

## Phase 6: Library Management (Task #7)

- [ ] src/core/library/library_manager.h/cpp
- [ ] src/core/library/import_pipeline.h/cpp
- [ ] src/core/library/thumbnail_generator.h/cpp
- [ ] src/core/library/deduplication.h/cpp

Import Pipeline:
1. File validation (format check)
2. Hash computation (xxHash)
3. Duplicate check (query DB by hash)
4. Parse geometry (loader)
5. Generate thumbnail (offscreen render)
6. Store in database
7. Copy source file to data directory

Search/Filter:
- By name (fuzzy match)
- By format (STL, OBJ, 3MF)
- By date range
- By tags
- By vertex/triangle count range

---

## Phase 7: Export Systems (Tasks #8, #9)

### 7.1 Archive Export/Import
- [ ] src/core/archive/archive_exporter.h/cpp
- [ ] src/core/archive/archive_importer.h/cpp

Archive format (ZIP):
```
project_name.dwproj/
├── project.json          # Metadata
├── models/               # Included model files
│   ├── model1.stl
│   └── model2.obj
└── thumbnails/           # Optional
```

### 7.2 Model Export
- [ ] src/core/export/model_exporter.h/cpp
  - Export to STL (binary)
  - Export to OBJ

---

## Phase 8: UI Framework (Tasks #10, #11)

### 8.1 UI Framework
- [ ] src/ui/ui_manager.h/cpp - ImGui init, docking
- [ ] src/ui/layout.h/cpp - Dock layout management
- [ ] src/ui/icons.h - Icon definitions

### 8.2 Polished Theme
- [ ] src/ui/theme.h/cpp - Custom theme (not default ImGui)
  - Refined colors (subtle grays, accent colors)
  - Proper spacing and padding
  - Rounded corners
  - Custom widget styling
  - PySide/Qt-like appearance

---

## Phase 9: UI Panels (Tasks #12-15)

### 9.1 Base Panel
- [ ] src/ui/panels/panel.h - Base interface

### 9.2 Library Panel
- [ ] src/ui/panels/library_panel.h/cpp
  - Thumbnail grid view
  - List view option
  - Search bar
  - Filter dropdown
  - Import button
  - Context menu (add to project, delete, export)

### 9.3 Viewport Panel
- [ ] src/ui/panels/viewport_panel.h/cpp
  - 3D model viewer
  - Camera controls (orbit, pan, zoom)
  - View options (wireframe, solid, textured)
  - Grid toggle
  - Axis indicator

### 9.4 Properties Panel
- [ ] src/ui/panels/properties_panel.h/cpp
  - Model info (vertices, triangles, bounds)
  - File info (path, format, size)
  - Tags editor
  - Thumbnail preview

### 9.5 Project Panel
- [ ] src/ui/panels/project_panel.h/cpp
  - Project info (name, description)
  - Model list (reorderable)
  - Add/Remove buttons
  - Export project button

---

## Phase 10: UI Dialogs (Task #16)

- [ ] src/ui/dialogs/import_dialog.h/cpp
- [ ] src/ui/dialogs/export_dialog.h/cpp
- [ ] src/ui/dialogs/settings_dialog.h/cpp
- [ ] src/ui/dialogs/about_dialog.h/cpp
- [ ] src/ui/dialogs/new_project_dialog.h/cpp

---

## Phase 11: G-Code System (Task #17)

- [ ] src/core/gcode/gcode_types.h - Enums, structs
- [ ] src/core/gcode/gcode_parser.h/cpp
  - Line-by-line parsing
  - G0/G1 linear moves
  - G2/G3 arc moves
  - M commands
  - Unit handling (G20/G21)
- [ ] src/core/gcode/gcode_analyzer.h/cpp
  - Total path length
  - Time estimation
  - Tool change count
- [ ] src/core/gcode/gcode_path.h/cpp - For rendering
- [ ] src/ui/panels/gcode_panel.h/cpp - G-code analysis display

---

## Phase 12: Cut Optimizer (Task #18)

- [ ] src/core/optimizer/sheet.h - Sheet/Part definitions
- [ ] src/core/optimizer/cut_plan.h/cpp - Result structure
- [ ] src/core/optimizer/cut_optimizer.h - Interface
- [ ] src/core/optimizer/bin_packer.h/cpp - FFD algorithm
- [ ] src/core/optimizer/guillotine.h/cpp - Guillotine algorithm
- [ ] src/ui/panels/optimizer_panel.h/cpp - Cut planning UI

---

## Phase 13: Integration (Task #19)

- [ ] Connect all panels to workspace
- [ ] File menu: New/Open/Save/Close Project, Import, Export
- [ ] Edit menu: Preferences
- [ ] Project menu: Add/Remove Model, Export Archive
- [ ] Drag-and-drop file import
- [ ] Keyboard shortcuts
- [ ] Error handling with user-friendly messages
- [ ] Progress dialogs for long operations

Testing:
- [ ] Full workflow testing (create project, import, modify, save)
- [ ] Large file handling (600MB+)
- [ ] Stress testing with many models

---

## Phase 14: Git Setup (Task #20)

- [ ] Initialize git repository
- [ ] Create .gitignore (build/, *.db, .ralph/logs/, imgui.ini, .cache/)
- [ ] Create README.md with build instructions and screenshots
- [ ] Initial commit

---

## Completed

- [x] Rulebook created
- [x] Phase 1.1 Configuration files (.clang-format, .clang-tidy, .gitignore)
- [x] Phase 1.2 CMake build system
- [x] Phase 1.3 Core application (main.cpp, window, workspace)
- [x] Project builds and runs (6MB executable)

---

## Notes

- **Theme Priority**: User wants polished look (not default ImGui)
- **Database First**: Library is backed by database, not filesystem
- **Path Separation**: App files vs user files properly separated
- **No cross-version compatibility**: Delete DB/config freely
- **Use spectacle for visual verification**
- Source files (STL/OBJ/3MF) are the permanent record
