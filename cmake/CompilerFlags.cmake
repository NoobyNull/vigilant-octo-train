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

        # Treat warnings as errors in Debug builds only (Release may have dependency warnings)
        # But suppress conversion/cast-qual/double-promotion errors from external libraries (GLM, ImGui, STB)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE
                -Werror
                -Wno-error=conversion
                -Wno-error=sign-conversion
                -Wno-error=duplicated-branches
                -Wno-error=cast-qual
                -Wno-error=double-promotion
                -Wno-error=missing-field-initializers
                -Wno-c++20-extensions
            )
        endif()

    elseif(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
        )

        # Treat warnings as errors in Debug builds only (Release may have dependency warnings)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
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
    foreach(dep glm::glm imgui)
        if(TARGET ${dep})
            get_target_property(_INCLUDES ${dep} INTERFACE_INCLUDE_DIRECTORIES)
            if(_INCLUDES)
                target_include_directories(${target} SYSTEM PRIVATE ${_INCLUDES})
            endif()
            get_target_property(_SRC_DIR ${dep} SOURCE_DIR)
            if(_SRC_DIR)
                target_include_directories(${target} SYSTEM PRIVATE ${_SRC_DIR})
            endif()
        endif()
    endforeach()
endfunction()

# Apply all flags to a target
function(dw_configure_target target)
    dw_set_warning_flags(${target})
    dw_set_debug_flags(${target})
    dw_set_release_flags(${target})
    dw_suppress_dependency_warnings(${target})
endfunction()
