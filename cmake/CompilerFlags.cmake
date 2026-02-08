# Digital Workshop - Compiler Flags
# Platform-specific compiler warnings and settings

# Common warning flags
function(dw_set_warning_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wcast-qual
            -Wformat=2
            -Wundef
            -Wshadow
            -Wcast-align
            -Wunused
            -Wnull-dereference
            -Wdouble-promotion
            -Wimplicit-fallthrough
        )

        # GCC-specific warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
            )
        endif()

        # Treat warnings as errors in Release builds
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_options(${target} PRIVATE -Werror)
        endif()

    elseif(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
        )

        # Treat warnings as errors in Release builds
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_options(${target} PRIVATE /WX)
        endif()
    endif()
endfunction()

# Debug-specific flags
function(dw_set_debug_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-g3 -O0 -fno-omit-frame-pointer>
        )

        # Address sanitizer for debug builds (optional)
        if(DW_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
            target_link_options(${target} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
        endif()

    elseif(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/Zi /Od /RTC1>
        )
    endif()
endfunction()

# Release-specific flags
function(dw_set_release_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:-O3 -DNDEBUG>
        )
    elseif(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:/O2 /DNDEBUG>
        )
    endif()
endfunction()

# Mark dependency include directories as SYSTEM to suppress their warnings
function(dw_suppress_dependency_warnings target)
    if(TARGET glm::glm)
        get_target_property(GLM_INCLUDES glm::glm INTERFACE_INCLUDE_DIRECTORIES)
        if(GLM_INCLUDES)
            target_include_directories(${target} SYSTEM PRIVATE ${GLM_INCLUDES})
        endif()
    endif()
endfunction()

# Apply all flags to a target
function(dw_configure_target target)
    dw_set_warning_flags(${target})
    dw_set_debug_flags(${target})
    dw_set_release_flags(${target})
    dw_suppress_dependency_warnings(${target})
endfunction()
