#pragma once

#include <string>
#include <unordered_map>

namespace dw {

// Named layout preset controlling panel visibility.
// Persisted as JSON strings in config.ini (same pattern as MachineProfile).
struct LayoutPreset {
    std::string name;
    bool builtIn = false; // Prevents deletion

    // Panel visibility: key -> visible (19 keys matching UIManager m_show* flags)
    std::unordered_map<std::string, bool> visibility;

    // Auto-context: focusing a panel with this key activates this preset.
    // Empty means no auto-trigger.
    std::string autoTriggerPanelKey;

    // JSON serialization (follows MachineProfile pattern)
    std::string toJsonString() const;
    static LayoutPreset fromJsonString(const std::string& jsonStr);

    // Built-in preset factories
    static LayoutPreset modelDefault();
    static LayoutPreset cncDefault();
};

// All valid panel keys (for validation and iteration)
inline constexpr const char* PANEL_KEYS[] = {
    "viewport",      "library",     "properties",   "project",
    "gcode",         "cut_optimizer", "cost_estimator", "materials",
    "tool_browser",  "start_page",
    "cnc_status",    "cnc_jog",     "cnc_console",  "cnc_wcs",
    "cnc_tool",      "cnc_job",     "cnc_safety",   "cnc_settings",
    "cnc_macros",
};
inline constexpr int PANEL_KEY_COUNT = 19;

} // namespace dw
