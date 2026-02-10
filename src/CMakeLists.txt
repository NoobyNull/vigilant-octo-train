# Digital Workshop - Source CMakeLists.txt
# Main executable target

# Collect source files
set(DW_SOURCES
    main.cpp
    app/application.cpp
    app/window.cpp
    app/workspace.cpp

    # Managers (extracted from Application)
    managers/ui_manager.cpp
    managers/file_io_manager.cpp
    managers/config_manager.cpp

    # Core types
    core/types.cpp

    # Core utilities
    core/utils/log.cpp
    core/utils/file_utils.cpp
    core/utils/string_utils.cpp

    # Threading
    core/threading/main_thread_queue.cpp

    # Core paths
    core/paths/app_paths.cpp

    # Core config
    core/config/config.cpp
    core/config/config_watcher.cpp
    core/config/input_binding.cpp

    # Database
    core/database/database.cpp
    core/database/connection_pool.cpp
    core/database/schema.cpp
    core/database/model_repository.cpp
    core/database/project_repository.cpp
    core/database/gcode_repository.cpp

    # Mesh
    core/mesh/mesh.cpp
    core/mesh/hash.cpp

    # Loaders
    core/loaders/loader_factory.cpp
    core/loaders/stl_loader.cpp
    core/loaders/obj_loader.cpp
    core/loaders/threemf_loader.cpp
    core/loaders/gcode_loader.cpp

    # Project
    core/project/project.cpp

    # Import
    core/import/import_queue.cpp
    core/import/file_handler.cpp

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
    render/thumbnail_generator.cpp

    # UI
    # ui/ui_manager.cpp  -- dead code, replaced by managers/ui_manager.cpp (Phase 1.6 cleanup)
    ui/theme.cpp

    # UI Panels
    ui/panels/viewport_panel.cpp
    ui/panels/library_panel.cpp
    ui/panels/properties_panel.cpp
    ui/panels/project_panel.cpp
    ui/panels/gcode_panel.cpp
    ui/panels/cut_optimizer_panel.cpp
    ui/panels/start_page.cpp

    # UI Widgets
    ui/widgets/binding_recorder.cpp

    # UI Dialogs
    ui/dialogs/message_dialog.cpp
    ui/dialogs/file_dialog.cpp
    ui/dialogs/lighting_dialog.cpp
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
    ZLIB::ZLIB
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

# Install target
install(TARGETS digital_workshop RUNTIME DESTINATION bin)
