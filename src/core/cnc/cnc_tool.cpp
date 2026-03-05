#include "cnc_tool.h"

#include <cstdio>

namespace dw {

namespace {

const char* toolTypeName(VtdbToolType t) {
    switch (t) {
    case VtdbToolType::BallNose:        return "Ball Nose";
    case VtdbToolType::EndMill:         return "End Mill";
    case VtdbToolType::Radiused:        return "Radiused";
    case VtdbToolType::VBit:            return "V-Bit";
    case VtdbToolType::TaperedBallNose: return "Tapered Ball Nose";
    case VtdbToolType::Drill:           return "Drill";
    case VtdbToolType::ThreadMill:      return "Thread Mill";
    case VtdbToolType::FormTool:        return "Form Tool";
    case VtdbToolType::DiamondDrag:     return "Diamond Drag";
    default:                            return "Tool";
    }
}

// Format a double value with an optional format specifier.
// "|.0" → 0 decimal places, "|.1" → 1, "|.2" → 2, "|F" or empty → %g
std::string formatValue(f64 val, const std::string& spec) {
    char buf[64];
    if (spec.empty() || spec == "F") {
        std::snprintf(buf, sizeof(buf), "%g", val);
    } else if (spec == ".0") {
        std::snprintf(buf, sizeof(buf), "%.0f", val);
    } else if (spec == ".1") {
        std::snprintf(buf, sizeof(buf), "%.1f", val);
    } else if (spec == ".2") {
        std::snprintf(buf, sizeof(buf), "%.2f", val);
    } else if (spec == ".3") {
        std::snprintf(buf, sizeof(buf), "%.3f", val);
    } else {
        std::snprintf(buf, sizeof(buf), "%g", val);
    }
    return buf;
}

} // namespace

std::string resolveToolNameFormat(const VtdbToolGeometry& g) {
    if (g.name_format.empty()) {
        // Fallback: generate a simple name
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s %gmm %d-flute",
                      toolTypeName(g.tool_type), g.diameter, g.num_flutes);
        return buf;
    }

    std::string result;
    result.reserve(g.name_format.size());
    size_t i = 0;

    while (i < g.name_format.size()) {
        if (g.name_format[i] == '{') {
            size_t close = g.name_format.find('}', i + 1);
            if (close == std::string::npos) {
                result += g.name_format[i];
                ++i;
                continue;
            }

            std::string token = g.name_format.substr(i + 1, close - i - 1);
            i = close + 1;

            // Split token into name and optional format spec (e.g. "Diameter|F")
            std::string name = token;
            std::string spec;
            auto pipe = token.find('|');
            if (pipe != std::string::npos) {
                name = token.substr(0, pipe);
                spec = token.substr(pipe + 1);
            }

            if (name == "Tool Type") {
                result += toolTypeName(g.tool_type);
            } else if (name == "Diameter") {
                result += formatValue(g.diameter, spec);
            } else if (name == "Included Angle") {
                result += formatValue(g.included_angle, spec);
            } else if (name == "Side Angle") {
                // Tapered tools: side angle = (included_angle) for display
                result += formatValue(g.included_angle, spec);
            } else if (name == "Tip Radius") {
                result += formatValue(g.tip_radius, spec);
            } else if (name == "Flat Diameter") {
                result += formatValue(g.flat_diameter, spec);
            } else if (name == "Flutes") {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%d", g.num_flutes);
                result += buf;
            } else if (name == "Units Short") {
                result += (g.units == VtdbUnits::Metric) ? "mm" : "in";
            } else {
                // Unrecognized token — leave as-is for debugging
                result += '{';
                result += token;
                result += '}';
            }
        } else {
            result += g.name_format[i];
            ++i;
        }
    }

    return result;
}

} // namespace dw
