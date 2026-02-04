# Ralph Agent Configuration

## Build Instructions

```bash
# Configure and build the project
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

## Test Instructions

```bash
# Run tests
ctest --test-dir build --output-on-failure
```

## Run Instructions

```bash
# Start the application
./build/digital_workshop
```

## Lint Instructions

```bash
# Check formatting
clang-format --dry-run -Werror src/**/*.cpp src/**/*.h

# Run static analysis
clang-tidy src/main.cpp -- -I build/_deps
```

## Screenshot Verification

```bash
# Capture active window for visual verification
spectacle -a -b -n -o /tmp/dw_screenshot.png
# View, then delete when done
rm /tmp/dw_screenshot.png
```

## Prerequisites

- CMake 3.20+
- C++17 compiler (GCC 11+, Clang 15+, MSVC 2022)
- SDL2, OpenGL 3.3
- Dependencies fetched automatically via CMake FetchContent

## Notes

- Update this file when build process changes
- All dependencies are fetched via CMake - no manual installation
- Build artifacts go in `build/` directory
- Database and config files can be deleted freely (no migration)
