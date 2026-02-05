#include "gl_utils.h"

#include "../core/utils/log.h"

namespace dw {
namespace gl {

bool checkError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        const char* errorStr = "Unknown";
        switch (error) {
            case GL_INVALID_ENUM:
                errorStr = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorStr = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorStr = "GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorStr = "GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        log::errorf("GL", "Error %s at: %s", errorStr, operation);
        return false;
    }
    return true;
}

std::string getVersionString() {
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    return version ? version : "Unknown";
}

std::string getRendererString() {
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    return renderer ? renderer : "Unknown";
}

}  // namespace gl
}  // namespace dw
