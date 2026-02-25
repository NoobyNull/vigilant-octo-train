# Digital Workshop - Source CMakeLists.txt
# Main executable target

# Collect source files
set(DW_SOURCES
    main.cpp
    app/application.cpp
    app/application_wiring.cpp
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
    core/utils/gemini_http.cpp

    # Threading
    core/threading/main_thread_queue.cpp
    core/threading/thread_pool.cpp

    # Core paths
    core/paths/app_paths.cpp
    core/paths/path_resolver.cpp

    # Core config
    core/config/config.cpp
    core/config/config_watcher.cpp
    core/config/input_binding.cpp

    # Database
    core/database/database.cpp
    core/database/connection_pool.cpp
    core/database/schema.cpp
    core/database/statement_helper.cpp
    core/database/model_repository.cpp
    core/database/material_repository.cpp
    core/database/project_repository.cpp
    core/database/gcode_repository.cpp
    core/database/cost_repository.cpp
    core/database/cut_plan_repository.cpp
    core/database/tool_database.cpp

    # Graph
    core/graph/graph_manager.cpp

    # Mesh
    core/mesh/mesh.cpp
    core/mesh/hash.cpp

    # Loaders
    core/loaders/loader_factory.cpp
    core/loaders/stl_loader.cpp
    core/loaders/obj_loader.cpp
    core/loaders/threemf_loader.cpp
    core/loaders/gcode_loader.cpp
    core/loaders/texture_loader.cpp

    # Project
    core/project/project.cpp

    # Storage
    core/storage/storage_manager.cpp

    # Import
    core/import/import_queue.cpp
    core/import/import_log.cpp
    core/import/background_tagger.cpp
    core/import/file_handler.cpp
    core/import/filesystem_detector.cpp

    # Library
    core/library/library_manager.cpp

    # Export
    core/export/model_exporter.cpp
    core/export/project_export_manager.cpp
    core/export/project_import.cpp

    # G-code
    core/gcode/gcode_parser.cpp
    core/gcode/gcode_analyzer.cpp
    core/gcode/gcode_modal_scanner.cpp
    core/gcode/machine_profile.cpp

    # CNC Controller (multi-firmware support)
    core/cnc/serial_port.cpp
    core/cnc/cnc_controller.cpp
    core/cnc/preflight_check.cpp
    core/cnc/tool_calculator.cpp
    core/cnc/grbl_settings.cpp

    # Optimizer
    core/optimizer/cut_optimizer.cpp
    core/optimizer/cut_list_file.cpp
    core/optimizer/bin_packer.cpp
    core/optimizer/guillotine.cpp

    # Archive
    core/archive/archive.cpp

    # Materials
    core/materials/default_materials.cpp
    core/materials/gemini_material_service.cpp
    core/materials/gemini_descriptor_service.cpp
    core/materials/material_archive.cpp
    core/materials/material_manager.cpp

    # Render
    render/gl_utils.cpp
    render/shader.cpp
    render/camera.cpp
    render/framebuffer.cpp
    render/renderer.cpp
    render/thumbnail_generator.cpp
    render/texture.cpp

    # UI
    # ui/ui_manager.cpp  -- dead code, replaced by managers/ui_manager.cpp (Phase 1.6 cleanup)
    ui/theme.cpp
    ui/context_menu_manager.cpp

    # UI Panels
    ui/panels/viewport_panel.cpp
    ui/panels/library_panel.cpp
    ui/panels/library_panel_items.cpp
    ui/panels/materials_panel.cpp
    ui/panels/materials_panel_dialogs.cpp
    ui/panels/properties_panel.cpp
    ui/panels/project_panel.cpp
    ui/panels/gcode_panel.cpp
    ui/panels/cost_panel.cpp
    ui/panels/cut_optimizer_panel.cpp
    ui/panels/start_page.cpp
    ui/panels/tool_browser_panel.cpp
    ui/panels/cnc_status_panel.cpp
    ui/panels/cnc_jog_panel.cpp
    ui/panels/cnc_console_panel.cpp
    ui/panels/cnc_wcs_panel.cpp
    ui/panels/cnc_tool_panel.cpp
    ui/panels/cnc_job_panel.cpp
    ui/panels/cnc_safety_panel.cpp

    # UI Widgets
    ui/widgets/binding_recorder.cpp
    ui/widgets/status_bar.cpp
    ui/widgets/toast.cpp

    # UI Dialogs
    ui/dialogs/message_dialog.cpp
    ui/dialogs/file_dialog.cpp
    ui/dialogs/lighting_dialog.cpp
    ui/dialogs/machine_profile_dialog.cpp
    ui/dialogs/import_summary_dialog.cpp
    ui/dialogs/import_options_dialog.cpp
    ui/dialogs/progress_dialog.cpp
    ui/dialogs/tag_image_dialog.cpp
    ui/dialogs/maintenance_dialog.cpp
    ui/dialogs/tagger_shutdown_dialog.cpp
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
    miniz_static
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# Include directories
target_include_directories(digital_workshop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
    ${stb_SOURCE_DIR}
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

# --- Bundle GraphQLite extension (if available) ---
if(DW_GRAPHQLITE_AVAILABLE)
    add_custom_command(TARGET digital_workshop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DW_GRAPHQLITE_LIB_PATH}"
            "$<TARGET_FILE_DIR:digital_workshop>/${GRAPHQLITE_LIB}"
        COMMENT "Copying GraphQLite extension to build directory"
    )
endif()

# --- Bundle resources (materials) ---
# Copy .dwmat files from resources/materials/ to the build output directory.
# Uses a stamp file to avoid issues with special characters in filenames.
file(GLOB DW_BUNDLED_MATERIALS "${CMAKE_SOURCE_DIR}/resources/materials/*.dwmat")
if(DW_BUNDLED_MATERIALS)
    set(DW_MATERIALS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/resources/materials")
    set(DW_MATERIALS_STAMP "${CMAKE_BINARY_DIR}/materials_copied.stamp")

    add_custom_command(
        OUTPUT "${DW_MATERIALS_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DW_MATERIALS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/materials" "${DW_MATERIALS_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${DW_MATERIALS_STAMP}"
        DEPENDS ${DW_BUNDLED_MATERIALS}
        COMMENT "Copying bundled materials to build dir"
    )

    add_custom_target(dw_bundle_materials ALL DEPENDS "${DW_MATERIALS_STAMP}")
    add_dependencies(digital_workshop dw_bundle_materials)
endif()

# Install target
install(TARGETS digital_workshop RUNTIME DESTINATION bin)

# Install bundled materials if directory exists
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/materials/")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/materials/"
        DESTINATION share/digitalworkshop/resources/materials
        FILES_MATCHING PATTERN "*.dwmat"
    )
endif()
