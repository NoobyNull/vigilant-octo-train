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
class Database;
class ConnectionPool;
class LibraryManager;
class ProjectManager;
class Workspace;
class ThumbnailGenerator;
class ImportQueue;
class MainThreadQueue;
class StorageManager;
class MaterialManager;
class CostRepository;
class GraphManager;
class ModelRepository;
class GCodeRepository;
class CutPlanRepository;
class GeminiMaterialService;
class GeminiDescriptorService;
class ProjectExportManager;
class CutListFile;
class CncController;
class ToolDatabase;
class MacroManager;
class BackgroundTagger;
class ImportLog;
class Mesh;
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
    auto mainThreadQueue() -> MainThreadQueue&;

  private:
    void processEvents();
    void update();
    void render();
    void shutdown();
    void initWiring(); // Panel/callback wiring — implemented in application_wiring.cpp

    // Callbacks (business logic stays in Application)
    void onModelSelected(int64_t modelId);
    void assignMaterialToCurrentModel(int64_t materialId);
    void loadMaterialTextureForModel(int64_t modelId);
    bool generateMaterialThumbnail(int64_t modelId, Mesh& mesh);

    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    bool m_running = false;
    bool m_initialized = false;

    // Core systems
    std::unique_ptr<MainThreadQueue> m_mainThreadQueue;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<ConnectionPool> m_connectionPool;
    std::unique_ptr<LibraryManager> m_libraryManager;
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<Workspace> m_workspace;
    std::unique_ptr<ThumbnailGenerator> m_thumbnailGenerator;
    std::unique_ptr<ImportQueue> m_importQueue;
    std::unique_ptr<ImportLog> m_importLog;
    std::unique_ptr<BackgroundTagger> m_backgroundTagger;
    std::unique_ptr<StorageManager> m_storageManager;

    // UI Manager - owns all panels, dialogs, visibility state
    std::unique_ptr<UIManager> m_uiManager;

    // File I/O Manager - orchestrates import, export, project operations
    std::unique_ptr<FileIOManager> m_fileIOManager;

    // Config Manager - config watching, applying, workspace state, settings, relaunch
    std::unique_ptr<ConfigManager> m_configManager;

    // Materials Manager - coordinates material archives, defaults, and database
    std::unique_ptr<MaterialManager> m_materialManager;

    // Repositories for project asset navigator
    std::unique_ptr<ModelRepository> m_modelRepo;
    std::unique_ptr<GCodeRepository> m_gcodeRepo;
    std::unique_ptr<CutPlanRepository> m_cutPlanRepo;

    // File-based cut list persistence
    std::unique_ptr<CutListFile> m_cutListFile;

    // Cost estimation repository
    std::unique_ptr<CostRepository> m_costRepo;

    // Graph query engine (Cypher via GraphQLite extension)
    std::unique_ptr<GraphManager> m_graphManager;

    // Gemini AI material generation service
    std::unique_ptr<GeminiMaterialService> m_geminiService;

    // Gemini AI model descriptor (thumbnail classification)
    std::unique_ptr<GeminiDescriptorService> m_descriptorService;

    // Project export/import (.dwproj archives)
    std::unique_ptr<ProjectExportManager> m_projectExportManager;

    // CNC tool database (Vectric .vtdb format)
    std::unique_ptr<ToolDatabase> m_toolDatabase;

    // CNC controller (multi-firmware support: GRBL, grblHAL, FluidNC, Smoothieware)
    std::unique_ptr<CncController> m_cncController;

    // CNC macro manager (SQLite-backed macro storage)
    std::unique_ptr<MacroManager> m_macroManager;

    // Currently focused model ID (for material assignment)
    int64_t m_focusedModelId = -1;

    // Active material texture for rendering (cached GPU texture)
    std::unique_ptr<Texture> m_activeMaterialTexture;
    int64_t m_activeMaterialId = -1;

    // Model loading state and thread (for async mesh loading)
    LoadingState m_loadingState;
    std::thread m_loadThread;

    // DPI scaling
    void rebuildFontAtlas(float scale);
    float detectDpiScale() const;
    float m_dpiScale = 1.0f;
    float m_uiScale = 1.0f; // Combined dpi * user scale
    int m_displayIndex = 0;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Digital Workshop";
};

} // namespace dw
