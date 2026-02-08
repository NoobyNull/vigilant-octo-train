# Technology Stack

**Analysis Date:** 2026-02-08

## Languages

**Primary:**
- C++17 - Desktop application core engine, graphics rendering, and data processing
  - Location: `src/` (main application logic)
  - Enforced via CMakeLists.txt: `set(CMAKE_CXX_STANDARD 17)`

**Secondary:**
- C - SQLite3 integration and compilation
  - Location: Dependencies fetched and compiled from source
  - Used for: Database layer bindings

## Runtime

**Environment:**
- Native compiled binary (no virtual machine)
- Cross-platform: Linux, Windows, macOS support
- Compiled to single executable: `digital_workshop` or `dw_settings`

**Package Manager:**
- CMake - Version 3.20+ required
- Lockfile: No lockfile - dependencies pinned by git tag in CMakeLists.txt

## Frameworks

**Core GUI:**
- Dear ImGui (docking branch) - Immediate-mode UI toolkit
  - Repository: https://github.com/ocornut/imgui
  - Branch: docking (not main release)
  - Purpose: Complete UI framework including panels, windows, menus
  - Compiled as static library: `imgui` target in `cmake/Dependencies.cmake`

**Graphics & Windowing:**
- SDL2 (2.30.0) - Windowing, input events, cross-platform abstraction
  - Repository: https://github.com/libsdl-org/SDL.git
  - Tag: release-2.30.0
  - Purpose: Window management, keyboard/mouse input, event loop
  - Link mode: Prefers system SDL2, falls back to static compilation
  - Configuration: `set(SDL_STATIC ON)`, `set(SDL_SHARED OFF)`

**OpenGL:**
- OpenGL 3.3 Core - Graphics API
  - GLAD (v2.0.6) - OpenGL loader for core 3.3 profile
    - Pre-generated loader for efficiency
    - Repository: https://github.com/Dav1dde/glad

**Math Library:**
- GLM (1.0.1) - Header-only vector/matrix math
  - Repository: https://github.com/g-truc/glm
  - Used for: 3D transformations, matrix operations
  - Type: Header-only, no compilation needed

**Testing:**
- GoogleTest (v1.14.0) - C++ unit testing framework
  - Repository: https://github.com/google/googletest
  - Configured with: `set(BUILD_GMOCK OFF)`, `set(INSTALL_GTEST OFF)`
  - Test executable: `dw_tests`

**Build/Dev:**
- CMake (3.20+) - Cross-platform build system
  - Export compile_commands.json for clang tools
  - FetchContent module for automatic dependency downloading
  - CPack for packaging (NSIS on Windows, TGZ on Linux)

## Key Dependencies

**Critical:**
- SQLite3 (3.45.0) - Embedded relational database
  - Repository: https://github.com/azadkuh/sqlite-amalgamation
  - Static compilation: Entire database engine compiled into executable
  - Purpose: Project metadata, model library, cost estimates
  - Location: `src/core/database/`
  - Features: Transaction support, RAII wrapper, prepared statements

- zlib (1.3.1) - Compression library
  - Repository: https://github.com/madler/zlib
  - Purpose: Deflate compression for ZIP and 3MF file format support
  - Used by: `src/core/archive/` and 3MF loader
  - Build: Static compilation with OpenGL deps

**Infrastructure:**
- OpenGL (native system library) - Graphics rendering
  - CMake: `find_package(OpenGL REQUIRED)`
  - Essential for: Viewport rendering, thumbnail generation

## Configuration

**Build Configuration:**
- CMAKE_BUILD_TYPE: Release (default), Debug supported
- CMAKE_CXX_STANDARD: 17 (enforced)
- CMAKE_EXPORT_COMPILE_COMMANDS: ON (enables clang-tools integration)
- DW_BUILD_TESTS: ON/OFF (enabled by default in CMakeLists.txt)

**Compiler Configuration:**
- File: `cmake/CompilerFlags.cmake`
- Clang-Format: `.clang-format` (LLVM style, 4-space indents, 100-column limit)
- Clang-Tidy: `.clang-tidy` (linting rules configuration)

**Build Output:**
- Main executable location: `${CMAKE_BINARY_DIR}/digital_workshop`
- Settings app location: `${CMAKE_BINARY_DIR}/dw_settings`
- Test executable location: `${CMAKE_BINARY_DIR}/tests/dw_tests`
- Generated code: `${CMAKE_BINARY_DIR}/generated/` (includes version.h)

## Platform Requirements

**Development:**
- CMake 3.20 or later
- C++17 compatible compiler (tested with GCC/Clang)
- SDL2 development headers (or automatic build from source)
- OpenGL 3.3 capable GPU
- System libraries: libgl-dev (Linux), zlib1g-dev (Linux)
- Linux CI: Ubuntu 24.04 (with libsdl2-dev, libgl-dev, zlib1g-dev, makeself)

**Production:**
- Deployment targets:
  - **Linux**: x86_64 Linux binary packaged as self-extracting .run installer (makeself)
  - **Windows**: x86_64 Windows EXE with NSIS installer
  - **macOS**: Not currently supported in CI (would need similar setup)

**Runtime Dependencies:**
- SDL2 libraries (if system SDL2 used, not statically linked)
- OpenGL drivers for target GPU
- Standard C++ runtime

## Dependency Management Strategy

**FetchContent Model:**
All dependencies use CMake FetchContent with system fallback pattern:
1. Try to find system-installed package via `find_package()`
2. If not found, download from GitHub using FetchContent_Declare()
3. Pin specific git tags for reproducibility (e.g., `GIT_TAG release-2.30.0`)
4. Use `GIT_SHALLOW TRUE` for faster clones

**Example from Dependencies.cmake:**
```cmake
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    FetchContent_Declare(
        sqlite3
        GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation.git
        GIT_TAG 3.45.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sqlite3)
endif()
```

Benefits:
- No external package manager required
- System libraries used when available
- Reproducible builds with pinned versions
- Clean build from clean checkout

## Version Management

**Application Version:**
- Source: `CMakeLists.txt` project version (0.1.0)
- Runtime identifier: Git commit hash + build date
- Generated header: `${CMAKE_BINARY_DIR}/generated/version.h`
- Pattern: 7-char short hash, "-dirty" suffix if uncommitted changes
- Query method: `cmake/GitVersion.cmake` extracts via `git rev-parse --short=7 HEAD`

---

*Stack analysis: 2026-02-08*
