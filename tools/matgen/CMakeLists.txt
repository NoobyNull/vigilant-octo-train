# Digital Workshop - Material Generator Tool
# CLI tool to generate .dwmat archives for built-in materials via Gemini API

# Source files needed from main project (selective recompilation)
set(MATGEN_DEPS
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/log.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/file_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/gemini_http.cpp
    ${CMAKE_SOURCE_DIR}/src/core/paths/app_paths.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/input_binding.cpp
    ${CMAKE_SOURCE_DIR}/src/core/threading/thread_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/core/gcode/machine_profile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/default_materials.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/gemini_material_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/material_archive.cpp
    ${CMAKE_SOURCE_DIR}/src/core/archive/archive.cpp
)

set(MATGEN_SOURCES
    matgen_main.cpp
)

add_executable(dw_matgen ${MATGEN_SOURCES} ${MATGEN_DEPS})

dw_configure_target(dw_matgen)

target_link_libraries(dw_matgen PRIVATE
    imgui
    miniz_static
    nlohmann_json::nlohmann_json
    CURL::libcurl
    ZLIB::ZLIB
    glm::glm
)

target_include_directories(dw_matgen PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
    ${stb_SOURCE_DIR}
)

set_target_properties(dw_matgen PROPERTIES
    OUTPUT_NAME "dw_matgen"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
