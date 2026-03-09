#include "alignment_validator.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>

namespace dw {
namespace carve {

namespace {

// Closest point on triangle (a,b,c) to point p.
// Standard Ericson/Eberly algorithm using Voronoi region checks.
Vec3 closestPointOnTriangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c)
{
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 ap = p - a;

    f32 d1 = glm::dot(ab, ap);
    f32 d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a; // Vertex A region

    Vec3 bp = p - b;
    f32 d3 = glm::dot(ab, bp);
    f32 d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b; // Vertex B region

    f32 vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        f32 v = d1 / (d1 - d3);
        return a + v * ab; // Edge AB region
    }

    Vec3 cp = p - c;
    f32 d5 = glm::dot(ab, cp);
    f32 d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c; // Vertex C region

    f32 vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        f32 w = d2 / (d2 - d6);
        return a + w * ac; // Edge AC region
    }

    f32 va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * (c - b); // Edge BC region
    }

    // Inside triangle
    f32 denom = 1.0f / (va + vb + vc);
    f32 v = vb * denom;
    f32 w = vc * denom;
    return a + v * ab + w * ac;
}

} // anonymous namespace

AlignmentResult validateAlignment(
    const gcode::Program& program,
    const Mesh& mesh,
    const ModelFitter& fitter,
    const FitParams& params,
    f32 samplePercent,
    f32 toleranceMm)
{
    // Early exit: insufficient data
    if (program.path.empty() || mesh.triangleCount() == 0) {
        return {false, false, 0, 0, 0.0f};
    }

    // Collect indices of non-rapid (cutting) segments
    std::vector<size_t> cuttingIndices;
    cuttingIndices.reserve(program.path.size());
    for (size_t i = 0; i < program.path.size(); ++i) {
        if (!program.path[i].isRapid) {
            cuttingIndices.push_back(i);
        }
    }

    if (cuttingIndices.empty()) {
        return {false, false, 0, 0, 0.0f};
    }

    // Forward-transform mesh vertices into G-code space
    const auto& verts = mesh.vertices();
    std::vector<Vec3> transformedVerts(verts.size());
    for (size_t i = 0; i < verts.size(); ++i) {
        transformedVerts[i] = fitter.transform(verts[i].position, params);
    }

    // Deterministic stride-based sampling
    int sampleCount = std::max(1, static_cast<int>(
        static_cast<f32>(cuttingIndices.size()) * samplePercent / 100.0f));
    int stride = std::max(1, static_cast<int>(cuttingIndices.size()) / sampleCount);

    const auto& indices = mesh.indices();
    u32 triCount = mesh.triangleCount();
    f32 toleranceSq = toleranceMm * toleranceMm;

    int pointsTested = 0;
    int pointsNear = 0;

    for (size_t si = 0; si < cuttingIndices.size(); si += static_cast<size_t>(stride)) {
        const auto& seg = program.path[cuttingIndices[si]];
        // Use midpoint of segment as test point
        Vec3 testPoint = (seg.start + seg.end) * 0.5f;

        // Find minimum squared distance to any triangle
        f32 minDistSq = std::numeric_limits<f32>::max();

        for (u32 t = 0; t < triCount; ++t) {
            u32 i0 = indices[t * 3 + 0];
            u32 i1 = indices[t * 3 + 1];
            u32 i2 = indices[t * 3 + 2];

            Vec3 closest = closestPointOnTriangle(
                testPoint, transformedVerts[i0], transformedVerts[i1], transformedVerts[i2]);

            Vec3 diff = testPoint - closest;
            f32 distSq = glm::dot(diff, diff);
            if (distSq < minDistSq) {
                minDistSq = distSq;
                // Early exit if already within tolerance
                if (minDistSq < toleranceSq) break;
            }
        }

        pointsTested++;
        if (minDistSq < toleranceSq) {
            pointsNear++;
        }
    }

    if (pointsTested == 0) {
        return {false, false, 0, 0, 0.0f};
    }

    f32 nearRatio = static_cast<f32>(pointsNear) / static_cast<f32>(pointsTested);
    bool aligned = nearRatio >= 0.7f;

    return {true, aligned, pointsTested, pointsNear, nearRatio};
}

} // namespace carve
} // namespace dw
