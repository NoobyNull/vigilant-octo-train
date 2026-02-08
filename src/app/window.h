#pragma once

// Digital Workshop - Window Management
// Wraps SDL2 window and OpenGL context creation

struct SDL_Window;

namespace dw {

struct WindowConfig {
    int width = 1280;
    int height = 720;
    const char* title = "Digital Workshop";
    bool vsync = true;
};

class Window {
  public:
    Window() = default;
    ~Window();

    // Disable copy and move
    Window(const Window&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    Window(Window&&) = delete;
    auto operator=(Window&&) -> Window& = delete;

    // Create window and OpenGL context
    auto create(const WindowConfig& config) -> bool;

    // Destroy window and context
    void destroy();

    // Accessors
    auto getSDLWindow() const -> SDL_Window* { return m_window; }
    auto getGLContext() const -> void* { return m_glContext; }
    auto isOpen() const -> bool { return m_window != nullptr; }

    // Get current drawable size (may differ from window size on HiDPI)
    void getDrawableSize(int& width, int& height) const;

    // Swap buffers
    void swapBuffers();

  private:
    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
};

} // namespace dw
