#pragma once

// Digital Workshop - Application Class
// Main application lifecycle: init, run loop, shutdown

#include <memory>
#include <string>

struct SDL_Window;

namespace dw {

// Forward declarations
class Database;
class LibraryManager;
class ProjectManager;
class Workspace;
class ViewportPanel;
class LibraryPanel;
class PropertiesPanel;
class ProjectPanel;

class Application {
public:
    Application();
    ~Application();

    // Disable copy
    Application(const Application&) = delete;
    auto operator=(const Application&) -> Application& = delete;

    // Disable move (singleton-like usage)
    Application(Application&&) = delete;
    auto operator=(Application&&) -> Application& = delete;

    // Initialize SDL2, OpenGL, ImGui
    auto init() -> bool;

    // Main loop - returns exit code
    auto run() -> int;

    // Request application to quit
    void quit();

    // Accessors
    auto isRunning() const -> bool { return m_running; }
    auto getWindow() const -> SDL_Window* { return m_window; }

private:
    void processEvents();
    void update();
    void render();
    void shutdown();

    void setupMenus();
    void renderMenuBar();
    void renderPanels();

    // Callbacks
    void onImportModel();
    void onExportModel();
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onModelSelected(int64_t modelId);

    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    bool m_running = false;
    bool m_initialized = false;

    // Core systems
    std::unique_ptr<Database> m_database;
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<Workspace> m_workspace;

    // UI Panels
    std::unique_ptr<ViewportPanel> m_viewportPanel;
    std::unique_ptr<LibraryPanel> m_libraryPanel;
    std::unique_ptr<PropertiesPanel> m_propertiesPanel;
    std::unique_ptr<ProjectPanel> m_projectPanel;

    // Panel visibility
    bool m_showViewport = true;
    bool m_showLibrary = true;
    bool m_showProperties = true;
    bool m_showProject = true;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Digital Workshop";
};

} // namespace dw
