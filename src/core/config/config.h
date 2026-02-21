#pragma once

#include <array>
#include <string>
#include <vector>

#include "../types.h"
#include "input_binding.h"

namespace dw {

// Forward declaration for ParallelismTier from thread_pool.h
enum class ParallelismTier;

// Navigation style for 3D viewport
enum class NavStyle : int {
    Default = 0, // Left=Orbit, Shift+Left=Pan, Middle=Pan, Right=Zoom
    CAD = 1,     // Middle=Orbit, Shift+Middle=Pan, Right=Pan, Scroll=Zoom
    Maya = 2,    // Alt+Left=Orbit, Alt+Middle=Pan, Alt+Right=Zoom
};

// File handling mode for imported models
enum class FileHandlingMode : int {
    LeaveInPlace = 0,  // Leave files at original location (no copy)
    CopyToLibrary = 1, // Copy imported files to library directory
    MoveToLibrary = 2, // Move imported files to library directory (delete original)
};

// Application configuration (persisted to config directory)
class Config {
  public:
    // Singleton access
    static Config& instance();

    // Load/save configuration
    bool load();
    bool save();

    // Recent projects
    const std::vector<Path>& getRecentProjects() const { return m_recentProjects; }
    void addRecentProject(const Path& path);
    void removeRecentProject(const Path& path);
    void clearRecentProjects();

    // Config file path (for external watcher)
    Path configFilePath() const { return getConfigFilePath(); }

    // Theme (0=Dark, 1=Light, 2=HighContrast)
    int getThemeIndex() const { return m_themeIndex; }
    void setThemeIndex(int index) { m_themeIndex = index; }

    f32 getUiScale() const { return m_uiScale; }
    void setUiScale(f32 scale) { m_uiScale = scale; }

    bool getShowGrid() const { return m_showGrid; }
    void setShowGrid(bool show) { m_showGrid = show; }

    bool getShowAxis() const { return m_showAxis; }
    void setShowAxis(bool show) { m_showAxis = show; }

    bool getAutoOrient() const { return m_autoOrient; }
    void setAutoOrient(bool v) { m_autoOrient = v; }

    bool getInvertOrbitX() const { return m_invertOrbitX; }
    void setInvertOrbitX(bool v) { m_invertOrbitX = v; }

    bool getInvertOrbitY() const { return m_invertOrbitY; }
    void setInvertOrbitY(bool v) { m_invertOrbitY = v; }

    NavStyle getNavStyle() const { return m_navStyle; }
    void setNavStyle(NavStyle style) { m_navStyle = style; }
    int getNavStyleIndex() const { return static_cast<int>(m_navStyle); }
    void setNavStyleIndex(int index) { m_navStyle = static_cast<NavStyle>(index); }

    // Default paths
    const Path& getLastImportDir() const { return m_lastImportDir; }
    void setLastImportDir(const Path& path) { m_lastImportDir = path; }

    const Path& getLastExportDir() const { return m_lastExportDir; }
    void setLastExportDir(const Path& path) { m_lastExportDir = path; }

    const Path& getLastProjectDir() const { return m_lastProjectDir; }
    void setLastProjectDir(const Path& path) { m_lastProjectDir = path; }

    // Window state
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    void setWindowSize(int width, int height) {
        m_windowWidth = width;
        m_windowHeight = height;
    }

    bool getWindowMaximized() const { return m_windowMaximized; }
    void setWindowMaximized(bool maximized) { m_windowMaximized = maximized; }

    // Render settings (persisted)
    Vec3 getRenderLightDir() const { return m_lightDir; }
    void setRenderLightDir(const Vec3& dir) { m_lightDir = dir; }

    Vec3 getRenderLightColor() const { return m_lightColor; }
    void setRenderLightColor(const Vec3& c) { m_lightColor = c; }

    Vec3 getRenderAmbient() const { return m_ambient; }
    void setRenderAmbient(const Vec3& a) { m_ambient = a; }

    Color getRenderObjectColor() const { return m_objectColor; }
    void setRenderObjectColor(const Color& c) { m_objectColor = c; }

    f32 getRenderShininess() const { return m_shininess; }
    void setRenderShininess(f32 s) { m_shininess = s; }

    // Input bindings
    InputBinding getBinding(BindAction action) const;
    void setBinding(BindAction action, const InputBinding& binding);

    // Log level (maps to log::Level enum)
    int getLogLevel() const { return m_logLevel; }
    void setLogLevel(int level) { m_logLevel = level; }

    // Workspace state (panel visibility)
    bool getShowViewport() const { return m_wsShowViewport; }
    void setShowViewport(bool v) { m_wsShowViewport = v; }

    bool getShowLibrary() const { return m_wsShowLibrary; }
    void setShowLibrary(bool v) { m_wsShowLibrary = v; }

    bool getShowProperties() const { return m_wsShowProperties; }
    void setShowProperties(bool v) { m_wsShowProperties = v; }

    bool getShowProject() const { return m_wsShowProject; }
    void setShowProject(bool v) { m_wsShowProject = v; }

    bool getShowGCode() const { return m_wsShowGCode; }
    void setShowGCode(bool v) { m_wsShowGCode = v; }

    bool getShowMaterials() const { return m_wsShowMaterials; }
    void setShowMaterials(bool v) { m_wsShowMaterials = v; }

    bool getShowCutOptimizer() const { return m_wsShowCutOptimizer; }
    void setShowCutOptimizer(bool v) { m_wsShowCutOptimizer = v; }

    bool getShowCostEstimator() const { return m_wsShowCostEstimator; }
    void setShowCostEstimator(bool v) { m_wsShowCostEstimator = v; }

    bool getShowStartPage() const { return m_wsShowStartPage; }
    void setShowStartPage(bool v) { m_wsShowStartPage = v; }

    i64 getLastSelectedModelId() const { return m_wsLastSelectedModelId; }
    void setLastSelectedModelId(i64 id) { m_wsLastSelectedModelId = id; }

    f32 getLibraryThumbSize() const { return m_wsLibraryThumbSize; }
    void setLibraryThumbSize(f32 size) { m_wsLibraryThumbSize = size; }

    f32 getMaterialsThumbSize() const { return m_wsMaterialsThumbSize; }
    void setMaterialsThumbSize(f32 size) { m_wsMaterialsThumbSize = size; }

    // Import pipeline settings
    ParallelismTier getParallelismTier() const { return m_parallelismTier; }
    void setParallelismTier(ParallelismTier tier) { m_parallelismTier = tier; }

    FileHandlingMode getFileHandlingMode() const { return m_fileHandlingMode; }
    void setFileHandlingMode(FileHandlingMode mode) { m_fileHandlingMode = mode; }

    Path getLibraryDir() const { return m_libraryDir; }
    void setLibraryDir(const Path& path) { m_libraryDir = path; }

    bool getShowImportErrorToasts() const { return m_showImportErrorToasts; }
    void setShowImportErrorToasts(bool show) { m_showImportErrorToasts = show; }

    bool getEnableFloatingWindows() const { return m_enableFloatingWindows; }
    void setEnableFloatingWindows(bool enable) { m_enableFloatingWindows = enable; }

    // Default material
    i64 getDefaultMaterialId() const { return m_defaultMaterialId; }
    void setDefaultMaterialId(i64 id) { m_defaultMaterialId = id; }

    // API keys
    const std::string& getGeminiApiKey() const { return m_geminiApiKey; }
    void setGeminiApiKey(const std::string& key) { m_geminiApiKey = key; }

  private:
    Config();
    void initDefaultBindings();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    Path getConfigFilePath() const;

    // Recent projects (most recent first, max 10)
    std::vector<Path> m_recentProjects;
    static constexpr int MAX_RECENT_PROJECTS = 10;

    // UI preferences
    int m_themeIndex = 0; // 0=Dark, 1=Light, 2=HighContrast
    f32 m_uiScale = 1.0f;
    bool m_showGrid = true;
    bool m_showAxis = true;
    bool m_autoOrient = true;
    bool m_invertOrbitX = false;
    bool m_invertOrbitY = false;
    NavStyle m_navStyle = NavStyle::Default;

    // Render settings
    Vec3 m_lightDir{-0.5f, -1.0f, -0.3f};
    Vec3 m_lightColor{1.0f, 1.0f, 1.0f};
    Vec3 m_ambient{0.2f, 0.2f, 0.2f};
    Color m_objectColor{0.4f, 0.6f, 0.8f, 1.0f};
    f32 m_shininess = 32.0f;

    // Logging
    int m_logLevel = 1; // Info

    // Default paths
    Path m_lastImportDir;
    Path m_lastExportDir;
    Path m_lastProjectDir;

    // Window state
    int m_windowWidth = 1600;
    int m_windowHeight = 900;
    bool m_windowMaximized = false;

    // Workspace state
    bool m_wsShowViewport = true;
    bool m_wsShowLibrary = true;
    bool m_wsShowProperties = true;
    bool m_wsShowProject = true;
    bool m_wsShowMaterials = false;
    bool m_wsShowGCode = false;
    bool m_wsShowCutOptimizer = false;
    bool m_wsShowCostEstimator = false;
    bool m_wsShowStartPage = true;
    i64 m_wsLastSelectedModelId = -1;
    f32 m_wsLibraryThumbSize = 96.0f;
    f32 m_wsMaterialsThumbSize = 96.0f;

    // Input bindings
    std::array<InputBinding, static_cast<int>(BindAction::COUNT)> m_bindings;

    // Import pipeline settings
    ParallelismTier m_parallelismTier;
    FileHandlingMode m_fileHandlingMode = FileHandlingMode::LeaveInPlace;
    Path m_libraryDir; // Empty means default (app data dir / "library")
    bool m_showImportErrorToasts = true;
    bool m_enableFloatingWindows = false;

    // Default material
    i64 m_defaultMaterialId = -1;

    // API keys
    std::string m_geminiApiKey;
};

} // namespace dw
