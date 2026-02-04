# Digital Workshop Rulebook

Standards, practices, and goals for the Digital Workshop project.

## System Requirements

| Spec | Minimum | Recommended |
|------|---------|-------------|
| CPU | Intel Quad-core | Intel 10th gen+ |
| RAM | 8 GB | 16 GB+ |
| GPU | OpenGL 3.3 capable | Dedicated GPU |
| OS | Windows 10, Linux, macOS 11+ | Latest |

## Technology Stack

| Component | Choice |
|-----------|--------|
| Language | C++17 |
| Windowing | SDL2 |
| UI | Dear ImGui (docking branch) |
| Rendering | OpenGL 3.3 |
| Database | SQLite3 |
| Build | CMake |

## Documents

| Document | Description |
|----------|-------------|
| [GOALS.md](GOALS.md) | Project vision, features, non-goals |
| [CODING_STANDARDS.md](CODING_STANDARDS.md) | Code style, naming, file organization |
| [ARCHITECTURE.md](ARCHITECTURE.md) | System design, modules, data flow, versioning |
| [QUALITY.md](QUALITY.md) | Testing, cross-platform checks |
| [BUILD.md](BUILD.md) | Build system, dependencies, deployment |
| [FORMATS.md](FORMATS.md) | Supported file formats (STL, OBJ, 3MF, G-code) |
| [UI_WORKFLOW.md](UI_WORKFLOW.md) | Visual design, screenshots, panel layout |

## Quick Reference

### Naming
- Classes: `PascalCase`
- Methods/variables: `camelCase`
- Private members: `m_` prefix
- Constants: `UPPER_CASE`
- Namespace: `dw`

### Key Principles
1. Cross-platform from day one (Windows, Linux, macOS)
2. Simple loading - modern systems have plenty of RAM
3. Object-centric UI - panels act on focused object
4. Core has no UI dependencies
5. Permissive licenses only (MIT, zlib, BSD, Public Domain)

### File Limits
- `.cpp`: max 800 lines
- `.h`: max 400 lines
- One class per header

### Supported Formats
- **3D Models:** STL, OBJ, 3MF
- **Toolpaths:** G-code

### Target Deployment Size
- 8-25 MB depending on platform
