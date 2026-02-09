#pragma once

// Digital Workshop - Application Class
// Main application lifecycle: init, run loop, shutdown.
// UI ownership delegated to UIManager (src/managers/ui_manager.h).
// File I/O orchestration delegated to FileIOManager (src/managers/file_io_manager.h).

#include <memory>
#include <string>
#include <vector>

#include "../core/types.h"

struct SDL_Window;

namespace dw {

// Forward declarations
class EventBus;
class Database;
class ConnectionPool;
class LibraryManager;
class ProjectManager;
class Workspace;
class ThumbnailGenerator;
class ConfigWatcher;
class ImportQueue;
class MainThreadQueue;

} // namespace dw

// Forward declare to avoid pulling in managers/*.h
namespace dw {
class UIManager;
class FileIOManager;
} // namespace dw

namespace dw {

class Application {
  public:
    Application();
    ~Application();

    // Disable copy
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Disable move (singleton-like usage)
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // Initialize SDL2, OpenGL, ImGui
    bool init();

    // Main loop - returns exit code
    int run();

    // Request application to quit
    void quit();

    // Accessors
    auto isRunning() const -> bool { return m_running; }
    auto getWindow() const -> SDL_Window* { return m_window; }
    auto eventBus() -> EventBus&;
    auto mainThreadQueue() -> MainThreadQueue&;

  private:
    void processEvents();
    void update();
    void render();
    void shutdown();

    // Callbacks (business logic stays in Application)
    void onModelSelected(int64_t modelId);

    // Config watcher (stays in Application until Plan 03)
    void onConfigFileChanged();
    void applyConfig();
    void spawnSettingsApp();
    void relaunchApp();
    void saveWorkspaceState();

    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    bool m_running = false;
    bool m_initialized = false;

    // Core systems
    std::unique_ptr<MainThreadQueue> m_mainThreadQueue;
    std::unique_ptr<EventBus> m_eventBus;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<ConnectionPool> m_connectionPool;
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<ThumbnailGenerator> m_thumbnailGenerator;
    std::unique_ptr<ImportQueue> m_importQueue;

    // UI Manager - owns all panels, dialogs, visibility state
    std::unique_ptr<UIManager> m_uiManager;

    // File I/O Manager - orchestrates import, export, project operations
    std::unique_ptr<FileIOManager> m_fileIOManager;

    // Config watching (stays in Application until Plan 03)
    std::unique_ptr<ConfigWatcher> m_configWatcher;
    float m_lastAppliedUiScale = 1.0f;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Digital Workshop";
};

} // namespace dw
