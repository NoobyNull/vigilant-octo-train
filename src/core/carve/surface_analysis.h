#pragma once

#include "../types.h"
#include "heightmap.h"

namespace dw {
namespace carve {

struct CurvatureResult {
    f32 minConcaveRadius = 0.0f;  // Smallest concave radius found (mm)
    int minRadiusCol = 0;          // Grid position of minimum
    int minRadiusRow = 0;
    f32 avgConcaveRadius = 0.0f;  // Average concave radius
    int concavePointCount = 0;     // Number of concave grid cells
};

// Analyze heightmap curvature to find minimum concave feature radius.
// Uses discrete Laplacian on the heightmap grid.
// Only considers cells where curvature indicates concavity (valleys/grooves).
CurvatureResult analyzeCurvature(const Heightmap& heightmap);

// Compute local radius of curvature at a grid cell.
// Returns positive for concave, negative for convex, 0 for flat.
f32 computeLocalRadius(const Heightmap& heightmap, int col, int row);

} // namespace carve
} // namespace dw
