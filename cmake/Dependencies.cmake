# Digital Workshop - Dependencies
# All dependencies fetched via FetchContent (no manual installation)

include(FetchContent)

# SDL2 - Windowing, input, events
find_package(SDL2 QUIET)
if(SDL2_FOUND)
    message(STATUS "Found system SDL2")
    # System SDL2 uses dynamic library
    set(DW_SDL2_TARGET SDL2::SDL2)
else()
    message(STATUS "SDL2 not found, fetching from GitHub...")
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.0
        GIT_SHALLOW TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)
    set(DW_SDL2_TARGET SDL2::SDL2-static)
endif()

# Dear ImGui with docking branch
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# Create ImGui library target
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC ${DW_SDL2_TARGET})

# OpenGL
find_package(OpenGL REQUIRED)

# GLAD - OpenGL loader (pre-generated for OpenGL 3.3 Core)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.6
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl33_core STATIC API gl:core=3.3)

# GLM - Math library (header-only)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# SQLite3
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found, fetching from GitHub...")
    FetchContent_Declare(
        sqlite3
        GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation.git
        GIT_TAG 3.45.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sqlite3)

    # Create SQLite3 target
    add_library(sqlite3 STATIC
        ${sqlite3_SOURCE_DIR}/sqlite3.c
    )
    target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
    add_library(SQLite::SQLite3 ALIAS sqlite3)
endif()

# zlib - Compression (needed for deflate in ZIP/3MF files)
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    message(STATUS "ZLIB not found, fetching from GitHub...")
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
        GIT_SHALLOW TRUE
    )
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zlib)

    # Create ZLIB::ZLIB alias if not already defined
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    endif()
    # Expose include dirs for consumers
    target_include_directories(zlibstatic PUBLIC
        ${zlib_SOURCE_DIR}
        ${zlib_BINARY_DIR}
    )
endif()

# miniz - Compression library for ZIP/3MF files
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(miniz)
if(NOT miniz_POPULATED)
    FetchContent_Populate(miniz)
    # miniz 3.0.2 splits the library into multiple .c files:
    #   miniz.c         - zlib API (deflate/inflate)
    #   miniz_tdef.c    - deflate compressor
    #   miniz_tinfl.c   - inflate decompressor
    #   miniz_zip.c     - ZIP reader/writer
    # All four are required for ZIP operations (mz_zip_*).
    add_library(miniz_static STATIC
        ${miniz_SOURCE_DIR}/miniz.c
        ${miniz_SOURCE_DIR}/miniz_tdef.c
        ${miniz_SOURCE_DIR}/miniz_tinfl.c
        ${miniz_SOURCE_DIR}/miniz_zip.c
    )
    target_include_directories(miniz_static PUBLIC ${miniz_SOURCE_DIR})
    # Define MINIZ_EXPORT to avoid export header dependency
    target_compile_definitions(miniz_static PUBLIC MINIZ_EXPORT=)
endif()

# stb - Single-file header libraries (image loading, etc.)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# nlohmann/json - JSON library
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json)

# GoogleTest (for testing only)
if(DW_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Status message
message(STATUS "Dependencies configured:")
message(STATUS "  SDL2:     ${SDL2_VERSION}")
message(STATUS "  ImGui:    docking branch")
message(STATUS "  OpenGL:   ${OPENGL_gl_LIBRARY}")
message(STATUS "  GLM:      1.0.1")
message(STATUS "  SQLite3:  3.45.0")
message(STATUS "  zlib:     ${ZLIB_VERSION_STRING}${zlib_VERSION}")
message(STATUS "  miniz:    3.0.2")
message(STATUS "  stb:      master (header-only)")
message(STATUS "  nlohmann/json: 3.11.3")
