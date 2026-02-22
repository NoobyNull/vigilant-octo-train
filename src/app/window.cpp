// Digital Workshop - Window Implementation

#include "app/window.h"

#include <cstdio>

#include <glad/gl.h>
#include <SDL.h>

namespace dw {

Window::~Window() {
    destroy();
}

auto Window::create(const WindowConfig& config) -> bool {
    if (m_window != nullptr) {
        return true; // Already created
    }

    // Initialize SDL2 video subsystem
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // Request OpenGL 3.3 Core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    auto flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                              SDL_WINDOW_ALLOW_HIGHDPI);

    m_window = SDL_CreateWindow(config.title,
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                config.width,
                                config.height,
                                flags);

    if (m_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);
    if (m_glContext == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        destroy();
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);

    // Load OpenGL functions
    int gladVersion = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (gladVersion == 0) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        destroy();
        return false;
    }

    std::printf("OpenGL %s\n", glGetString(GL_VERSION));
    return true;
}

void Window::destroy() {
    if (m_glContext != nullptr) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void Window::getDrawableSize(int& width, int& height) const {
    if (m_window != nullptr) {
        SDL_GL_GetDrawableSize(m_window, &width, &height);
    } else {
        width = 0;
        height = 0;
    }
}

void Window::swapBuffers() {
    if (m_window != nullptr) {
        SDL_GL_SwapWindow(m_window);
    }
}

} // namespace dw
