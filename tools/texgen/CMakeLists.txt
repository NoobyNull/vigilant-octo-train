# Digital Workshop - Material Texture Generator Tool
# CLI tool to batch-generate material textures using Gemini API

# Source files needed from main project
set(TEXGEN_DEPS
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/log.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/file_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/paths/app_paths.cpp
    ${CMAKE_SOURCE_DIR}/src/core/archive/archive.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/material_archive.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/default_materials.cpp
    ${CMAKE_SOURCE_DIR}/src/core/materials/gemini_material_service.cpp
)

set(TEXGEN_SOURCES
    texgen_main.cpp
)

add_executable(dw_texgen ${TEXGEN_SOURCES} ${TEXGEN_DEPS})

dw_configure_target(dw_texgen)

target_link_libraries(dw_texgen PRIVATE
    miniz_static
    nlohmann_json::nlohmann_json
    CURL::libcurl
    ZLIB::ZLIB
    glm::glm
)

target_include_directories(dw_texgen PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

set_target_properties(dw_texgen PROPERTIES
    OUTPUT_NAME "dw_texgen"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Install target
install(TARGETS dw_texgen RUNTIME DESTINATION bin)
