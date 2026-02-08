# Digital Workshop - Settings Application
# Separate executable for editing application settings

# Source files needed from main project (selective recompilation)
set(SETTINGS_DEPS
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/log.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/file_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/paths/app_paths.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/input_binding.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/binding_recorder.cpp
)

set(SETTINGS_SOURCES
    settings_main.cpp
    settings_app.cpp
)

add_executable(dw_settings ${SETTINGS_SOURCES} ${SETTINGS_DEPS})

dw_configure_target(dw_settings)

target_link_libraries(dw_settings PRIVATE
    imgui
    glad_gl33_core
    OpenGL::GL
    glm::glm
)

target_include_directories(dw_settings PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

set_target_properties(dw_settings PROPERTIES
    OUTPUT_NAME "dw_settings"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
