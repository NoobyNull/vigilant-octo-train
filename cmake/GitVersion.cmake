# Digital Workshop - Git Version
# Extracts git hash for version identification
# Per ARCHITECTURE.md: Version = git commit hash

find_package(Git QUIET)

set(DW_GIT_HASH "unknown")
set(DW_BUILD_DATE "")

# Get current date
string(TIMESTAMP DW_BUILD_DATE "%Y-%m-%d")

# Get git hash if available
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE DW_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RESULT
    )

    if(NOT GIT_RESULT EQUAL 0)
        set(DW_GIT_HASH "nogit")
    endif()

    # Check if working directory is dirty
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY_RESULT
    )

    if(NOT GIT_DIRTY_RESULT EQUAL 0)
        set(DW_GIT_HASH "${DW_GIT_HASH}-dirty")
    endif()
else()
    set(DW_GIT_HASH "nogit")
endif()

# Configure version header
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)

# Make generated header available
include_directories(${CMAKE_BINARY_DIR}/generated)

message(STATUS "Version: ${DW_GIT_HASH} (${DW_BUILD_DATE})")
