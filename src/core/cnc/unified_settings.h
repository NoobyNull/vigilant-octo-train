#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dw {

enum class FirmwareType { GRBL, GrblHAL, FluidNC };

// How to render/validate the setting value
enum class SettingType { Float, Bool, Bitmask, Enum };

// A firmware-agnostic setting definition that maps to firmware-specific wire formats
struct UnifiedSetting {
    std::string key;          // "steps_per_mm_x", "max_feed_x", etc.
    std::string displayName;  // "Steps per mm (X)"
    std::string category;     // "signal", "limits", "spindle", "motion", "status", "per_axis", "extension"
    std::string units;        // "steps/mm", "mm/min", etc.
    SettingType type = SettingType::Float;

    // Firmware-specific identifiers (empty/negative = not supported)
    int grblId = -1;                // $100
    std::string fluidncPath;        // "axes/x/steps_per_mm"

    // Validation
    float min = 0.0f;
    float max = 1e9f;

    // Current value (string to support both numeric and path-based values)
    std::string value;
    bool modified = false;
    bool changedFromDefault = false; // FluidNC $SC tracking
};

// Manages a collection of unified settings with firmware translation
class UnifiedSettingsMap {
  public:
    UnifiedSettingsMap();

    // Parse firmware responses
    bool parseGrblLine(const std::string& line);      // "$N=value" format
    bool parseFluidNCLine(const std::string& line);    // "$path=value" from $S
    void markChangedFromDefault(const std::string& line); // $SC line

    // Build a set command for the given firmware
    std::string buildSetCommand(const std::string& key, const std::string& value,
                                FirmwareType fw) const;

    // Access
    const std::map<std::string, UnifiedSetting>& getAll() const { return m_settings; }
    const UnifiedSetting* get(const std::string& key) const;
    bool set(const std::string& key, const std::string& value);

    // Get settings grouped by category
    std::vector<std::pair<std::string, std::vector<const UnifiedSetting*>>>
    getByCategory() const;

    // Filter to settings supported by the given firmware
    std::vector<const UnifiedSetting*> getVisible(FirmwareType fw) const;

    // Check if any settings have been modified
    bool hasModified() const;

    // Get all modified settings
    std::vector<const UnifiedSetting*> getModified() const;

    // Clear all settings values (keeps definitions)
    void clear();

    // Check if any settings have values loaded
    bool empty() const;

    // JSON serialization (for backup/restore)
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& j);
    std::string toJsonString() const;
    bool fromJsonString(const std::string& jsonStr);

    // Diff against another settings map (for restore preview)
    struct DiffEntry {
        std::string key;
        std::string displayName;
        std::string currentValue;
        std::string backupValue;
    };
    std::vector<DiffEntry> diff(const UnifiedSettingsMap& other) const;

    // Export as plain text (firmware-specific format)
    std::string exportPlainText(FirmwareType fw) const;

    // Lookup helpers
    const UnifiedSetting* getByGrblId(int id) const;
    const UnifiedSetting* getByFluidNCPath(const std::string& path) const;

  private:
    void initDefinitions();

    std::map<std::string, UnifiedSetting> m_settings;

    // Reverse lookup indices
    std::map<int, std::string> m_grblIdToKey;
    std::map<std::string, std::string> m_fluidncPathToKey;
};

} // namespace dw
