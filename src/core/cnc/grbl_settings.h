#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dw {

// Metadata describing a single GRBL $ setting
struct GrblSettingMeta {
    int id = 0;
    std::string description;
    std::string units;
    float min = 0.0f;
    float max = 255.0f;
    bool isBitmask = false;
    bool isBoolean = false;
};

// A concrete GRBL setting with its current value
struct GrblSetting {
    int id = 0;
    float value = 0.0f;
    std::string description;
    std::string units;
    float min = 0.0f;
    float max = 255.0f;
    bool isBitmask = false;
    bool isBoolean = false;
    bool modified = false; // True if user edited but not yet written to GRBL
};

// Group of related settings for UI display
enum class GrblSettingGroup {
    General,     // $0-$6
    Motion,      // $10-$13
    Limits,      // $20-$27
    Spindle,     // $30-$32
    StepsPerMm,  // $100-$102
    FeedRates,   // $110-$112
    Acceleration, // $120-$122
    MaxTravel,   // $130-$132
    Unknown      // Extension settings not in standard GRBL
};

// Returns a human-readable group name
const char* grblSettingGroupName(GrblSettingGroup group);

// Returns the group for a given setting ID
GrblSettingGroup grblSettingGroup(int id);

// GRBL settings manager — parses $$ responses, validates, serializes to JSON
class GrblSettings {
  public:
    GrblSettings();

    // Parse a single "$N=V" line. Returns true if valid.
    bool parseLine(const std::string& line);

    // Parse a full $$ response (multiple lines separated by \n)
    int parseSettingsResponse(const std::string& response);

    // Access settings
    const std::map<int, GrblSetting>& getAll() const { return m_settings; }
    const GrblSetting* get(int id) const;

    // Set a value (with validation). Returns true if valid and accepted.
    bool set(int id, float value);

    // Get settings grouped for UI display
    std::vector<std::pair<GrblSettingGroup, std::vector<const GrblSetting*>>>
    getGrouped() const;

    // JSON serialization
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& j);

    // Convenience: serialize to/from string
    std::string toJsonString() const;
    bool fromJsonString(const std::string& jsonStr);

    // Clear all settings
    void clear() { m_settings.clear(); }

    // Check if settings have been loaded
    bool empty() const { return m_settings.empty(); }

    // Get settings that differ from another set (for diff display)
    std::vector<std::pair<GrblSetting, GrblSetting>>
    diff(const GrblSettings& other) const;

    // Build "$N=V\n" command string for a single setting
    static std::string buildSetCommand(int id, float value);

  private:
    // Apply metadata from the built-in table to a setting
    void applyMeta(GrblSetting& setting) const;

    std::map<int, GrblSetting> m_settings;

    // Static metadata table for standard GRBL settings
    static const std::map<int, GrblSettingMeta>& metadata();
};

} // namespace dw
