#include "surface_analysis.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dw {
namespace carve {

namespace {

// Count concave neighbors in 4-connected neighborhood
int countConcaveNeighbors(const Heightmap& hm, int col, int row, f32 noiseThreshold) {
    int count = 0;
    const f32 res = hm.resolution();
    const f32 resSq = res * res;

    // Offsets for 4-connected neighbors (only interior cells have all 4)
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; ++i) {
        const int nc = col + dx[i];
        const int nr = row + dy[i];
        if (nc < 1 || nc >= hm.cols() - 1 || nr < 1 || nr >= hm.rows() - 1) {
            continue;
        }

        // Check if this neighbor is also concave
        const f32 d2x = (hm.at(nc + 1, nr) - 2.0f * hm.at(nc, nr) + hm.at(nc - 1, nr)) / resSq;
        const f32 d2y = (hm.at(nc, nr + 1) - 2.0f * hm.at(nc, nr) + hm.at(nc, nr - 1)) / resSq;
        const f32 meanH = (d2x + d2y) * 0.5f;

        if (meanH > noiseThreshold) {
            ++count;
        }
    }
    return count;
}

} // namespace

f32 computeLocalRadius(const Heightmap& heightmap, int col, int row) {
    if (col < 1 || col >= heightmap.cols() - 1 ||
        row < 1 || row >= heightmap.rows() - 1) {
        return 0.0f;
    }

    const f32 res = heightmap.resolution();
    const f32 resSq = res * res;

    // Central differences for second derivatives
    const f32 z = heightmap.at(col, row);
    const f32 d2x = (heightmap.at(col + 1, row) - 2.0f * z + heightmap.at(col - 1, row)) / resSq;
    const f32 d2y = (heightmap.at(col, row + 1) - 2.0f * z + heightmap.at(col, row - 1)) / resSq;

    const f32 meanCurvature = (d2x + d2y) * 0.5f;

    if (std::abs(meanCurvature) < 1e-8f) {
        return 0.0f;
    }

    // Positive curvature = concave (valleys), negative = convex (peaks)
    // Return positive for concave, negative for convex
    const f32 radius = 1.0f / meanCurvature;
    return radius;
}

CurvatureResult analyzeCurvature(const Heightmap& heightmap) {
    CurvatureResult result;

    if (heightmap.empty() || heightmap.cols() < 3 || heightmap.rows() < 3) {
        return result;
    }

    const f32 res = heightmap.resolution();
    const f32 resSq = res * res;

    // Minimum detectable curvature (noise floor).
    // Derived from resolution: the smallest meaningful Z delta in central
    // differences is limited by float precision on typical mesh heights.
    // A curvature below this threshold is indistinguishable from flat.
    const f32 noiseThreshold = 0.001f / resSq;

    f32 minRadius = std::numeric_limits<f32>::max();
    f64 radiusSum = 0.0;
    int concaveCount = 0;

    // Iterate interior cells (skip 1-cell border)
    for (int row = 1; row < heightmap.rows() - 1; ++row) {
        for (int col = 1; col < heightmap.cols() - 1; ++col) {
            const f32 z = heightmap.at(col, row);
            const f32 d2x = (heightmap.at(col + 1, row) - 2.0f * z +
                             heightmap.at(col - 1, row)) / resSq;
            const f32 d2y = (heightmap.at(col, row + 1) - 2.0f * z +
                             heightmap.at(col, row - 1)) / resSq;
            const f32 meanH = (d2x + d2y) * 0.5f;

            // Only consider concave cells above noise threshold
            if (meanH <= noiseThreshold) {
                continue;
            }

            // Require at least 2 concave neighbors to filter noise
            if (countConcaveNeighbors(heightmap, col, row, noiseThreshold) < 2) {
                continue;
            }

            const f32 radius = 1.0f / meanH;
            ++concaveCount;
            radiusSum += static_cast<f64>(radius);

            if (radius < minRadius) {
                minRadius = radius;
                result.minRadiusCol = col;
                result.minRadiusRow = row;
            }
        }
    }

    result.concavePointCount = concaveCount;
    if (concaveCount > 0) {
        result.minConcaveRadius = minRadius;
        result.avgConcaveRadius = static_cast<f32>(radiusSum / concaveCount);
    }

    return result;
}

} // namespace carve
} // namespace dw
