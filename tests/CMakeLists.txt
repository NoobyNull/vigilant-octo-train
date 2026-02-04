# Digital Workshop - Tests CMakeLists.txt
# Test suite using GoogleTest

# Collect test sources
set(DW_TEST_SOURCES
    test_main.cpp
)

# Will be populated as tests are added:
# test_stl_loader.cpp
# test_obj_loader.cpp
# test_gcode_parser.cpp
# test_cut_optimizer.cpp
# test_database.cpp

# Create test executable
add_executable(dw_tests ${DW_TEST_SOURCES})

# Apply compiler flags
dw_configure_target(dw_tests)

# Link dependencies
target_link_libraries(dw_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    glm::glm
)

# Include source directories for testing internal modules
target_include_directories(dw_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(dw_tests)

# Copy test data
# file(COPY data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
