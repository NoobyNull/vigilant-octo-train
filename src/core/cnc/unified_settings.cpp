#include "unified_settings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace dw {

// --- Initialization ---

namespace {

struct SettingDef {
    const char* key;
    const char* displayName;
    const char* category;
    const char* units;
    SettingType type;
    int grblId;
    const char* fluidncPath;
    float min;
    float max;
};

// Static mapping table: GRBL $N <-> FluidNC path
const SettingDef kDefinitions[] = {
    // Signal Configuration
    {"step_pulse_time",   "Step pulse time",      "signal", "us",      SettingType::Float, 0,  "stepping/pulse_us",          3,    255},
    {"step_idle_delay",   "Step idle delay",       "signal", "ms",      SettingType::Float, 1,  "stepping/idle_ms",           0,    255},
    {"step_pulse_invert", "Step pulse invert",     "signal", "",        SettingType::Bitmask, 2, "stepping/step_invert",      0,    7},
    {"direction_invert",  "Direction invert",      "signal", "",        SettingType::Bitmask, 3, "stepping/dir_invert",       0,    7},
    {"step_enable_invert","Step enable invert",    "signal", "",        SettingType::Bool,  4,  "stepping/disable_delay_us",  0,    1},
    {"limit_invert",      "Limit pins invert",     "signal", "",        SettingType::Bool,  5,  "",                           0,    1},
    {"probe_invert",      "Probe pin invert",      "signal", "",        SettingType::Bool,  6,  "probe/check_mode_start",     0,    1},

    // Status Report
    {"status_report",     "Status report options", "status", "",        SettingType::Bitmask, 10, "",                         0,    3},

    // Motion Parameters
    {"junction_deviation","Junction deviation",    "motion", "mm",      SettingType::Float, 11, "",                           0.001f, 1.0f},
    {"arc_tolerance",     "Arc tolerance",         "motion", "mm",      SettingType::Float, 12, "",                           0.001f, 1.0f},
    {"report_inches",     "Report in inches",      "motion", "",        SettingType::Bool,  13, "",                           0,    1},

    // Limits & Homing
    {"soft_limits",       "Soft limits",           "limits", "",        SettingType::Bool,  20, "",                           0,    1},
    {"hard_limits",       "Hard limits",           "limits", "",        SettingType::Bool,  21, "",                           0,    1},
    {"homing_enable",     "Homing cycle",          "limits", "",        SettingType::Bool,  22, "",                           0,    1},
    {"homing_dir_invert", "Homing direction invert","limits","",        SettingType::Bitmask, 23, "",                        0,    7},
    {"homing_feed",       "Homing feed rate",      "limits", "mm/min",  SettingType::Float, 24, "",                           1,    10000},
    {"homing_seek",       "Homing seek rate",      "limits", "mm/min",  SettingType::Float, 25, "",                           1,    10000},
    {"homing_debounce",   "Homing debounce",       "limits", "ms",      SettingType::Float, 26, "",                           0,    1000},
    {"homing_pulloff",    "Homing pull-off",       "limits", "mm",      SettingType::Float, 27, "",                           0,    100},

    // Spindle
    {"max_spindle",       "Max spindle speed",     "spindle","RPM",     SettingType::Float, 30, "spindle/pwm/max_rpm",        0,    100000},
    {"min_spindle",       "Min spindle speed",     "spindle","RPM",     SettingType::Float, 31, "spindle/pwm/min_rpm",        0,    100000},
    {"laser_mode",        "Laser mode",            "spindle","",        SettingType::Bool,  32, "",                           0,    1},

    // Per-axis: Steps per mm
    {"steps_per_mm_x",    "Steps per mm (X)",      "per_axis","steps/mm",SettingType::Float, 100,"axes/x/steps_per_mm",       1,    10000},
    {"steps_per_mm_y",    "Steps per mm (Y)",      "per_axis","steps/mm",SettingType::Float, 101,"axes/y/steps_per_mm",       1,    10000},
    {"steps_per_mm_z",    "Steps per mm (Z)",      "per_axis","steps/mm",SettingType::Float, 102,"axes/z/steps_per_mm",       1,    10000},

    // Per-axis: Max feed rate
    {"max_feed_x",        "Max feed rate (X)",     "per_axis","mm/min", SettingType::Float, 110,"axes/x/max_rate_mm_per_min", 1,    100000},
    {"max_feed_y",        "Max feed rate (Y)",     "per_axis","mm/min", SettingType::Float, 111,"axes/y/max_rate_mm_per_min", 1,    100000},
    {"max_feed_z",        "Max feed rate (Z)",     "per_axis","mm/min", SettingType::Float, 112,"axes/z/max_rate_mm_per_min", 1,    100000},

    // Per-axis: Acceleration
    {"accel_x",           "Acceleration (X)",      "per_axis","mm/s\xc2\xb2",SettingType::Float, 120,"axes/x/acceleration_mm_per_sec2",1, 10000},
    {"accel_y",           "Acceleration (Y)",      "per_axis","mm/s\xc2\xb2",SettingType::Float, 121,"axes/y/acceleration_mm_per_sec2",1, 10000},
    {"accel_z",           "Acceleration (Z)",      "per_axis","mm/s\xc2\xb2",SettingType::Float, 122,"axes/z/acceleration_mm_per_sec2",1, 10000},

    // Per-axis: Max travel
    {"max_travel_x",      "Max travel (X)",        "per_axis","mm",    SettingType::Float, 130,"axes/x/max_travel_mm",        1,    10000},
    {"max_travel_y",      "Max travel (Y)",        "per_axis","mm",    SettingType::Float, 131,"axes/y/max_travel_mm",        1,    10000},
    {"max_travel_z",      "Max travel (Z)",        "per_axis","mm",    SettingType::Float, 132,"axes/z/max_travel_mm",        1,    10000},
};

constexpr int kDefinitionCount = sizeof(kDefinitions) / sizeof(kDefinitions[0]);

} // namespace

UnifiedSettingsMap::UnifiedSettingsMap() {
    initDefinitions();
}

void UnifiedSettingsMap::initDefinitions() {
    m_settings.clear();
    m_grblIdToKey.clear();
    m_fluidncPathToKey.clear();

    for (int i = 0; i < kDefinitionCount; ++i) {
        const auto& d = kDefinitions[i];
        UnifiedSetting s;
        s.key = d.key;
        s.displayName = d.displayName;
        s.category = d.category;
        s.units = d.units;
        s.type = d.type;
        s.grblId = d.grblId;
        s.fluidncPath = d.fluidncPath;
        s.min = d.min;
        s.max = d.max;

        m_settings[s.key] = std::move(s);

        if (d.grblId >= 0) {
            m_grblIdToKey[d.grblId] = d.key;
        }
        if (d.fluidncPath[0] != '\0') {
            m_fluidncPathToKey[d.fluidncPath] = d.key;
        }
    }
}

// --- Parsing ---

bool UnifiedSettingsMap::parseGrblLine(const std::string& line) {
    // Expected: "$N=V" where N is integer
    if (line.empty() || line[0] != '$') return false;

    auto eqPos = line.find('=');
    if (eqPos == std::string::npos || eqPos < 2) return false;

    // Check if this is a numeric GRBL setting (not a FluidNC path)
    std::string idStr = line.substr(1, eqPos - 1);
    int id = 0;
    try {
        id = std::stoi(idStr);
    } catch (...) {
        return false;
    }

    std::string value = line.substr(eqPos + 1);

    auto it = m_grblIdToKey.find(id);
    if (it != m_grblIdToKey.end()) {
        auto& setting = m_settings[it->second];
        setting.value = value;
        setting.modified = false;
        return true;
    }

    // Unknown GRBL setting — store as extension
    std::string key = "grbl_" + std::to_string(id);
    if (m_settings.find(key) == m_settings.end()) {
        UnifiedSetting s;
        s.key = key;
        s.displayName = "GRBL $" + std::to_string(id);
        s.category = "extension";
        s.grblId = id;
        s.type = SettingType::Float;
        m_settings[key] = std::move(s);
        m_grblIdToKey[id] = key;
    }
    m_settings[key].value = value;
    m_settings[key].modified = false;
    return true;
}

bool UnifiedSettingsMap::parseFluidNCLine(const std::string& line) {
    // Expected: "$path=value" from $S response
    if (line.empty() || line[0] != '$') return false;

    auto eqPos = line.find('=');
    if (eqPos == std::string::npos || eqPos < 2) return false;

    std::string path = line.substr(1, eqPos - 1);
    std::string value = line.substr(eqPos + 1);

    auto it = m_fluidncPathToKey.find(path);
    if (it != m_fluidncPathToKey.end()) {
        auto& setting = m_settings[it->second];
        setting.value = value;
        setting.modified = false;
        return true;
    }

    // Unknown FluidNC setting — store as extension
    std::string key = "fnc_" + path;
    // Replace / with _ for map key
    std::replace(key.begin(), key.end(), '/', '_');

    if (m_settings.find(key) == m_settings.end()) {
        UnifiedSetting s;
        s.key = key;
        s.displayName = path;
        s.category = "extension";
        s.fluidncPath = path;
        s.type = SettingType::Float;
        m_settings[key] = std::move(s);
        m_fluidncPathToKey[path] = key;
    }
    m_settings[key].value = value;
    m_settings[key].modified = false;
    return true;
}

void UnifiedSettingsMap::markChangedFromDefault(const std::string& line) {
    // $SC line has same format as $S: "$path=value"
    if (line.empty() || line[0] != '$') return;

    auto eqPos = line.find('=');
    if (eqPos == std::string::npos || eqPos < 2) return;

    std::string path = line.substr(1, eqPos - 1);

    auto it = m_fluidncPathToKey.find(path);
    if (it != m_fluidncPathToKey.end()) {
        m_settings[it->second].changedFromDefault = true;
    }
}

// --- Command building ---

std::string UnifiedSettingsMap::buildSetCommand(const std::string& key,
                                                 const std::string& value,
                                                 FirmwareType fw) const {
    auto it = m_settings.find(key);
    if (it == m_settings.end()) return "";

    const auto& s = it->second;

    if (fw == FirmwareType::FluidNC && !s.fluidncPath.empty()) {
        return "$/" + s.fluidncPath + "=" + value;
    }

    if (s.grblId >= 0) {
        return "$" + std::to_string(s.grblId) + "=" + value;
    }

    return "";
}

// --- Access ---

const UnifiedSetting* UnifiedSettingsMap::get(const std::string& key) const {
    auto it = m_settings.find(key);
    return it != m_settings.end() ? &it->second : nullptr;
}

bool UnifiedSettingsMap::set(const std::string& key, const std::string& value) {
    auto it = m_settings.find(key);
    if (it == m_settings.end()) return false;
    it->second.value = value;
    it->second.modified = true;
    return true;
}

const UnifiedSetting* UnifiedSettingsMap::getByGrblId(int id) const {
    auto it = m_grblIdToKey.find(id);
    if (it == m_grblIdToKey.end()) return nullptr;
    auto sit = m_settings.find(it->second);
    return sit != m_settings.end() ? &sit->second : nullptr;
}

const UnifiedSetting* UnifiedSettingsMap::getByFluidNCPath(const std::string& path) const {
    auto it = m_fluidncPathToKey.find(path);
    if (it == m_fluidncPathToKey.end()) return nullptr;
    auto sit = m_settings.find(it->second);
    return sit != m_settings.end() ? &sit->second : nullptr;
}

// --- Grouped access ---

std::vector<std::pair<std::string, std::vector<const UnifiedSetting*>>>
UnifiedSettingsMap::getByCategory() const {
    static const char* kCategoryOrder[] = {
        "signal", "limits", "spindle", "motion", "status", "per_axis", "extension"
    };
    static const char* kCategoryNames[] = {
        "Signal Configuration", "Limits & Homing", "Spindle",
        "Motion Parameters", "Status Report", "Per-Axis Parameters", "Extension Settings"
    };

    std::map<std::string, std::vector<const UnifiedSetting*>> groups;
    for (const auto& [key, s] : m_settings) {
        if (!s.value.empty()) {
            groups[s.category].push_back(&s);
        }
    }

    std::vector<std::pair<std::string, std::vector<const UnifiedSetting*>>> result;
    for (int i = 0; i < 7; ++i) {
        auto it = groups.find(kCategoryOrder[i]);
        if (it != groups.end() && !it->second.empty()) {
            result.push_back({kCategoryNames[i], std::move(it->second)});
        }
    }
    return result;
}

std::vector<const UnifiedSetting*> UnifiedSettingsMap::getVisible(FirmwareType fw) const {
    std::vector<const UnifiedSetting*> result;
    for (const auto& [key, s] : m_settings) {
        if (s.value.empty()) continue;

        bool supported = false;
        if (fw == FirmwareType::FluidNC) {
            supported = !s.fluidncPath.empty() || s.category == "extension";
        } else {
            supported = s.grblId >= 0 || s.category == "extension";
        }
        if (supported) {
            result.push_back(&s);
        }
    }
    return result;
}

bool UnifiedSettingsMap::hasModified() const {
    for (const auto& [key, s] : m_settings) {
        if (s.modified) return true;
    }
    return false;
}

std::vector<const UnifiedSetting*> UnifiedSettingsMap::getModified() const {
    std::vector<const UnifiedSetting*> result;
    for (const auto& [key, s] : m_settings) {
        if (s.modified) result.push_back(&s);
    }
    return result;
}

void UnifiedSettingsMap::clear() {
    for (auto& [key, s] : m_settings) {
        s.value.clear();
        s.modified = false;
        s.changedFromDefault = false;
    }
    // Remove dynamic extension entries
    for (auto it = m_settings.begin(); it != m_settings.end();) {
        if (it->second.category == "extension" &&
            (it->first.substr(0, 5) == "grbl_" || it->first.substr(0, 4) == "fnc_")) {
            // Remove reverse lookups
            if (it->second.grblId >= 0) m_grblIdToKey.erase(it->second.grblId);
            if (!it->second.fluidncPath.empty()) m_fluidncPathToKey.erase(it->second.fluidncPath);
            it = m_settings.erase(it);
        } else {
            ++it;
        }
    }
}

bool UnifiedSettingsMap::empty() const {
    for (const auto& [key, s] : m_settings) {
        if (!s.value.empty()) return false;
    }
    return true;
}

// --- JSON ---

nlohmann::json UnifiedSettingsMap::toJson() const {
    nlohmann::json j;
    nlohmann::json settings = nlohmann::json::array();
    for (const auto& [key, s] : m_settings) {
        if (s.value.empty()) continue;
        nlohmann::json entry;
        entry["key"] = s.key;
        entry["value"] = s.value;
        if (s.grblId >= 0) entry["grbl_id"] = s.grblId;
        if (!s.fluidncPath.empty()) entry["fluidnc_path"] = s.fluidncPath;
        settings.push_back(std::move(entry));
    }
    j["settings"] = settings;
    j["version"] = "2.0";
    return j;
}

bool UnifiedSettingsMap::fromJson(const nlohmann::json& j) {
    if (!j.contains("settings") || !j["settings"].is_array())
        return false;

    // Handle v1.0 format (GrblSettings JSON with numeric IDs)
    std::string version = j.value("version", "1.0");
    if (version == "1.0") {
        for (const auto& item : j["settings"]) {
            if (!item.contains("id") || !item.contains("value")) continue;
            int id = item["id"].get<int>();
            float val = item["value"].get<float>();
            char buf[32];
            std::snprintf(buf, sizeof(buf), "$%d=%.3f", id, static_cast<double>(val));
            parseGrblLine(buf);
        }
        return true;
    }

    // v2.0 format
    for (const auto& item : j["settings"]) {
        if (!item.contains("key") || !item.contains("value")) continue;
        std::string key = item["key"].get<std::string>();
        std::string value = item["value"].get<std::string>();

        auto it = m_settings.find(key);
        if (it != m_settings.end()) {
            it->second.value = value;
            it->second.modified = false;
        } else {
            // Reconstruct extension setting
            UnifiedSetting s;
            s.key = key;
            s.displayName = key;
            s.category = "extension";
            s.value = value;
            if (item.contains("grbl_id")) {
                s.grblId = item["grbl_id"].get<int>();
                m_grblIdToKey[s.grblId] = key;
            }
            if (item.contains("fluidnc_path")) {
                s.fluidncPath = item["fluidnc_path"].get<std::string>();
                m_fluidncPathToKey[s.fluidncPath] = key;
            }
            m_settings[key] = std::move(s);
        }
    }
    return true;
}

std::string UnifiedSettingsMap::toJsonString() const {
    return toJson().dump(2);
}

bool UnifiedSettingsMap::fromJsonString(const std::string& jsonStr) {
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) return false;
    return fromJson(j);
}

// --- Diff ---

std::vector<UnifiedSettingsMap::DiffEntry>
UnifiedSettingsMap::diff(const UnifiedSettingsMap& other) const {
    std::vector<DiffEntry> result;
    for (const auto& [key, s] : m_settings) {
        if (s.value.empty()) continue;
        const auto* otherS = other.get(key);
        if (otherS && !otherS->value.empty() && otherS->value != s.value) {
            result.push_back({key, s.displayName, s.value, otherS->value});
        }
    }
    // Settings in other that aren't in this
    for (const auto& [key, s] : other.m_settings) {
        if (s.value.empty()) continue;
        const auto* thisS = get(key);
        if (!thisS || thisS->value.empty()) {
            result.push_back({key, s.displayName, "", s.value});
        }
    }
    return result;
}

// --- Plain text export ---

std::string UnifiedSettingsMap::exportPlainText(FirmwareType fw) const {
    std::string content;
    if (fw == FirmwareType::FluidNC) {
        content += "; FluidNC Settings Export\n";
    } else {
        content += "; GRBL Settings Export\n";
    }
    content += "; Generated by Digital Workshop\n;\n";

    for (const auto& [key, s] : m_settings) {
        if (s.value.empty()) continue;

        if (fw == FirmwareType::FluidNC && !s.fluidncPath.empty()) {
            content += "$" + s.fluidncPath + "=" + s.value;
        } else if (s.grblId >= 0) {
            content += "$" + std::to_string(s.grblId) + "=" + s.value;
        } else {
            continue;
        }

        content += " ; " + s.displayName + "\n";
    }
    return content;
}

} // namespace dw
