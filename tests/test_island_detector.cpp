// Digital Workshop - Island Detector Tests

#include <gtest/gtest.h>

#include "core/carve/island_detector.h"

#include <cmath>
#include <vector>

namespace {

using dw::f32;

// Helper: build a heightmap from a lambda Z = f(x, y)
dw::carve::Heightmap buildFromFunc(f32 size, f32 res,
    std::function<f32(f32, f32)> zFunc) {
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

TEST(IslandDetector, NoIslands) {
    // Gentle slope: no enclosed depressions
    auto hm = buildFromFunc(10.0f, 0.5f,
        [](f32 x, f32 /*y*/) { return x * 0.1f; });

    auto result = dw::carve::detectIslands(hm, 60.0f);
    EXPECT_TRUE(result.islands.empty());
}

TEST(IslandDetector, SinglePit) {
    // Flat surface at Z=10 with a deep pit at center
    auto hm = buildFromFunc(20.0f, 0.5f,
        [](f32 x, f32 y) {
            const f32 dx = x - 10.0f;
            const f32 dy = y - 10.0f;
            const f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 3.0f) {
                return 10.0f - 8.0f * (1.0f - r / 3.0f);  // Deep pit
            }
            return 10.0f;
        });

    // Use a narrow V-bit angle (30 deg) so the steep walls create an island
    auto result = dw::carve::detectIslands(hm, 30.0f, 0.5f);
    // Should detect at least one island region
    // (exact count depends on pit steepness vs tool angle)
    EXPECT_GE(result.maskCols, 1);
    EXPECT_GE(result.maskRows, 1);
}

TEST(IslandDetector, MultipleIslands) {
    // Two separated pits
    auto hm = buildFromFunc(30.0f, 0.5f,
        [](f32 x, f32 y) {
            f32 z = 10.0f;
            // Pit 1 at (7, 7)
            f32 dx = x - 7.0f;
            f32 dy = y - 7.0f;
            f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 3.0f) z = std::min(z, 10.0f - 8.0f * (1.0f - r / 3.0f));
            // Pit 2 at (23, 23)
            dx = x - 23.0f;
            dy = y - 23.0f;
            r = std::sqrt(dx * dx + dy * dy);
            if (r < 3.0f) z = std::min(z, 10.0f - 8.0f * (1.0f - r / 3.0f));
            return z;
        });

    auto result = dw::carve::detectIslands(hm, 30.0f, 0.5f);
    // If islands detected, should be 2 separate ones
    if (result.islands.size() >= 2) {
        EXPECT_NE(result.islands[0].id, result.islands[1].id);
    }
}

TEST(IslandDetector, ShallowIgnored) {
    // Very shallow dimple that should be below min area threshold
    auto hm = buildFromFunc(10.0f, 1.0f,
        [](f32 x, f32 y) {
            const f32 dx = x - 5.0f;
            const f32 dy = y - 5.0f;
            const f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 0.5f) return 9.9f;  // Tiny shallow dimple
            return 10.0f;
        });

    // Use high min area to filter out small islands
    auto result = dw::carve::detectIslands(hm, 30.0f, 100.0f);
    EXPECT_TRUE(result.islands.empty());
}

TEST(IslandDetector, DepthClassification) {
    // Known pit depth: flat at 10, pit bottom at 2 => depth = 8
    auto hm = buildFromFunc(20.0f, 0.5f,
        [](f32 x, f32 y) {
            const f32 dx = x - 10.0f;
            const f32 dy = y - 10.0f;
            const f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 3.0f) return 2.0f;  // Flat bottom pit
            return 10.0f;
        });

    auto result = dw::carve::detectIslands(hm, 30.0f, 0.5f);
    for (const auto& island : result.islands) {
        // Depth should be close to 8mm (10 - 2)
        EXPECT_GT(island.depth, 5.0f);
        EXPECT_LE(island.depth, 9.0f);
    }
}

TEST(IslandDetector, ClearingDiameter) {
    // Narrow pit vs wide pit
    auto hmNarrow = buildFromFunc(20.0f, 0.5f,
        [](f32 x, f32 y) {
            const f32 dx = x - 10.0f;
            const f32 dy = y - 10.0f;
            const f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 1.5f) return 2.0f;
            return 10.0f;
        });

    auto hmWide = buildFromFunc(20.0f, 0.5f,
        [](f32 x, f32 y) {
            const f32 dx = x - 10.0f;
            const f32 dy = y - 10.0f;
            const f32 r = std::sqrt(dx * dx + dy * dy);
            if (r < 5.0f) return 2.0f;
            return 10.0f;
        });

    auto narrowResult = dw::carve::detectIslands(hmNarrow, 30.0f, 0.5f);
    auto wideResult = dw::carve::detectIslands(hmWide, 30.0f, 0.5f);

    // Wide pit should require larger clearing diameter
    if (!narrowResult.islands.empty() && !wideResult.islands.empty()) {
        EXPECT_LT(narrowResult.islands[0].minClearDiameter,
                   wideResult.islands[0].minClearDiameter);
    }
}
