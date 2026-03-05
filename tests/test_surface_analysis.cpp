// Digital Workshop - Surface Analysis (Curvature) Tests

#include <gtest/gtest.h>

#include "core/carve/surface_analysis.h"

#include <cmath>
#include <vector>

namespace {

using dw::f32;

// Helper: build a heightmap from a lambda Z = f(x, y)
dw::carve::Heightmap buildFromFunc(f32 size, f32 res,
    std::function<f32(f32, f32)> zFunc) {
    // Create mesh vertices for a grid of triangles
    const int gridN = static_cast<int>(size / res) + 1;
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;

    for (int r = 0; r < gridN; ++r) {
        for (int c = 0; c < gridN; ++c) {
            const f32 x = static_cast<f32>(c) * res;
            const f32 y = static_cast<f32>(r) * res;
            verts.push_back(dw::Vertex({x, y, zFunc(x, y)}));
        }
    }
    for (int r = 0; r < gridN - 1; ++r) {
        for (int c = 0; c < gridN - 1; ++c) {
            const auto i = static_cast<dw::u32>(r * gridN + c);
            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + static_cast<dw::u32>(gridN));
            indices.push_back(i + 1);
            indices.push_back(i + static_cast<dw::u32>(gridN) + 1);
            indices.push_back(i + static_cast<dw::u32>(gridN));
        }
    }

    dw::carve::Heightmap hm;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = res;
    hm.build(verts, indices,
             dw::Vec3(0.0f), dw::Vec3(size, size, 100.0f), cfg);
    return hm;
}

} // namespace

TEST(CurvatureAnalysis, FlatSurface) {
    auto hm = buildFromFunc(10.0f, 1.0f,
        [](f32 /*x*/, f32 /*y*/) { return 5.0f; });

    auto result = dw::carve::analyzeCurvature(hm);
    EXPECT_EQ(result.concavePointCount, 0);
}

TEST(CurvatureAnalysis, SphericalBowl) {
    // Parabolic bowl: z = k * (dx^2 + dy^2), centered at (10,10) on 20x20mm domain.
    // Mean curvature H = 2*k everywhere, so radius of curvature R = 1/(2*k).
    // Using k=0.1 gives expected radius = 5mm.
    const f32 k = 0.1f;
    const f32 expectedRadius = 1.0f / (2.0f * k);  // 5.0mm
    const f32 size = 20.0f;
    auto hm = buildFromFunc(size, 0.25f,
        [k, size](f32 x, f32 y) {
            const f32 cx = size * 0.5f;
            const f32 cy = size * 0.5f;
            const f32 dx = x - cx;
            const f32 dy = y - cy;
            return k * (dx * dx + dy * dy);
        });

    auto result = dw::carve::analyzeCurvature(hm);
    EXPECT_GT(result.concavePointCount, 0);
    // The minimum radius should be close to the theoretical value.
    // Allow generous tolerance for discrete approximation.
    EXPECT_GT(result.minConcaveRadius, expectedRadius * 0.3f);
    EXPECT_LT(result.minConcaveRadius, expectedRadius * 3.0f);
    // Average should also be near expected
    EXPECT_GT(result.avgConcaveRadius, expectedRadius * 0.3f);
}

TEST(CurvatureAnalysis, VGroove) {
    // V-groove: z = -|x - 5| (sharp valley at x=5)
    auto hm = buildFromFunc(10.0f, 0.5f,
        [](f32 x, f32 /*y*/) {
            return -std::abs(x - 5.0f);
        });

    auto result = dw::carve::analyzeCurvature(hm);
    // V-groove has very high curvature at the bottom, small radius
    if (result.concavePointCount > 0) {
        EXPECT_LT(result.minConcaveRadius, 5.0f);
    }
}

TEST(CurvatureAnalysis, LargeRadius) {
    // Very gentle curve: z = 0.001 * (x-5)^2
    auto hm = buildFromFunc(10.0f, 0.5f,
        [](f32 x, f32 /*y*/) {
            const f32 dx = x - 5.0f;
            return 0.001f * dx * dx;
        });

    auto result = dw::carve::analyzeCurvature(hm);
    if (result.concavePointCount > 0) {
        // Very gentle curve should have a large radius
        EXPECT_GT(result.minConcaveRadius, 50.0f);
    }
}

TEST(CurvatureAnalysis, MinRadiusLocation) {
    // Bowl at (3,3) with small radius, flat elsewhere
    auto hm = buildFromFunc(10.0f, 0.5f,
        [](f32 x, f32 y) {
            const f32 dx = x - 3.0f;
            const f32 dy = y - 3.0f;
            const f32 r2 = dx * dx + dy * dy;
            if (r2 < 4.0f) {
                return -0.5f * (4.0f - r2);  // Bowl
            }
            return 0.0f;
        });

    auto result = dw::carve::analyzeCurvature(hm);
    if (result.concavePointCount > 0) {
        // Min radius should be near grid position of (3,3)
        const f32 res = hm.resolution();
        const f32 worldX = hm.boundsMin().x + static_cast<f32>(result.minRadiusCol) * res;
        const f32 worldY = hm.boundsMin().y + static_cast<f32>(result.minRadiusRow) * res;
        EXPECT_NEAR(worldX, 3.0f, 2.0f);
        EXPECT_NEAR(worldY, 3.0f, 2.0f);
    }
}
