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
option(DW_BUILD_TOOLS "Build developer tools (matgen, etc.)" OFF)
option(DW_ENABLE_GRAPHQLITE "Enable GraphQLite extension for graph queries" ON)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)
include(Dependencies)
include(GitVersion)

# Main executable sources will be in src/
add_subdirectory(src)

# Settings application (separate process)
add_subdirectory(settings)

# Developer tools (material generator, etc.)
if(DW_BUILD_TOOLS)
    add_subdirectory(tools/matgen)
    add_subdirectory(tools/texgen)
endif()

# Tests
if(DW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Packaging (CPack)
set(CPACK_PACKAGE_NAME "DigitalWorkshop")
set(CPACK_PACKAGE_VENDOR "Digital Workshop")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DigitalWorkshop")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    # NSIS installer for Windows
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Digital Workshop ${PROJECT_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "Digital Workshop")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)

    # Start menu and desktop shortcuts
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Digital Workshop.lnk' '$INSTDIR\\\\bin\\\\digital_workshop.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Digital Workshop.lnk'"
    )

    # File associations for .stl, .obj, .3mf, .dwp
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        WriteRegStr HKCR '.dwp' '' 'DigitalWorkshop.Project'
        WriteRegStr HKCR 'DigitalWorkshop.Project' '' 'Digital Workshop Project'
        WriteRegStr HKCR 'DigitalWorkshop.Project\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\bin\\\\digital_workshop.exe\\\" \\\"%1\\\"'
    ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        DeleteRegKey HKCR '.dwp'
        DeleteRegKey HKCR 'DigitalWorkshop.Project'
    ")
else()
    # TGZ archive for Linux
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

# Summary
message(STATUS "")
message(STATUS "Digital Workshop Configuration:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  Git Hash:     ${DW_GIT_HASH}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests:        ${DW_BUILD_TESTS}")
message(STATUS "  Tools:        ${DW_BUILD_TOOLS}")
message(STATUS "")
