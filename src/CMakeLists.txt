# Digital Workshop - Source CMakeLists.txt
# Main executable target

# Collect source files
set(DW_SOURCES
    main.cpp
    app/application.cpp
    app/window.cpp
    app/workspace.cpp

    # Core types
    core/types.cpp

    # Core utilities
    core/utils/log.cpp
    core/utils/file_utils.cpp
    core/utils/string_utils.cpp

    # Core paths
    core/paths/app_paths.cpp

    # Core config
    core/config/config.cpp

    # Database
    core/database/database.cpp
    core/database/schema.cpp
    core/database/model_repository.cpp
    core/database/project_repository.cpp

    # Mesh
    core/mesh/mesh.cpp
    core/mesh/hash.cpp

    # Loaders
    core/loaders/loader_factory.cpp
    core/loaders/stl_loader.cpp
    core/loaders/obj_loader.cpp

    # Project
    core/project/project.cpp

    # Library
    core/library/library_manager.cpp

    # Export
    core/export/model_exporter.cpp

    # G-code
    core/gcode/gcode_parser.cpp
    core/gcode/gcode_analyzer.cpp

    # Optimizer
    core/optimizer/cut_optimizer.cpp
    core/optimizer/bin_packer.cpp
    core/optimizer/guillotine.cpp

    # Archive
    core/archive/archive.cpp

    # Render
    render/gl_utils.cpp
    render/shader.cpp
    render/camera.cpp
    render/framebuffer.cpp
    render/renderer.cpp

    # UI
    ui/ui_manager.cpp
    ui/theme.cpp

    # UI Panels
    ui/panels/viewport_panel.cpp
    ui/panels/library_panel.cpp
    ui/panels/properties_panel.cpp
    ui/panels/project_panel.cpp

    # UI Dialogs
    ui/dialogs/message_dialog.cpp
    ui/dialogs/file_dialog.cpp
    ui/dialogs/settings_dialog.cpp
)

# Create executable
add_executable(digital_workshop ${DW_SOURCES})

# Apply compiler flags
dw_configure_target(digital_workshop)

# Link dependencies
target_link_libraries(digital_workshop PRIVATE
    imgui
    glad_gl33_core
    OpenGL::GL
    glm::glm
    SQLite::SQLite3
)

# Include directories
target_include_directories(digital_workshop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(digital_workshop PRIVATE
        SDL2::SDL2main
    )
endif()

# Set output name
set_target_properties(digital_workshop PROPERTIES
    OUTPUT_NAME "digital_workshop"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
