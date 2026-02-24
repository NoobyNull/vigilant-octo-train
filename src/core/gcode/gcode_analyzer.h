#pragma once

#include "gcode_types.h"
#include "machine_profile.h"

namespace dw {
namespace gcode {

class Analyzer {
  public:
    Analyzer() = default;

    // Analyze a parsed program
    Statistics analyze(const Program& program);

    // Set machine profile for trapezoidal motion planning
    void setMachineProfile(const MachineProfile& profile) { m_profile = profile; }

    // Legacy setters — write into the embedded profile
    void setDefaultRapidRate(f32 rate) { m_profile.rapidRate = rate; }
    void setDefaultFeedRate(f32 rate) { m_profile.defaultFeedRate = rate; }

  private:
    f32 calculateSegmentLength(const PathSegment& segment);
    f32 calculateSegmentTime(const PathSegment& segment);

    // Per-axis velocity limiting: scale commanded rate so no axis exceeds its max
    f32 effectiveFeedRate(f32 commandedRate, const Vec3& direction) const;

    // Per-axis acceleration limiting
    f32 effectiveAccel(const Vec3& direction) const;

    MachineProfile m_profile;
};

} // namespace gcode
} // namespace dw
