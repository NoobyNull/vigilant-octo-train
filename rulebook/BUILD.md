# Build & Deployment

## Dependencies

| Dependency | Version | Purpose | License |
|------------|---------|---------|---------|
| SDL2 | 2.28+ | Windowing, input | zlib |
| Dear ImGui | 1.90+ (docking) | UI framework | MIT |
| OpenGL | 3.3+ | Rendering | - |
| SQLite3 | 3.40+ | Database | Public Domain |
| xxHash | 0.8+ | Content hashing | BSD-2 |
| stb_image | latest | Image loading | Public Domain |

All dependencies are permissively licensed - no GPL concerns.

## Compiler Requirements

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| Windows | MSVC | 2022 (v143) |
| Windows | Clang | 15+ |
| Linux | GCC | 11+ |
| Linux | Clang | 15+ |
| macOS | Clang | 15+ (Xcode 14+) |

## Build Commands

### Configure
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Build
```bash
cmake --build build --config Release --parallel
```

### Test
```bash
ctest --test-dir build --output-on-failure
```

## CMake Structure

```
CMakeLists.txt              # Root configuration
├── cmake/
│   ├── Dependencies.cmake  # Find/fetch dependencies
│   ├── CompilerFlags.cmake # Warning flags per platform
│   └── Packaging.cmake     # CPack configuration
├── src/
│   └── CMakeLists.txt      # Main executable
├── tests/
│   └── CMakeLists.txt      # Test executable
└── third_party/
    └── CMakeLists.txt      # Vendored dependencies
```

## Dependency Management

Prefer FetchContent for dependencies:
```cmake
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)
```

## Platform-Specific Notes

### Windows
- Use vcpkg or FetchContent for SDL2
- Bundle SDL2.dll with executable
- Sign executable for distribution (optional)

### Linux
- SDL2 from system package manager or FetchContent
- AppImage for distribution (self-contained)
- Flatpak as alternative

### macOS
- SDL2 via Homebrew or FetchContent
- Create .app bundle for distribution
- Notarization required for Gatekeeper

## Deployment Sizes (Target)

| Platform | Size |
|----------|------|
| Windows | 8-15 MB |
| Linux AppImage | 15-25 MB |
| macOS .app | 10-20 MB |

## Build Artifacts

```
build/
├── digital_workshop         # Main executable
├── digital_workshop.exe     # Windows
├── DigitalWorkshop.app/     # macOS bundle
└── tests/
    └── dw_tests             # Test executable
```

## CI/CD

GitHub Actions workflow:
```yaml
jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: ctest --test-dir build --output-on-failure
```
