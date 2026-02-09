#pragma once

// Digital Workshop - Application Class
// Main application lifecycle: init, run loop, shutdown

#include <memory>
#include <string>
#include <vector>

#include "../core/types.h"

struct SDL_Window;

namespace dw {

// Forward declarations
class EventBus;
class Database;
class LibraryManager;
class ProjectManager;
class Workspace;
class ViewportPanel;
class LibraryPanel;
class PropertiesPanel;
class ProjectPanel;
class GCodePanel;
class CutOptimizerPanel;
class StartPage;
class ThumbnailGenerator;
class FileDialog;
class ConfigWatcher;
class LightingDialog;
class MessageDialog;
class ConfirmDialog;
class ImportQueue;

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
    auto eventBus() -> EventBus& { return *m_eventBus; }

  private:
    void processEvents();
    void update();
    void render();
    void shutdown();

    void setupMenus();
    void renderMenuBar();
    void renderPanels();
    void renderImportProgress();

    // Callbacks
    void onImportModel();
    void onExportModel();
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onModelSelected(int64_t modelId);
    void onFilesDropped(const std::vector<std::string>& paths);
    void processCompletedImports();
    void showAboutDialog();
    void handleKeyboardShortcuts();

    // Config watcher
    void onConfigFileChanged();
    void applyConfig();
    void spawnSettingsApp();
    void relaunchApp();
    void renderRestartPopup();
    void saveWorkspaceState();
    void onOpenRecentProject(const Path& path);
    void setupDefaultDockLayout(unsigned int dockspaceId);

    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    bool m_running = false;
    bool m_initialized = false;

    // Core systems
    std::unique_ptr<EventBus> m_eventBus;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<ThumbnailGenerator> m_thumbnailGenerator;
    std::unique_ptr<ImportQueue> m_importQueue;

    // UI Panels
    std::unique_ptr<ViewportPanel> m_viewportPanel;
    std::unique_ptr<LibraryPanel> m_libraryPanel;
    std::unique_ptr<PropertiesPanel> m_propertiesPanel;
    std::unique_ptr<ProjectPanel> m_projectPanel;
    std::unique_ptr<GCodePanel> m_gcodePanel;
    std::unique_ptr<CutOptimizerPanel> m_cutOptimizerPanel;
    std::unique_ptr<StartPage> m_startPage;

    // Panel visibility
    bool m_showViewport = true;
    bool m_showLibrary = true;
    bool m_showProperties = true;
    bool m_showProject = true;
    bool m_showGCode = false;
    bool m_showCutOptimizer = false;
    bool m_showStartPage = true;

    // Dialogs
    std::unique_ptr<FileDialog> m_fileDialog;
    std::unique_ptr<LightingDialog> m_lightingDialog;

    // Config watching
    std::unique_ptr<ConfigWatcher> m_configWatcher;
    bool m_showRestartPopup = false;
    float m_lastAppliedUiScale = 1.0f;
    bool m_firstFrame = true;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Digital Workshop";
};

} // namespace dw
