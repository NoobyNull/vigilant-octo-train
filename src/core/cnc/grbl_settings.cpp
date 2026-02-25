#include "grbl_settings.h"

#include <cmath>
#include <sstream>

namespace dw {

// --- Group utilities ---

const char* grblSettingGroupName(GrblSettingGroup group) {
    switch (group) {
    case GrblSettingGroup::General: return "General";
    case GrblSettingGroup::Motion: return "Motion";
    case GrblSettingGroup::Limits: return "Limits & Homing";
    case GrblSettingGroup::Spindle: return "Spindle";
    case GrblSettingGroup::StepsPerMm: return "Steps/mm";
    case GrblSettingGroup::FeedRates: return "Feed Rates";
    case GrblSettingGroup::Acceleration: return "Acceleration";
    case GrblSettingGroup::MaxTravel: return "Max Travel";
    case GrblSettingGroup::Unknown: return "Other";
    default: return "Unknown";
    }
}

GrblSettingGroup grblSettingGroup(int id) {
    if (id >= 0 && id <= 6) return GrblSettingGroup::General;
    if (id >= 10 && id <= 13) return GrblSettingGroup::Motion;
    if (id >= 20 && id <= 27) return GrblSettingGroup::Limits;
    if (id >= 30 && id <= 32) return GrblSettingGroup::Spindle;
    if (id >= 100 && id <= 102) return GrblSettingGroup::StepsPerMm;
    if (id >= 110 && id <= 112) return GrblSettingGroup::FeedRates;
    if (id >= 120 && id <= 122) return GrblSettingGroup::Acceleration;
    if (id >= 130 && id <= 132) return GrblSettingGroup::MaxTravel;
    return GrblSettingGroup::Unknown;
}

// --- Static metadata table ---

const std::map<int, GrblSettingMeta>& GrblSettings::metadata() {
    static const std::map<int, GrblSettingMeta> meta = {
        {0,   {0,   "Step pulse time",          "microseconds", 3,     255,    false, false}},
        {1,   {1,   "Step idle delay",           "ms",          0,     255,    false, false}},
        {2,   {2,   "Step port invert mask",     "",            0,     7,      true,  false}},
        {3,   {3,   "Direction port invert mask", "",           0,     7,      true,  false}},
        {4,   {4,   "Step enable invert",        "",            0,     1,      false, true}},
        {5,   {5,   "Limit pins invert",         "",            0,     1,      false, true}},
        {6,   {6,   "Probe pin invert",          "",            0,     1,      false, true}},
        {10,  {10,  "Status report options",     "",            0,     3,      true,  false}},
        {11,  {11,  "Junction deviation",        "mm",          0.001f, 1.0f,  false, false}},
        {12,  {12,  "Arc tolerance",             "mm",          0.001f, 1.0f,  false, false}},
        {13,  {13,  "Report in inches",          "",            0,     1,      false, true}},
        {20,  {20,  "Soft limits enable",        "",            0,     1,      false, true}},
        {21,  {21,  "Hard limits enable",        "",            0,     1,      false, true}},
        {22,  {22,  "Homing cycle enable",       "",            0,     1,      false, true}},
        {23,  {23,  "Homing direction invert mask", "",         0,     7,      true,  false}},
        {24,  {24,  "Homing locate feed rate",   "mm/min",      1,     10000,  false, false}},
        {25,  {25,  "Homing search seek rate",   "mm/min",      1,     10000,  false, false}},
        {26,  {26,  "Homing switch debounce",    "ms",          0,     1000,   false, false}},
        {27,  {27,  "Homing switch pull-off",    "mm",          0,     100,    false, false}},
        {30,  {30,  "Max spindle speed",         "RPM",         0,     100000, false, false}},
        {31,  {31,  "Min spindle speed",         "RPM",         0,     100000, false, false}},
        {32,  {32,  "Laser mode enable",         "",            0,     1,      false, true}},
        {100, {100, "X-axis steps per mm",       "steps/mm",    1,     10000,  false, false}},
        {101, {101, "Y-axis steps per mm",       "steps/mm",    1,     10000,  false, false}},
        {102, {102, "Z-axis steps per mm",       "steps/mm",    1,     10000,  false, false}},
        {110, {110, "X-axis max rate",           "mm/min",      1,     100000, false, false}},
        {111, {111, "Y-axis max rate",           "mm/min",      1,     100000, false, false}},
        {112, {112, "Z-axis max rate",           "mm/min",      1,     100000, false, false}},
        {120, {120, "X-axis acceleration",       "mm/s^2",      1,     10000,  false, false}},
        {121, {121, "Y-axis acceleration",       "mm/s^2",      1,     10000,  false, false}},
        {122, {122, "Z-axis acceleration",       "mm/s^2",      1,     10000,  false, false}},
        {130, {130, "X-axis max travel",         "mm",          1,     10000,  false, false}},
        {131, {131, "Y-axis max travel",         "mm",          1,     10000,  false, false}},
        {132, {132, "Z-axis max travel",         "mm",          1,     10000,  false, false}},
    };
    return meta;
}

// --- Constructor ---

GrblSettings::GrblSettings() = default;

// --- Parsing ---

bool GrblSettings::parseLine(const std::string& line) {
    // Expected format: "$N=V" where N is integer, V is float
    if (line.empty() || line[0] != '$') return false;

    auto eqPos = line.find('=');
    if (eqPos == std::string::npos || eqPos < 2) return false;

    // Parse setting ID
    int id = 0;
    try {
        id = std::stoi(line.substr(1, eqPos - 1));
    } catch (...) {
        return false;
    }

    // Parse value
    float value = 0.0f;
    try {
        value = std::stof(line.substr(eqPos + 1));
    } catch (...) {
        return false;
    }

    GrblSetting setting;
    setting.id = id;
    setting.value = value;
    applyMeta(setting);
    setting.modified = false;

    m_settings[id] = setting;
    return true;
}

int GrblSettings::parseSettingsResponse(const std::string& response) {
    int count = 0;
    std::istringstream stream(response);
    std::string line;
    while (std::getline(stream, line)) {
        // Trim \r if present
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (parseLine(line))
            ++count;
    }
    return count;
}

// --- Access ---

const GrblSetting* GrblSettings::get(int id) const {
    auto it = m_settings.find(id);
    if (it == m_settings.end()) return nullptr;
    return &it->second;
}

bool GrblSettings::set(int id, float value) {
    // Look up metadata for validation
    const auto& meta = metadata();
    auto metaIt = meta.find(id);
    if (metaIt != meta.end()) {
        const auto& m = metaIt->second;
        if (value < m.min || value > m.max) return false;
        if (m.isBoolean && value != 0.0f && value != 1.0f) return false;
        if (m.isBitmask && (value != std::floor(value) || value < 0)) return false;
    }

    auto it = m_settings.find(id);
    if (it != m_settings.end()) {
        it->second.value = value;
        it->second.modified = true;
    } else {
        // Create new setting entry
        GrblSetting setting;
        setting.id = id;
        setting.value = value;
        applyMeta(setting);
        setting.modified = true;
        m_settings[id] = setting;
    }
    return true;
}

// --- Grouped access ---

std::vector<std::pair<GrblSettingGroup, std::vector<const GrblSetting*>>>
GrblSettings::getGrouped() const {
    // Collect settings by group
    std::map<GrblSettingGroup, std::vector<const GrblSetting*>> groups;
    for (const auto& [id, setting] : m_settings) {
        auto group = grblSettingGroup(id);
        groups[group].push_back(&setting);
    }

    // Return in canonical group order
    static const GrblSettingGroup order[] = {
        GrblSettingGroup::General,
        GrblSettingGroup::Motion,
        GrblSettingGroup::Limits,
        GrblSettingGroup::Spindle,
        GrblSettingGroup::StepsPerMm,
        GrblSettingGroup::FeedRates,
        GrblSettingGroup::Acceleration,
        GrblSettingGroup::MaxTravel,
        GrblSettingGroup::Unknown,
    };

    std::vector<std::pair<GrblSettingGroup, std::vector<const GrblSetting*>>> result;
    for (auto g : order) {
        auto it = groups.find(g);
        if (it != groups.end() && !it->second.empty()) {
            result.push_back({g, std::move(it->second)});
        }
    }
    return result;
}

// --- JSON ---

nlohmann::json GrblSettings::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    nlohmann::json settings = nlohmann::json::array();
    for (const auto& [id, s] : m_settings) {
        settings.push_back({
            {"id", s.id},
            {"value", s.value},
        });
    }
    j["settings"] = settings;
    j["version"] = "1.0";
    return j;
}

bool GrblSettings::fromJson(const nlohmann::json& j) {
    if (!j.contains("settings") || !j["settings"].is_array())
        return false;

    m_settings.clear();
    for (const auto& item : j["settings"]) {
        if (!item.contains("id") || !item.contains("value"))
            continue;

        GrblSetting setting;
        setting.id = item["id"].get<int>();
        setting.value = item["value"].get<float>();
        applyMeta(setting);
        setting.modified = false;
        m_settings[setting.id] = setting;
    }
    return true;
}

std::string GrblSettings::toJsonString() const {
    return toJson().dump(2);
}

bool GrblSettings::fromJsonString(const std::string& jsonStr) {
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) return false;
    return fromJson(j);
}

// --- Diff ---

std::vector<std::pair<GrblSetting, GrblSetting>>
GrblSettings::diff(const GrblSettings& other) const {
    std::vector<std::pair<GrblSetting, GrblSetting>> diffs;
    for (const auto& [id, current] : m_settings) {
        const auto* otherSetting = other.get(id);
        if (otherSetting && otherSetting->value != current.value) {
            diffs.push_back({current, *otherSetting});
        }
    }
    // Also check for settings in other that are not in this
    for (const auto& [id, otherSetting] : other.m_settings) {
        if (m_settings.find(id) == m_settings.end()) {
            GrblSetting empty;
            empty.id = id;
            diffs.push_back({empty, otherSetting});
        }
    }
    return diffs;
}

// --- Command building ---

std::string GrblSettings::buildSetCommand(int id, float value) {
    // Format: "$N=V\n"
    char buf[64];
    // Use integer format if value is integral
    if (value == std::floor(value) && std::abs(value) < 1e6f) {
        std::snprintf(buf, sizeof(buf), "$%d=%d\n", id, static_cast<int>(value));
    } else {
        std::snprintf(buf, sizeof(buf), "$%d=%.3f\n", id, static_cast<double>(value));
    }
    return buf;
}

// --- Internal ---

void GrblSettings::applyMeta(GrblSetting& setting) const {
    const auto& meta = metadata();
    auto it = meta.find(setting.id);
    if (it != meta.end()) {
        const auto& m = it->second;
        setting.description = m.description;
        setting.units = m.units;
        setting.min = m.min;
        setting.max = m.max;
        setting.isBitmask = m.isBitmask;
        setting.isBoolean = m.isBoolean;
    } else {
        // Unknown/extension setting
        setting.description = "Unknown setting";
        setting.units = "";
        setting.min = -1e9f;
        setting.max = 1e9f;
        setting.isBitmask = false;
        setting.isBoolean = false;
    }
}

} // namespace dw
