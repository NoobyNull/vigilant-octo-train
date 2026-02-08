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

    // Analyze path segments
    for (const auto& segment : program.path) {
        f32 length = calculateSegmentLength(segment);

        stats.totalPathLength += length;

        if (segment.isRapid) {
            stats.rapidPathLength += length;
        } else {
            stats.cuttingPathLength += length;
        }

        stats.estimatedTime += calculateSegmentTime(segment);

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

f32 Analyzer::calculateSegmentTime(const PathSegment& segment) {
    f32 length = calculateSegmentLength(segment);

    f32 feedRate;
    if (segment.isRapid) {
        feedRate = m_defaultRapidRate;
    } else if (segment.feedRate > 0.0f) {
        feedRate = segment.feedRate;
    } else {
        feedRate = m_defaultFeedRate;
    }

    // Time = distance / speed (in minutes since feed rate is mm/min)
    if (feedRate > 0.0f) {
        return length / feedRate;
    }

    return 0.0f;
}

} // namespace gcode
} // namespace dw
