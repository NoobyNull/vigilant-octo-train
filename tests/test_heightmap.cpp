// Digital Workshop - Heightmap Tests

#include <gtest/gtest.h>

#include "core/carve/heightmap.h"

#include <cmath>
#include <vector>

namespace {

using dw::f32;

// Helper: create a flat quad at given Z from two triangles
void makeFlatQuad(f32 z, f32 size,
                  std::vector<dw::Vertex>& verts,
                  std::vector<dw::u32>& indices) {
    verts.clear();
    indices.clear();
    verts.push_back(dw::Vertex({0.0f, 0.0f, z}));
    verts.push_back(dw::Vertex({size, 0.0f, z}));
    verts.push_back(dw::Vertex({size, size, z}));
    verts.push_back(dw::Vertex({0.0f, size, z}));
    indices = {0, 1, 2, 0, 2, 3};
}

// Helper: create a pyramid (peak at center, base at Z=0)
void makePyramid(f32 baseSize, f32 peakZ,
                 std::vector<dw::Vertex>& verts,
                 std::vector<dw::u32>& indices) {
    verts.clear();
    indices.clear();
    const f32 half = baseSize * 0.5f;
    // Base corners
    verts.push_back(dw::Vertex({0.0f, 0.0f, 0.0f}));       // 0
    verts.push_back(dw::Vertex({baseSize, 0.0f, 0.0f}));    // 1
    verts.push_back(dw::Vertex({baseSize, baseSize, 0.0f})); // 2
    verts.push_back(dw::Vertex({0.0f, baseSize, 0.0f}));     // 3
    // Peak
    verts.push_back(dw::Vertex({half, half, peakZ}));        // 4
    // 4 triangular faces
    indices = {
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4,
    };
}

} // namespace

TEST(Heightmap, EmptyMesh) {
    dw::carve::Heightmap hm;
    EXPECT_TRUE(hm.empty());
    EXPECT_EQ(hm.cols(), 0);
    EXPECT_EQ(hm.rows(), 0);

    // Building with empty data should stay empty
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = 1.0f;
    hm.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f), cfg);
    EXPECT_TRUE(hm.empty());
}

TEST(Heightmap, FlatPlane) {
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    makeFlatQuad(5.0f, 10.0f, verts, indices);

    dw::carve::Heightmap hm;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = 1.0f;
    hm.build(verts, indices, dw::Vec3(0.0f, 0.0f, 0.0f), dw::Vec3(10.0f, 10.0f, 5.0f), cfg);

    EXPECT_FALSE(hm.empty());
    EXPECT_GT(hm.cols(), 0);
    EXPECT_GT(hm.rows(), 0);
    EXPECT_FLOAT_EQ(hm.minZ(), 5.0f);
    EXPECT_FLOAT_EQ(hm.maxZ(), 5.0f);

    // Sample interior cells - all should be 5.0
    for (int r = 1; r < hm.rows() - 1; ++r) {
        for (int c = 1; c < hm.cols() - 1; ++c) {
            EXPECT_NEAR(hm.at(c, r), 5.0f, 0.01f)
                << "at col=" << c << " row=" << r;
        }
    }
}

TEST(Heightmap, SimplePeak) {
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    makePyramid(10.0f, 5.0f, verts, indices);

    dw::carve::Heightmap hm;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = 0.5f;
    hm.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f, 10.0f, 5.0f), cfg);

    EXPECT_FALSE(hm.empty());

    // Center should have the highest Z
    const int midCol = hm.cols() / 2;
    const int midRow = hm.rows() / 2;
    const dw::f32 centerZ = hm.at(midCol, midRow);
    EXPECT_GT(centerZ, 3.0f);  // Near peak

    // Edges should be lower than center
    const dw::f32 edgeZ = hm.at(1, 1);
    EXPECT_LT(edgeZ, centerZ);
}

TEST(Heightmap, Resolution) {
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    makeFlatQuad(1.0f, 10.0f, verts, indices);

    dw::carve::Heightmap lowRes;
    dw::carve::HeightmapConfig cfgLow;
    cfgLow.resolutionMm = 2.0f;
    lowRes.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f, 10.0f, 1.0f), cfgLow);

    dw::carve::Heightmap highRes;
    dw::carve::HeightmapConfig cfgHigh;
    cfgHigh.resolutionMm = 0.5f;
    highRes.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f, 10.0f, 1.0f), cfgHigh);

    EXPECT_GT(highRes.cols(), lowRes.cols());
    EXPECT_GT(highRes.rows(), lowRes.rows());
    EXPECT_FLOAT_EQ(lowRes.resolution(), 2.0f);
    EXPECT_FLOAT_EQ(highRes.resolution(), 0.5f);
}

TEST(Heightmap, BilinearInterp) {
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    makePyramid(10.0f, 5.0f, verts, indices);

    dw::carve::Heightmap hm;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = 1.0f;
    hm.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f, 10.0f, 5.0f), cfg);

    // atMm at center should be close to at() at center cell
    const dw::f32 centerMm = hm.atMm(5.0f, 5.0f);
    const int midCol = hm.cols() / 2;
    const int midRow = hm.rows() / 2;
    const dw::f32 centerGrid = hm.at(midCol, midRow);
    EXPECT_NEAR(centerMm, centerGrid, 0.5f);

    // Clamping: way outside bounds should return a valid value (no crash)
    const dw::f32 outsideZ = hm.atMm(-100.0f, -100.0f);
    EXPECT_GE(outsideZ, hm.minZ());
    EXPECT_LE(outsideZ, hm.maxZ());
}

TEST(Heightmap, ProgressCallback) {
    std::vector<dw::Vertex> verts;
    std::vector<dw::u32> indices;
    makeFlatQuad(1.0f, 10.0f, verts, indices);

    std::vector<dw::f32> progressValues;
    auto onProgress = [&](dw::f32 p) { progressValues.push_back(p); };

    dw::carve::Heightmap hm;
    dw::carve::HeightmapConfig cfg;
    cfg.resolutionMm = 1.0f;
    hm.build(verts, indices, dw::Vec3(0.0f), dw::Vec3(10.0f, 10.0f, 1.0f), cfg, onProgress);

    EXPECT_FALSE(progressValues.empty());

    // Values should be monotonically increasing
    for (size_t i = 1; i < progressValues.size(); ++i) {
        EXPECT_GE(progressValues[i], progressValues[i - 1]);
    }

    // Final value should be 1.0
    EXPECT_FLOAT_EQ(progressValues.back(), 1.0f);
}
