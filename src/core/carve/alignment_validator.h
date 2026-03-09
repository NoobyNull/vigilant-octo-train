#pragma once

#include "../gcode/gcode_types.h"
#include "../mesh/mesh.h"
#include "../types.h"
#include "model_fitter.h"

namespace dw {
namespace carve {

struct AlignmentResult {
    bool valid = false;      // Enough data to validate
    bool aligned = false;    // Points are near the mesh surface
    int pointsTested = 0;    // Number of G-code points sampled
    int pointsNear = 0;      // Number that are near the mesh surface
    f32 nearRatio = 0.0f;    // pointsNear / pointsTested
};

// Validate alignment between G-code toolpath and transformed mesh.
// Samples approximately samplePercent of cutting segments from the G-code,
// transforms mesh vertices using ModelFitter + FitParams, and checks
// whether sampled G-code points lie within toleranceMm of the mesh surface.
//
// Works entirely in G-code space (Z-up). Does NOT apply Y<->Z swap.
AlignmentResult validateAlignment(
    const gcode::Program& program,
    const Mesh& mesh,
    const ModelFitter& fitter,
    const FitParams& params,
    f32 samplePercent = 1.0f,     // % of cutting segments to test
    f32 toleranceMm = 1.0f);       // proximity threshold in mm

} // namespace carve
} // namespace dw
