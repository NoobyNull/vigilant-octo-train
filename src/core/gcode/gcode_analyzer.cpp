#include "gcode_analyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dw {
namespace gcode {

Statistics Analyzer::analyze(const Program& program) {
    Statistics stats;

    stats.lineCount = static_cast<int>(program.commands.size());
    stats.commandCount = 0;

    // Initialize bounds
    bool boundsInitialized = false;

    for (const auto& cmd : program.commands) {
        if (cmd.type != CommandType::Unknown) {
            stats.commandCount++;
        }

        if (cmd.type == CommandType::M6) {
            stats.toolChangeCount++;
        }
    }

    // Reserve segment times
    stats.segmentTimes.reserve(program.path.size());

    // Analyze path segments
    for (const auto& segment : program.path) {
        f32 length = calculateSegmentLength(segment);

        stats.totalPathLength += length;

        if (segment.isRapid) {
            stats.rapidPathLength += length;
        } else {
            stats.cuttingPathLength += length;
        }

        f32 segTime = calculateSegmentTime(segment);
        stats.segmentTimes.push_back(segTime);
        stats.estimatedTime += segTime;

        // Update bounds
        if (!boundsInitialized) {
            stats.boundsMin = segment.start;
            stats.boundsMax = segment.start;
            boundsInitialized = true;
        }

        stats.boundsMin.x = std::min({stats.boundsMin.x, segment.start.x, segment.end.x});
        stats.boundsMin.y = std::min({stats.boundsMin.y, segment.start.y, segment.end.y});
        stats.boundsMin.z = std::min({stats.boundsMin.z, segment.start.z, segment.end.z});
        stats.boundsMax.x = std::max({stats.boundsMax.x, segment.start.x, segment.end.x});
        stats.boundsMax.y = std::max({stats.boundsMax.y, segment.start.y, segment.end.y});
        stats.boundsMax.z = std::max({stats.boundsMax.z, segment.start.z, segment.end.z});
    }

    return stats;
}

f32 Analyzer::calculateSegmentLength(const PathSegment& segment) {
    Vec3 delta = segment.end - segment.start;
    return std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
}

f32 Analyzer::effectiveFeedRate(f32 commandedRate, const Vec3& direction) const {
    // Scale commanded rate so no single axis exceeds its max feed rate.
    // direction is normalized. For each active axis:
    //   axisRate = commandedRate * |d_axis|   must be <= maxFeedRateAxis
    //   => commandedRate <= maxFeedRateAxis / |d_axis|
    f32 scale = 1.0f;
    constexpr f32 kEps = 1e-6f;

    if (std::fabs(direction.x) > kEps) {
        scale = std::min(scale, m_profile.maxFeedRateX / (commandedRate * std::fabs(direction.x)));
    }
    if (std::fabs(direction.y) > kEps) {
        scale = std::min(scale, m_profile.maxFeedRateY / (commandedRate * std::fabs(direction.y)));
    }
    if (std::fabs(direction.z) > kEps) {
        scale = std::min(scale, m_profile.maxFeedRateZ / (commandedRate * std::fabs(direction.z)));
    }

    return commandedRate * std::min(scale, 1.0f);
}

f32 Analyzer::effectiveAccel(const Vec3& direction) const {
    // Limit acceleration so no axis exceeds its max.
    // For combined-axis accel 'a' along direction d (normalized):
    //   a * |d_axis| <= accelAxis  =>  a <= accelAxis / |d_axis|
    // Take the minimum across active axes.
    f32 accel = std::numeric_limits<f32>::max();
    constexpr f32 kEps = 1e-6f;

    if (std::fabs(direction.x) > kEps) {
        accel = std::min(accel, m_profile.accelX / std::fabs(direction.x));
    }
    if (std::fabs(direction.y) > kEps) {
        accel = std::min(accel, m_profile.accelY / std::fabs(direction.y));
    }
    if (std::fabs(direction.z) > kEps) {
        accel = std::min(accel, m_profile.accelZ / std::fabs(direction.z));
    }

    // Fallback if somehow no axis is active (shouldn't happen for non-zero moves)
    if (accel == std::numeric_limits<f32>::max()) {
        accel = m_profile.accelX;
    }

    return accel;
}

f32 Analyzer::calculateSegmentTime(const PathSegment& segment) {
    f32 length = calculateSegmentLength(segment);
    if (length < 1e-6f)
        return 0.0f;

    // Determine commanded feed rate (mm/min)
    f32 commandedRate;
    if (segment.isRapid) {
        commandedRate = m_profile.rapidRate;
    } else if (segment.feedRate > 0.0f) {
        commandedRate = segment.feedRate;
    } else {
        commandedRate = m_profile.defaultFeedRate;
    }

    if (commandedRate <= 0.0f)
        return 0.0f;

    // Direction vector (normalized)
    Vec3 delta = segment.end - segment.start;
    Vec3 dir = delta / length;

    // Per-axis velocity limiting (mm/min)
    f32 vMaxMmMin = effectiveFeedRate(commandedRate, dir);
    // Convert to mm/s for trapezoidal calculation
    f32 vMax = vMaxMmMin / 60.0f;

    // Per-axis acceleration limiting (mm/s^2)
    f32 accel = effectiveAccel(dir);

    // Trapezoidal profile with v_entry = v_exit = 0 (conservative, no lookahead).
    // Distance needed to accelerate to vMax and decelerate back to 0:
    //   d_full = vMax^2 / accel
    f32 dFull = (vMax * vMax) / accel;

    f32 timeSeconds;
    if (length >= dFull) {
        // Full trapezoid: accel phase + cruise + decel phase
        // t_accel = t_decel = vMax / accel
        // t_cruise = (length - d_full) / vMax
        timeSeconds = 2.0f * (vMax / accel) + (length - dFull) / vMax;
    } else {
        // Triangle profile: never reaches vMax
        // t = 2 * sqrt(length / accel)
        timeSeconds = 2.0f * std::sqrt(length / accel);
    }

    // Return time in minutes to match estimatedTime units
    return timeSeconds / 60.0f;
}

} // namespace gcode
} // namespace dw
