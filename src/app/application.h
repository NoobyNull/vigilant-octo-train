#pragma once

// Digital Workshop - Application Class
// Main application lifecycle: init, run loop, shutdown.
// UI ownership delegated to UIManager (src/managers/ui_manager.h).
// File I/O orchestration delegated to FileIOManager (src/managers/file_io_manager.h).
// Config management delegated to ConfigManager (src/managers/config_manager.h).

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../core/threading/loading_state.h"
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
class ImportQueue;
class MainThreadQueue;
class MaterialManager;
class Texture;

// Managers (extracted from Application)
class UIManager;
class FileIOManager;
class ConfigManager;

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
    void assignMaterialToCurrentModel(int64_t materialId);
    void loadMaterialTextureForModel(int64_t modelId);

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

    // Config Manager - config watching, applying, workspace state, settings, relaunch
    std::unique_ptr<ConfigManager> m_configManager;

    // Materials Manager - coordinates material archives, defaults, and database
    std::unique_ptr<MaterialManager> m_materialManager;

    // Active material texture for rendering (cached GPU texture)
    std::unique_ptr<Texture> m_activeMaterialTexture;
    int64_t m_activeMaterialId = -1;

    // Async model loading
    LoadingState m_loadingState;
    std::thread m_loadThread;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Digital Workshop";
};

} // namespace dw
