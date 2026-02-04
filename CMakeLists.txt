# Digital Workshop - Root CMakeLists.txt
# Cross-platform C++17 desktop application
# Stack: SDL2 + Dear ImGui (docking) + OpenGL 3.3 + SQLite3

cmake_minimum_required(VERSION 3.20)

# Project definition (C needed for SQLite3)
project(DigitalWorkshop
    VERSION 0.1.0
    DESCRIPTION "3D model management, G-code analysis, and 2D cut optimization"
    LANGUAGES CXX C
)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clang tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(DW_BUILD_TESTS "Build test suite" ON)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)
include(Dependencies)
include(GitVersion)

# Main executable sources will be in src/
add_subdirectory(src)

# Tests
if(DW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Summary
message(STATUS "")
message(STATUS "Digital Workshop Configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Git Hash:     ${DW_GIT_HASH}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests:        ${DW_BUILD_TESTS}")
message(STATUS "")
