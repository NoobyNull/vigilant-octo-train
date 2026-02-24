#pragma once

#include <string>

#include "../types.h"

namespace dw {
namespace gcode {

// CNC machine kinematic parameters for accurate motion planning.
// Values map to GRBL $ settings where noted.
struct MachineProfile {
    std::string name = "Default";

    // Per-axis max feed rates (mm/min) — GRBL $110/$111/$112
    f32 maxFeedRateX = 5000.0f;
    f32 maxFeedRateY = 5000.0f;
    f32 maxFeedRateZ = 3000.0f;

    // Per-axis acceleration (mm/s^2) — GRBL $120/$121/$122
    f32 accelX = 200.0f;
    f32 accelY = 200.0f;
    f32 accelZ = 100.0f;

    // Max travel (mm) — GRBL $130/$131/$132
    f32 maxTravelX = 430.0f;
    f32 maxTravelY = 430.0f;
    f32 maxTravelZ = 100.0f;

    // Junction deviation (mm) — GRBL $11 (reserved for future lookahead)
    f32 junctionDeviation = 0.01f;

    // Default rates (mm/min)
    f32 rapidRate = 5000.0f;
    f32 defaultFeedRate = 1000.0f;

    // True for the 3 built-in presets (prevents deletion)
    bool builtIn = false;

    // JSON serialization (returns/accepts JSON string)
    std::string toJsonString() const;
    static MachineProfile fromJsonString(const std::string& jsonStr);

    // Named preset factories
    static MachineProfile shapeoko4();
    static MachineProfile longmillMK2();
    static MachineProfile defaultProfile();
};

} // namespace gcode
} // namespace dw
