#pragma once

#include "gcode_types.h"

namespace dw {
namespace gcode {

class Analyzer {
public:
    Analyzer() = default;

    // Analyze a parsed program
    Statistics analyze(const Program& program);

    // Set default feed rates for estimation
    void setDefaultRapidRate(f32 rate) { m_defaultRapidRate = rate; }
    void setDefaultFeedRate(f32 rate) { m_defaultFeedRate = rate; }

private:
    f32 calculateSegmentLength(const PathSegment& segment);
    f32 calculateSegmentTime(const PathSegment& segment);

    f32 m_defaultRapidRate = 5000.0f;  // mm/min
    f32 m_defaultFeedRate = 1000.0f;   // mm/min
};

}  // namespace gcode
}  // namespace dw
