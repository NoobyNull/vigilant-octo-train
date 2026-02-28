#pragma once

#include "../types.h"

#include <string>
#include <vector>

namespace dw {
namespace carve {

enum class ScanAxis {
    XOnly,     // Parallel lines along X
    YOnly,     // Parallel lines along Y
    XThenY,    // Two passes: X first, then Y
    YThenX     // Two passes: Y first, then X
};

enum class MillDirection {
    Climb,          // All lines in same direction
    Conventional,   // All lines in opposite direction
    Alternating     // Bidirectional (zigzag)
};

enum class StepoverPreset {
    UltraFine,  // 1% of tip diameter
    Fine,       // 8%
    Basic,      // 12%
    Rough,      // 25%
    Roughing    // 40%
};

struct ToolpathConfig {
    ScanAxis axis = ScanAxis::XOnly;
    MillDirection direction = MillDirection::Alternating;
    StepoverPreset stepoverPreset = StepoverPreset::Basic;
    f32 customStepoverPct = 0.0f;  // If non-zero, overrides preset
    f32 safeZMm = 5.0f;
    f32 feedRateMmMin = 1000.0f;
    f32 plungeRateMmMin = 300.0f;
};

// Single toolpath move
struct ToolpathPoint {
    Vec3 position;
    bool rapid = false;  // G0 (true) vs G1 (false)
};

// Complete toolpath
struct Toolpath {
    std::vector<ToolpathPoint> points;
    f32 totalDistanceMm = 0.0f;
    f32 estimatedTimeSec = 0.0f;
    int lineCount = 0;  // Number of G-code lines this will produce
    std::vector<std::string> warnings;  // Travel limit violations
};

// Convert preset to percentage
f32 stepoverPercent(StepoverPreset preset);

} // namespace carve
} // namespace dw
