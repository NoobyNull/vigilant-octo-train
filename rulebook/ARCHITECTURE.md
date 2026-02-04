# Architecture

## Technology Stack

| Layer | Technology | Purpose |
|-------|------------|---------|
| Windowing | SDL2 | Cross-platform windows, input, events |
| UI | Dear ImGui (docking) | Panels, dialogs, widgets |
| Rendering | OpenGL 3.3 | 3D viewport, thumbnails |
| Database | SQLite3 | Project data, model library |
| Build | CMake | Cross-platform builds |

## System Requirements

| Spec | Minimum | Recommended |
|------|---------|-------------|
| CPU | Intel Quad-core | Intel 10th gen+ |
| RAM | 8 GB | 16 GB+ |
| GPU | OpenGL 3.3 capable | Dedicated GPU |
| Storage | SSD recommended | SSD |

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                          │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                    UI Layer (ImGui)                   │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐  │  │
│  │  │ Library │ │Properties│ │ G-Code │ │  Optimizer  │  │  │
│  │  │  Panel  │ │  Panel  │ │  Panel  │ │    Panel    │  │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘  │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │              Central Viewport                   │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                   Core Layer (C++)                    │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐  │  │
│  │  │ Loaders │ │  Mesh   │ │ G-Code  │ │  Optimizer  │  │  │
│  │  │STL/OBJ/ │ │  Data   │ │ Parser  │ │  2D Cutting │  │  │
│  │  │  3MF    │ │         │ │ Timing  │ │  Bin Pack   │  │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                 Platform Layer                        │  │
│  │  SDL2 (window, input) │ OpenGL (render) │ SQLite (db) │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Core Principles

### Separation of Concerns
- **Core** has zero dependencies on UI or rendering
- **UI** only calls core APIs, never touches OpenGL directly
- **Render** is isolated, could be swapped for Vulkan later

### Object-Centric Design
The application revolves around a "focus" object:
```cpp
class Workspace {
    std::shared_ptr<Model> m_focusedModel;     // Current 3D model
    std::shared_ptr<GCodeFile> m_focusedGCode; // Current G-code
    std::shared_ptr<CutPlan> m_focusedPlan;    // Current cut plan

    // Panels observe and act on focused objects
};
```

### Simple Loading Strategy
Modern systems have plenty of RAM. Keep it simple:
```cpp
auto loadMesh(const std::filesystem::path& path) -> std::unique_ptr<Mesh> {
    // Read entire file
    auto data = readFile(path);

    // Parse into mesh
    auto mesh = parser.parse(data);

    // Upload to GPU
    mesh->uploadToGPU();

    return mesh;
}
```

No streaming, no chunking, no complexity. 600MB is ~7% of 8GB minimum RAM.

### No Cross-Version Compatibility

There is no user base. There is no legacy. Every build is a clean slate.

**What this means:**

| Data Type | Traditional Approach | Our Approach |
|-----------|---------------------|--------------|
| Database schema | Migration scripts | Delete DB, recreate |
| Config files | Version field + upgrade | Delete, regenerate defaults |
| Project files | Format converters | Re-import source files |
| Settings | Preserve user prefs | Defaults are fine |

**No migration code:**
```cpp
// Other projects - migration hell that grows forever
void upgradeDatabase() {
    if (version == 1) migrateV1toV2();
    if (version == 2) migrateV2toV3();
    if (version == 3) migrateV3toV4();
}

// Us - schema wrong? Start fresh.
void openDatabase(const Path& p) {
    if (!isCurrentSchema(p)) {
        std::filesystem::remove(p);
        createFreshDatabase(p);
    }
}
```

**No config versioning:**
```cpp
// Other projects
struct Settings {
    int version;
    void upgrade() {
        if (version < 2) { /* add fields */ }
        if (version < 3) { /* rename fields */ }
    }
};

// Us - invalid? Delete and use defaults.
auto loadSettings(const Path& p) -> Settings {
    auto settings = tryParse(p);
    if (!settings) {
        std::filesystem::remove(p);
        return Settings{};  // Defaults
    }
    return *settings;
}
```

**Benefits:**
- Change database schema freely
- Rename/restructure anything without worry
- No dead upgrade code accumulating
- Simpler, cleaner data layer
- Source files (STL, OBJ, 3MF) are the permanent record

### Versioning

Version = git commit hash. That's it.

```cpp
// version.h (generated at build)
#pragma once

namespace dw {
    constexpr const char* VERSION = "a3f7c";      // Short git hash
    constexpr const char* BUILD_DATE = "2026-02-03";
}
```

No semantic versioning ceremony. No major.minor.patch. The git hash identifies the exact code state.

## Module Responsibilities

### core/loaders/
- `STLLoader` - Binary and ASCII STL parsing
- `OBJLoader` - Wavefront OBJ with MTL support
- `ThreeMFLoader` - 3MF archive parsing

### core/mesh/
- `Mesh` - Vertex/index data, bounding box
- `MeshHash` - Content fingerprinting (xxHash)

### core/gcode/
- `GCodeParser` - Parse G-code commands
- `GCodeAnalyzer` - Calculate timing, tool changes
- `GCodePath` - Path data for preview rendering

### core/optimizer/
- `CutOptimizer` - 2D bin packing algorithms
- `CutPlan` - Resulting layout
- `SheetDefinition` - Material sheet sizes

### core/database/
- `Database` - SQLite wrapper
- `ModelRepository` - Model CRUD
- `ProjectRepository` - Project CRUD

### ui/panels/
- `LibraryPanel` - Browse model library
- `PropertiesPanel` - Edit focused object
- `GCodePanel` - G-code analysis display
- `OptimizerPanel` - Cut planning interface
- `ViewportPanel` - 3D/2D preview

### render/
- `Renderer` - OpenGL state management
- `MeshRenderer` - Draw meshes
- `GCodeRenderer` - Draw toolpaths
- `GridRenderer` - Draw workspace grid

## Data Flow

```
File Drop/Import
      │
      ▼
Detect Format (by extension)
      │
      ▼
Load File (read into memory)
      │
      ▼
Parse Mesh (loader based on format)
      │
      ▼
Compute Hash (xxHash)
      │
      ▼
Check Duplicates (hash lookup)
      │
      ▼
Upload to GPU
      │
      ▼
Generate Thumbnail (offscreen render)
      │
      ▼
Store in Database
      │
      ▼
Update Library Panel
```

## Background Loading

For UI responsiveness, load on a background thread:
```cpp
// Start async load
auto future = std::async(std::launch::async, [path]() {
    return loadMesh(path);
});

// In UI loop, check if done
if (future.wait_for(0ms) == std::future_status::ready) {
    auto mesh = future.get();
    mesh->uploadToGPU();  // Must be on main thread
    workspace.setFocusedModel(std::move(mesh));
}
```

## Performance Notes

- **Parsing** is the bottleneck, not RAM
- Binary STL is fastest (~1GB/s on modern CPU)
- OBJ is slower due to text parsing
- 3MF requires ZIP decompression
- GPU upload is fast (PCIe bandwidth)
