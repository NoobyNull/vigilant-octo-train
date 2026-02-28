#include "layout_preset.h"

#include <nlohmann/json.hpp>

namespace dw {

std::string LayoutPreset::toJsonString() const {
    nlohmann::json visObj;
    for (const auto& [key, val] : visibility) {
        visObj[key] = val;
    }

    nlohmann::json j{
        {"name", name},
        {"builtIn", builtIn},
        {"visibility", visObj},
        {"autoTrigger", autoTriggerPanelKey},
    };
    return j.dump();
}

LayoutPreset LayoutPreset::fromJsonString(const std::string& jsonStr) {
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded())
        return LayoutPreset{};

    LayoutPreset p;
    if (j.contains("name")) p.name = j["name"].get<std::string>();
    if (j.contains("builtIn")) p.builtIn = j["builtIn"].get<bool>();
    if (j.contains("autoTrigger")) p.autoTriggerPanelKey = j["autoTrigger"].get<std::string>();

    if (j.contains("visibility") && j["visibility"].is_object()) {
        for (auto& [key, val] : j["visibility"].items()) {
            if (val.is_boolean()) {
                p.visibility[key] = val.get<bool>();
            }
        }
    }
    return p;
}

// --- Built-in preset factories ---

LayoutPreset LayoutPreset::modelDefault() {
    LayoutPreset p;
    p.name = "Modeling";
    p.builtIn = true;
    p.visibility = {
        {"viewport", true},       {"library", true},
        {"properties", true},     {"project", true},
        {"start_page", true},     {"gcode", false},
        {"cut_optimizer", false}, {"cost_estimator", false},
        {"materials", false},     {"tool_browser", false},
        {"cnc_status", false},    {"cnc_jog", false},
        {"cnc_console", false},   {"cnc_wcs", false},
        {"cnc_tool", false},      {"cnc_job", false},
        {"cnc_safety", false},    {"cnc_settings", false},
        {"cnc_macros", false},
    };
    return p;
}

LayoutPreset LayoutPreset::cncDefault() {
    LayoutPreset p;
    p.name = "CNC Sender";
    p.builtIn = true;
    p.autoTriggerPanelKey = "cnc_status";
    p.visibility = {
        {"viewport", true},       {"library", false},
        {"properties", false},    {"project", true},
        {"start_page", false},    {"gcode", true},
        {"cut_optimizer", false}, {"cost_estimator", false},
        {"materials", false},     {"tool_browser", false},
        {"cnc_status", true},     {"cnc_jog", true},
        {"cnc_console", true},    {"cnc_wcs", true},
        {"cnc_tool", true},       {"cnc_job", true},
        {"cnc_safety", true},     {"cnc_settings", true},
        {"cnc_macros", true},
    };
    return p;
}

} // namespace dw
