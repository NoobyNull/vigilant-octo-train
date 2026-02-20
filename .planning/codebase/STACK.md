# Technology Stack

**Analysis Date:** 2026-02-19

## Languages

**Primary:**
- C++ 17 - Core application, all business logic, UI, rendering, and database management
- C - SQLite3 dependency (compiled from source)

**Build System:**
- CMake 3.20+ - Cross-platform build configuration and dependency management

## Runtime

**Environment:**
- Desktop application (cross-platform: Linux, Windows, macOS via SDL2/OpenGL)
- No external runtime required; all dependencies bundled or fetched at build time

**Platform Support:**
- Windows (NSIS installer generation, desktop/start menu shortcuts)
- Linux (TGZ archive packaging)
- macOS (via SDL2/OpenGL support)

## Frameworks

**Core UI & Windowing:**
- SDL2 2.30.0 - Window management, input handling, event loop
  - Strategy: System library with fallback to FetchContent from GitHub
  - Build mode: Static library (SDL_STATIC=ON)
  - Location: `cmake/Dependencies.cmake` (lines 6-25)

**Dear ImGui (docking branch)** - Immediate mode GUI framework
  - Branch: `docking` (for multi-viewport/window docking support)
  - Integration: Custom backends for SDL2 (`imgui_impl_sdl2.cpp`) and OpenGL3 (`imgui_impl_opengl3.cpp`)
  - Built as static library target in `cmake/Dependencies.cmake` (lines 28-50)

**Graphics/Rendering:**
- OpenGL 3.3 Core Profile - Rendering engine for 3D models and toolpaths
- GLAD 2.0.6 - OpenGL loader (pre-generated for GL 3.3 Core)
  - Location: `cmake/Dependencies.cmake` (lines 55-64)
  - Provides `glad_gl33_core` target

## Key Dependencies

**Critical (Core Functionality):**
- **SQLite3 3.45.0** - Embedded SQL database for model library, projects, materials, G-code metadata
  - Strategy: System fallback, else compile from sqlite-amalgamation source
  - Location: `cmake/Dependencies.cmake` (lines 75-93)
  - Usage: `src/core/database/` module with connection pooling and transaction support
  - Why it matters: Entire library, project, and import system depends on this

- **GLM 1.0.1** - Header-only math library for 3D vector/matrix operations
  - Location: `cmake/Dependencies.cmake` (lines 66-73)
  - Used throughout: camera, mesh transformations, rendering calculations

**Compression & Archive:**
- **zlib 1.3.1** - Compression library for ZIP/3MF file handling
  - Strategy: System library with GitHub FetchContent fallback
  - Location: `cmake/Dependencies.cmake` (lines 95-117)
  - Usage: `src/core/archive/archive.cpp` for 3MF/ZIP decompression

- **miniz 3.0.2** - Single-file ZIP compression library (alternative/supplement to zlib)
  - Location: `cmake/Dependencies.cmake` (lines 119-136)
  - Usage: `src/core/archive/` for ZIP/3MF material archive handling

**Image & Media:**
- **stb (header-only, master branch)** - Single-file libraries for image loading
  - Location: `cmake/Dependencies.cmake` (lines 138-145)
  - Provides: Image decoders for thumbnails
  - Usage: `src/render/thumbnail_generator.cpp`

**Serialization:**
- **nlohmann/json 3.11.3** - JSON library for configuration, serialization, and metadata
  - Location: `cmake/Dependencies.cmake` (lines 147-156)
  - Usage: Config files, cost estimates (stored as JSON in database), tag storage

**Testing:**
- **GoogleTest 1.14.0** - Unit testing framework (dev dependency only)
  - Location: `cmake/Dependencies.cmake` (lines 158-170)
  - Build: Conditional on `DW_BUILD_TESTS=ON` (default ON)
  - Usage: `tests/` directory with comprehensive tier-based test coverage

## Configuration

**Build Configuration:**
- CMake variables and options:
  - `DW_BUILD_TESTS` (default: ON) - Enable/disable test suite compilation
  - `DW_ENABLE_ASAN` (optional) - Enable AddressSanitizer for debug builds
  - `CMAKE_BUILD_TYPE` (Debug/Release) - Affects warning-as-error behavior

**Compiler Configuration (`cmake/CompilerFlags.cmake`):**
- **GCC/Clang common flags:**
  - `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` and 14+ strict warnings
  - `-Werror` in Release builds (treat warnings as errors)
  - Debug: `-g3 -O0 -fno-omit-frame-pointer`
  - Release: `-O3 -DNDEBUG`

- **MSVC flags:**
  - `/W4 /permissive- /utf-8` (strict conformance)
  - `/WX` (warnings as errors) in Release builds
  - `/Zi /Od /RTC1` in Debug, `/O2 /DNDEBUG` in Release

**Platform-Specific:**
- Windows: NSIS installer generation with file associations for `.dwp` (Digital Workshop Project) files
- Linux: TGZ archive packaging
- All platforms: Export `compile_commands.json` for IDE/tooling support

## Version Management

**Version Source:**
- Git commit hash (short 7 chars) injected at build time via `cmake/GitVersion.cmake`
- Build date (YYYY-MM-DD format) included
- Dirty working directory detected and appended (`-dirty` suffix)
- Fallback: `"nogit"` if Git not available

**Version Header:**
- Generated at build time: `${CMAKE_BINARY_DIR}/generated/version.h`
- Template source: `src/version.h.in`
- Included by: Application startup (`src/app/application.cpp` includes `version.h`)

## Build & Installation

**Build Process:**
- CMake generates platform-specific build files (Unix Makefiles, MSVC, Xcode, etc.)
- All C++ compiled to single executable: `digital_workshop` (output to `${CMAKE_BINARY_DIR}/`)
- Tests compiled to: `dw_tests` (when `DW_BUILD_TESTS=ON`)

**Installation Targets:**
- Main executable: Installed to `bin/` directory
- Platform-specific packaging:
  - Windows: NSIS `.exe` installer with shortcuts and file associations
  - Linux: TGZ archive

**Dependency Strategy:**
- FetchContent-based (prefer system libraries with fallback)
- All dependencies built as static libraries (except SDL2 which can use system dynamic lib)
- No external package manager required (npm, pip, cargo, etc.)

## Packaging

**CPack Integration:**
- Enabled in root `CMakeLists.txt` (lines 43-84)
- Package metadata:
  - Name: `DigitalWorkshop`
  - Version: From `PROJECT_VERSION` (defined in root CMakeLists.txt)
  - License file: `LICENSE`
  - Description: "3D model management, G-code analysis, and 2D cut optimization"

## Development Environment

**Minimum Requirements:**
- CMake 3.20+
- C++17 compiler (GCC, Clang, MSVC)
- Git (for version information, optional but recommended)

**Optional:**
- GoogleTest development (auto-fetched if tests enabled)
- SDL2 development libraries (system or auto-fetched)

## Platform-Specific Notes

**OpenGL Requirement:**
- Requires OpenGL 3.3 Core profile support
- Hardware: Any GPU supporting GL 3.3+ (2012+, most modern systems)

**SDL2 Strategy:**
- Primary: Uses system SDL2 if available
- Fallback: Clones and compiles from GitHub
- Windows behavior: Links `SDL2::SDL2main` for proper WinMain entry point

**File Format Support (via dedicated loaders):**
- STL (binary & ASCII) - `src/core/loaders/stl_loader.cpp`
- OBJ - `src/core/loaders/obj_loader.cpp`
- 3MF (ZIP-based with XML) - `src/core/loaders/threemf_loader.cpp`
- G-code - `src/core/loaders/gcode_loader.cpp`

---

*Stack analysis: 2026-02-19*
