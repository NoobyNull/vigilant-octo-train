#include "config.h"

#include "../paths/app_paths.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace dw {

Config& Config::instance() {
    static Config instance;
    return instance;
}

Path Config::getConfigFilePath() const {
    return paths::getConfigDir() / "config.ini";
}

bool Config::load() {
    Path configPath = getConfigFilePath();

    if (!file::exists(configPath)) {
        log::info("No config file found, using defaults");
        return true;
    }

    auto content = file::readText(configPath);
    if (!content) {
        log::error("Failed to read config file");
        return false;
    }

    std::istringstream stream(*content);
    std::string line;
    std::string section;

    while (std::getline(stream, line)) {
        line = str::trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Section header
        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }

        // Key=Value pair
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = str::trim(line.substr(0, pos));
        std::string value = str::trim(line.substr(pos + 1));

        // Parse based on section
        if (section == "ui") {
            if (key == "dark_mode") {
                m_darkMode = (value == "true" || value == "1");
            } else if (key == "scale") {
                str::parseFloat(value, m_uiScale);
            } else if (key == "show_grid") {
                m_showGrid = (value == "true" || value == "1");
            } else if (key == "show_axis") {
                m_showAxis = (value == "true" || value == "1");
            }
        } else if (section == "paths") {
            if (key == "last_import") {
                m_lastImportDir = value;
            } else if (key == "last_export") {
                m_lastExportDir = value;
            } else if (key == "last_project") {
                m_lastProjectDir = value;
            }
        } else if (section == "window") {
            if (key == "width") {
                str::parseInt(value, m_windowWidth);
            } else if (key == "height") {
                str::parseInt(value, m_windowHeight);
            } else if (key == "maximized") {
                m_windowMaximized = (value == "true" || value == "1");
            }
        } else if (section == "recent") {
            if (str::startsWith(key, "project")) {
                if (!value.empty() && file::exists(value)) {
                    m_recentProjects.push_back(value);
                }
            }
        }
    }

    log::infof("Loaded config from %s", configPath.string().c_str());
    return true;
}

bool Config::save() {
    Path configPath = getConfigFilePath();

    // Ensure config directory exists
    file::createDirectories(configPath.parent_path());

    std::ostringstream ss;

    ss << "# Digital Workshop Configuration\n\n";

    // UI section
    ss << "[ui]\n";
    ss << "dark_mode=" << (m_darkMode ? "true" : "false") << "\n";
    ss << "scale=" << m_uiScale << "\n";
    ss << "show_grid=" << (m_showGrid ? "true" : "false") << "\n";
    ss << "show_axis=" << (m_showAxis ? "true" : "false") << "\n";
    ss << "\n";

    // Paths section
    ss << "[paths]\n";
    if (!m_lastImportDir.empty()) {
        ss << "last_import=" << m_lastImportDir.string() << "\n";
    }
    if (!m_lastExportDir.empty()) {
        ss << "last_export=" << m_lastExportDir.string() << "\n";
    }
    if (!m_lastProjectDir.empty()) {
        ss << "last_project=" << m_lastProjectDir.string() << "\n";
    }
    ss << "\n";

    // Window section
    ss << "[window]\n";
    ss << "width=" << m_windowWidth << "\n";
    ss << "height=" << m_windowHeight << "\n";
    ss << "maximized=" << (m_windowMaximized ? "true" : "false") << "\n";
    ss << "\n";

    // Recent projects section
    ss << "[recent]\n";
    for (size_t i = 0; i < m_recentProjects.size(); ++i) {
        ss << "project" << i << "=" << m_recentProjects[i].string() << "\n";
    }

    if (!file::writeText(configPath, ss.str())) {
        log::error("Failed to save config file");
        return false;
    }

    log::debugf("Saved config to %s", configPath.string().c_str());
    return true;
}

void Config::addRecentProject(const Path& path) {
    // Remove if already exists
    removeRecentProject(path);

    // Add to front
    m_recentProjects.insert(m_recentProjects.begin(), path);

    // Trim to max size
    if (m_recentProjects.size() > MAX_RECENT_PROJECTS) {
        m_recentProjects.resize(MAX_RECENT_PROJECTS);
    }
}

void Config::removeRecentProject(const Path& path) {
    auto it = std::remove(m_recentProjects.begin(), m_recentProjects.end(), path);
    m_recentProjects.erase(it, m_recentProjects.end());
}

void Config::clearRecentProjects() {
    m_recentProjects.clear();
}

}  // namespace dw
