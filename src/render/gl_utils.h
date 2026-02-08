#pragma once

#include <string>

#include <glad/gl.h>

namespace dw {
namespace gl {

// Check for OpenGL errors and log them
bool checkError(const char* operation);

// RAII scope guard for error checking
#ifdef NDEBUG
    #define GL_CHECK(op) op
#else
    #define GL_CHECK(op)                                                                           \
        do {                                                                                       \
            op;                                                                                    \
            gl::checkError(#op);                                                                   \
        } while (0)
#endif

// Get OpenGL version string
std::string getVersionString();

// Get OpenGL renderer string
std::string getRendererString();

} // namespace gl
} // namespace dw
