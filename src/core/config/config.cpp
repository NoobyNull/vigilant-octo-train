#include "config.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "../paths/app_paths.h"
#include "../threading/thread_pool.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

Config::Config() : m_parallelismTier(ParallelismTier::Auto) {
    initDefaultBindings();
}

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
        log::info("Config", "No config file found, using defaults");
        return true;
    }

    auto content = file::readText(configPath);
    if (!content) {
        log::error("Config", "Failed to read config file");
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
            if (key == "theme") {
                str::parseInt(value, m_themeIndex);
            } else if (key == "scale") {
                str::parseFloat(value, m_uiScale);
            } else if (key == "show_grid") {
                m_showGrid = (value == "true" || value == "1");
            } else if (key == "show_axis") {
                m_showAxis = (value == "true" || value == "1");
            } else if (key == "auto_orient") {
                m_autoOrient = (value == "true" || value == "1");
            } else if (key == "invert_orbit_x") {
                m_invertOrbitX = (value == "true" || value == "1");
            } else if (key == "invert_orbit_y") {
                m_invertOrbitY = (value == "true" || value == "1");
            } else if (key == "nav_style") {
                int style = 0;
                str::parseInt(value, style);
                if (style >= 0 && style <= 2)
                    m_navStyle = static_cast<NavStyle>(style);
            } else if (key == "floating_windows") {
                m_enableFloatingWindows = (value == "true" || value == "1");
            }
        } else if (section == "render") {
            if (key == "light_dir_x")
                str::parseFloat(value, m_lightDir.x);
            else if (key == "light_dir_y")
                str::parseFloat(value, m_lightDir.y);
            else if (key == "light_dir_z")
                str::parseFloat(value, m_lightDir.z);
            else if (key == "light_color_r")
                str::parseFloat(value, m_lightColor.x);
            else if (key == "light_color_g")
                str::parseFloat(value, m_lightColor.y);
            else if (key == "light_color_b")
                str::parseFloat(value, m_lightColor.z);
            else if (key == "ambient_r")
                str::parseFloat(value, m_ambient.x);
            else if (key == "ambient_g")
                str::parseFloat(value, m_ambient.y);
            else if (key == "ambient_b")
                str::parseFloat(value, m_ambient.z);
            else if (key == "object_color_r")
                str::parseFloat(value, m_objectColor.r);
            else if (key == "object_color_g")
                str::parseFloat(value, m_objectColor.g);
            else if (key == "object_color_b")
                str::parseFloat(value, m_objectColor.b);
            else if (key == "shininess")
                str::parseFloat(value, m_shininess);
        } else if (section == "logging") {
            if (key == "level") {
                str::parseInt(value, m_logLevel);
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
        } else if (section == "workspace") {
            if (key == "show_viewport") {
                m_wsShowViewport = (value == "true" || value == "1");
            } else if (key == "show_library") {
                m_wsShowLibrary = (value == "true" || value == "1");
            } else if (key == "show_properties") {
                m_wsShowProperties = (value == "true" || value == "1");
            } else if (key == "show_project") {
                m_wsShowProject = (value == "true" || value == "1");
            } else if (key == "show_materials") {
                m_wsShowMaterials = (value == "true" || value == "1");
            } else if (key == "show_gcode") {
                m_wsShowGCode = (value == "true" || value == "1");
            } else if (key == "show_cut_optimizer") {
                m_wsShowCutOptimizer = (value == "true" || value == "1");
            } else if (key == "show_cost_estimator") {
                m_wsShowCostEstimator = (value == "true" || value == "1");
            } else if (key == "show_start_page") {
                m_wsShowStartPage = (value == "true" || value == "1");
            } else if (key == "last_selected_model") {
                str::parseInt64(value, m_wsLastSelectedModelId);
            } else if (key == "library_thumb_size") {
                str::parseFloat(value, m_wsLibraryThumbSize);
            } else if (key == "materials_thumb_size") {
                str::parseFloat(value, m_wsMaterialsThumbSize);
            }
        } else if (section == "bindings") {
            if (key == "light_dir_drag") {
                m_bindings[static_cast<int>(BindAction::LightDirDrag)] =
                    InputBinding::deserialize(value);
            } else if (key == "light_intensity_drag") {
                m_bindings[static_cast<int>(BindAction::LightIntensityDrag)] =
                    InputBinding::deserialize(value);
            }
        } else if (section == "import") {
            if (key == "parallelism_tier") {
                int tier = 0;
                str::parseInt(value, tier);
                if (tier >= 0 && tier <= 2)
                    m_parallelismTier = static_cast<ParallelismTier>(tier);
            } else if (key == "file_handling_mode") {
                int mode = 0;
                str::parseInt(value, mode);
                if (mode >= 0 && mode <= 2)
                    m_fileHandlingMode = static_cast<FileHandlingMode>(mode);
            } else if (key == "library_dir") {
                m_libraryDir = value;
            } else if (key == "show_error_toasts") {
                m_showImportErrorToasts = (value == "true" || value == "1");
            }
        } else if (section == "materials") {
            if (key == "default_material_id") {
                str::parseInt64(value, m_defaultMaterialId);
            }
        } else if (section == "api") {
            if (key == "gemini_key") {
                m_geminiApiKey = value;
            }
        } else if (section == "recent") {
            if (str::startsWith(key, "project")) {
                if (!value.empty() && file::exists(value)) {
                    m_recentProjects.push_back(value);
                }
            }
        }
    }

    log::infof("Config", "Loaded from %s", configPath.string().c_str());
    return true;
}

bool Config::save() {
    Path configPath = getConfigFilePath();

    // Ensure config directory exists
    if (!file::createDirectories(configPath.parent_path())) {
        log::error("Config", "Failed to create config directory");
        return false;
    }

    std::ostringstream ss;
    ss << std::fixed;

    ss << "# Digital Workshop Configuration\n\n";

    // UI section
    ss << "[ui]\n";
    ss << "theme=" << m_themeIndex << "\n";
    ss << "scale=" << m_uiScale << "\n";
    ss << "show_grid=" << (m_showGrid ? "true" : "false") << "\n";
    ss << "show_axis=" << (m_showAxis ? "true" : "false") << "\n";
    ss << "auto_orient=" << (m_autoOrient ? "true" : "false") << "\n";
    ss << "invert_orbit_x=" << (m_invertOrbitX ? "true" : "false") << "\n";
    ss << "invert_orbit_y=" << (m_invertOrbitY ? "true" : "false") << "\n";
    ss << "nav_style=" << static_cast<int>(m_navStyle) << "\n";
    ss << "floating_windows=" << (m_enableFloatingWindows ? "true" : "false") << "\n";
    ss << "\n";

    // Render section
    ss << "[render]\n";
    ss << "light_dir_x=" << m_lightDir.x << "\n";
    ss << "light_dir_y=" << m_lightDir.y << "\n";
    ss << "light_dir_z=" << m_lightDir.z << "\n";
    ss << "light_color_r=" << m_lightColor.x << "\n";
    ss << "light_color_g=" << m_lightColor.y << "\n";
    ss << "light_color_b=" << m_lightColor.z << "\n";
    ss << "ambient_r=" << m_ambient.x << "\n";
    ss << "ambient_g=" << m_ambient.y << "\n";
    ss << "ambient_b=" << m_ambient.z << "\n";
    ss << "object_color_r=" << m_objectColor.r << "\n";
    ss << "object_color_g=" << m_objectColor.g << "\n";
    ss << "object_color_b=" << m_objectColor.b << "\n";
    ss << "shininess=" << m_shininess << "\n";
    ss << "\n";

    // Logging section
    ss << "[logging]\n";
    ss << "level=" << m_logLevel << "\n";
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

    // Workspace section
    ss << "[workspace]\n";
    ss << "show_viewport=" << (m_wsShowViewport ? "true" : "false") << "\n";
    ss << "show_library=" << (m_wsShowLibrary ? "true" : "false") << "\n";
    ss << "show_properties=" << (m_wsShowProperties ? "true" : "false") << "\n";
    ss << "show_project=" << (m_wsShowProject ? "true" : "false") << "\n";
    ss << "show_materials=" << (m_wsShowMaterials ? "true" : "false") << "\n";
    ss << "show_gcode=" << (m_wsShowGCode ? "true" : "false") << "\n";
    ss << "show_cut_optimizer=" << (m_wsShowCutOptimizer ? "true" : "false") << "\n";
    ss << "show_cost_estimator=" << (m_wsShowCostEstimator ? "true" : "false") << "\n";
    ss << "show_start_page=" << (m_wsShowStartPage ? "true" : "false") << "\n";
    ss << "last_selected_model=" << m_wsLastSelectedModelId << "\n";
    ss << "library_thumb_size=" << m_wsLibraryThumbSize << "\n";
    ss << "materials_thumb_size=" << m_wsMaterialsThumbSize << "\n";
    ss << "\n";

    // Bindings section
    ss << "[bindings]\n";
    ss << "light_dir_drag=" << m_bindings[static_cast<int>(BindAction::LightDirDrag)].serialize()
       << "\n";
    ss << "light_intensity_drag="
       << m_bindings[static_cast<int>(BindAction::LightIntensityDrag)].serialize() << "\n";
    ss << "\n";

    // Import section
    ss << "[import]\n";
    ss << "parallelism_tier=" << static_cast<int>(m_parallelismTier) << "\n";
    ss << "file_handling_mode=" << static_cast<int>(m_fileHandlingMode) << "\n";
    if (!m_libraryDir.empty()) {
        ss << "library_dir=" << m_libraryDir.string() << "\n";
    }
    ss << "show_error_toasts=" << (m_showImportErrorToasts ? "true" : "false") << "\n";
    ss << "\n";

    // Materials section
    ss << "[materials]\n";
    ss << "default_material_id=" << m_defaultMaterialId << "\n";
    ss << "\n";

    // API section
    ss << "[api]\n";
    if (!m_geminiApiKey.empty()) {
        ss << "gemini_key=" << m_geminiApiKey << "\n";
    }
    ss << "\n";

    // Recent projects section
    ss << "[recent]\n";
    for (size_t i = 0; i < m_recentProjects.size(); ++i) {
        ss << "project" << i << "=" << m_recentProjects[i].string() << "\n";
    }

    // Atomic save: write to temp file, then rename
    Path tempPath = configPath.string() + ".tmp";
    if (!file::writeText(tempPath, ss.str())) {
        log::error("Config", "Failed to save config file");
        return false;
    }

    std::error_code ec;
    fs::rename(tempPath, configPath, ec);
    if (ec) {
        log::errorf("Config", "Failed to rename temp config: %s", ec.message().c_str());
        return false;
    }

    log::debugf("Config", "Saved to %s", configPath.string().c_str());
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

void Config::initDefaultBindings() {
    for (int i = 0; i < static_cast<int>(BindAction::COUNT); ++i) {
        m_bindings[static_cast<size_t>(i)] = defaultBinding(static_cast<BindAction>(i));
    }
}

InputBinding Config::getBinding(BindAction action) const {
    int idx = static_cast<int>(action);
    if (idx >= 0 && idx < static_cast<int>(BindAction::COUNT)) {
        return m_bindings[static_cast<size_t>(idx)];
    }
    return {};
}

void Config::setBinding(BindAction action, const InputBinding& binding) {
    int idx = static_cast<int>(action);
    if (idx >= 0 && idx < static_cast<int>(BindAction::COUNT)) {
        m_bindings[static_cast<size_t>(idx)] = binding;
    }
}

} // namespace dw
