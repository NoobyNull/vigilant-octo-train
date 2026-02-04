#pragma once

#include "../types.h"

#include <string>
#include <vector>

namespace dw {

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

    // UI preferences
    bool getDarkMode() const { return m_darkMode; }
    void setDarkMode(bool enabled) { m_darkMode = enabled; }

    f32 getUiScale() const { return m_uiScale; }
    void setUiScale(f32 scale) { m_uiScale = scale; }

    bool getShowGrid() const { return m_showGrid; }
    void setShowGrid(bool show) { m_showGrid = show; }

    bool getShowAxis() const { return m_showAxis; }
    void setShowAxis(bool show) { m_showAxis = show; }

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

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    Path getConfigFilePath() const;

    // Recent projects (most recent first, max 10)
    std::vector<Path> m_recentProjects;
    static constexpr int MAX_RECENT_PROJECTS = 10;

    // UI preferences
    bool m_darkMode = true;
    f32 m_uiScale = 1.0f;
    bool m_showGrid = true;
    bool m_showAxis = true;

    // Default paths
    Path m_lastImportDir;
    Path m_lastExportDir;
    Path m_lastProjectDir;

    // Window state
    int m_windowWidth = 1600;
    int m_windowHeight = 900;
    bool m_windowMaximized = false;
};

}  // namespace dw
