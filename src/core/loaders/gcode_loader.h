#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../gcode/gcode_analyzer.h"
#include "../gcode/gcode_parser.h"
#include "../gcode/gcode_types.h"
#include "loader.h"

namespace dw {

// Metadata extracted from G-code file
struct GCodeMetadata {
    Vec3 boundsMin;
    Vec3 boundsMax;
    f32 totalDistance = 0.0f;
    f32 estimatedTime = 0.0f;
    std::vector<f32> feedRates;   // Unique feed rates found
    std::vector<int> toolNumbers; // Unique tool numbers found
};

// G-code loader - converts toolpath to mesh geometry
class GCodeLoader : public MeshLoader {
  public:
    LoadResult load(const Path& path) override;
    LoadResult loadFromBuffer(const ByteBuffer& data) override;
    bool supports(const std::string& extension) const override;
    std::vector<std::string> extensions() const override;

    // Access metadata from last successful load
    const GCodeMetadata& lastMetadata() const { return m_lastMetadata; }

  private:
    LoadResult processProgram(const gcode::Program& program);
    MeshPtr toolpathToMesh(const std::vector<gcode::PathSegment>& path);
    GCodeMetadata extractMetadata(const gcode::Program& program, const gcode::Statistics& stats);

    GCodeMetadata m_lastMetadata;
};

} // namespace dw
