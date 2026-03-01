#pragma once

#include "../types.h"
#include "heightmap.h"
#include "island_detector.h"
#include "surface_analysis.h"

#include <vector>

namespace dw {
namespace carve {

// Generate colored overlay texture data for heightmap preview.
// Returns RGBA pixel data (width x height x 4 bytes).
std::vector<u8> generateAnalysisOverlay(
    const Heightmap& heightmap,
    const IslandResult& islands,
    const CurvatureResult& curvature,
    int width, int height);

} // namespace carve
} // namespace dw
