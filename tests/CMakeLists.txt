# Digital Workshop - Tests CMakeLists.txt
# Test suite using GoogleTest

# Collect test sources
set(DW_TEST_SOURCES
    test_main.cpp
    # Tier 0 — existing tests
    test_stl_loader.cpp
    test_obj_loader.cpp
    test_hash.cpp
    test_gcode_parser.cpp
    test_optimizer.cpp
    # Tier 0 — added in first pass
    test_string_utils.cpp
    test_file_utils.cpp
    test_mesh.cpp
    test_database.cpp
    test_model_repository.cpp
    test_project_repository.cpp
    test_loader_factory.cpp
    test_project.cpp
    test_exporter.cpp
    # Tier 1 — EventBus
    test_event_bus.cpp
    # Tier 1 — ConnectionPool
    test_connection_pool.cpp
    # Tier 1 — MainThreadQueue
    test_main_thread_queue.cpp
    # Tier 1 — core logic
    test_types.cpp
    test_gcode_analyzer.cpp
    test_schema.cpp
    test_camera.cpp
    test_archive.cpp
    test_library_manager.cpp
    test_threemf_loader.cpp
    test_import_pipeline.cpp
    test_config_watcher.cpp
    test_app_paths.cpp
    test_cost_repository.cpp
    # Tier 2 — lint/format compliance (no GL/SDL required)
    test_lint_compliance.cpp
    # Stubs for GL-dependent classes
    stub_thumbnail_generator.cpp
)

# Source files needed by the tests (compiled from src/)
set(DW_TEST_DEPS
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/events/event_bus.cpp
    ${CMAKE_SOURCE_DIR}/src/core/threading/main_thread_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/threading/thread_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/log.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utils/file_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/core/mesh/mesh.cpp
    ${CMAKE_SOURCE_DIR}/src/core/mesh/hash.cpp
    ${CMAKE_SOURCE_DIR}/src/core/loaders/stl_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/core/loaders/obj_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/core/loaders/threemf_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/core/loaders/gcode_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/core/loaders/loader_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/core/gcode/gcode_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/core/gcode/gcode_analyzer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/optimizer/cut_optimizer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/optimizer/bin_packer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/optimizer/guillotine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/database.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/connection_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/schema.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/model_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/project_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/cost_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/gcode_repository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/project/project.cpp
    ${CMAKE_SOURCE_DIR}/src/core/export/model_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/archive/archive.cpp
    ${CMAKE_SOURCE_DIR}/src/core/library/library_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/import/import_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/import/file_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/config_watcher.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config/input_binding.cpp
    ${CMAKE_SOURCE_DIR}/src/core/paths/app_paths.cpp
    ${CMAKE_SOURCE_DIR}/src/render/camera.cpp
)

# Create test executable
add_executable(dw_tests ${DW_TEST_SOURCES} ${DW_TEST_DEPS})

# Apply compiler flags
dw_configure_target(dw_tests)

# Link dependencies
target_link_libraries(dw_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    imgui
    glm::glm
    SQLite::SQLite3
    ZLIB::ZLIB
)

# Include source directories for testing internal modules
target_include_directories(dw_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

# Pass source dir to lint compliance tests
target_compile_definitions(dw_tests PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(dw_tests)

# Copy test data
# file(COPY data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
