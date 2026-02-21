// Digital Workshop - Mesh UV Generation Tests

#include <gtest/gtest.h>

#include "core/mesh/mesh.h"

#include <cmath>

namespace {

// Create a flat quad-like mesh in the XY plane (4 verts, 2 triangles)
// spanning (0,0,0) to (2,1,0)
dw::Mesh makeXYQuad() {
    std::vector<dw::Vertex> verts = {
        dw::Vertex({0.0f, 0.0f, 0.0f}),
        dw::Vertex({2.0f, 0.0f, 0.0f}),
        dw::Vertex({2.0f, 1.0f, 0.0f}),
        dw::Vertex({0.0f, 1.0f, 0.0f}),
    };
    std::vector<dw::u32> indices = {0, 1, 2, 0, 2, 3};
    return dw::Mesh(std::move(verts), std::move(indices));
}

// Create a flat quad in the XZ plane (4 verts, 2 triangles)
// spanning (0,0,0) to (2,0,1)
dw::Mesh makeXZQuad() {
    std::vector<dw::Vertex> verts = {
        dw::Vertex({0.0f, 0.0f, 0.0f}),
        dw::Vertex({2.0f, 0.0f, 0.0f}),
        dw::Vertex({2.0f, 0.0f, 1.0f}),
        dw::Vertex({0.0f, 0.0f, 1.0f}),
    };
    std::vector<dw::u32> indices = {0, 1, 2, 0, 2, 3};
    return dw::Mesh(std::move(verts), std::move(indices));
}

// Create a default (zero texcoord) mesh
dw::Mesh makeZeroUVMesh() {
    std::vector<dw::Vertex> verts = {
        dw::Vertex({0.0f, 0.0f, 0.0f}),
        dw::Vertex({1.0f, 0.0f, 0.0f}),
        dw::Vertex({0.0f, 1.0f, 0.0f}),
    };
    // All texCoords default to (0,0)
    std::vector<dw::u32> indices = {0, 1, 2};
    return dw::Mesh(std::move(verts), std::move(indices));
}

// Create a mesh with explicit non-zero UV coords
dw::Mesh makeNonZeroUVMesh() {
    std::vector<dw::Vertex> verts = {
        dw::Vertex({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.1f, 0.2f}),
        dw::Vertex({1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.9f, 0.1f}),
        dw::Vertex({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.3f, 0.8f}),
    };
    std::vector<dw::u32> indices = {0, 1, 2};
    return dw::Mesh(std::move(verts), std::move(indices));
}

}  // namespace

// --- needsUVGeneration ---

TEST(MeshUV, NeedsUVGeneration_EmptyMesh) {
    dw::Mesh mesh;
    // Empty mesh: no vertices → needs UV generation
    EXPECT_TRUE(mesh.needsUVGeneration());
}

TEST(MeshUV, NeedsUVGeneration_ZeroTexCoords) {
    auto mesh = makeZeroUVMesh();
    // Default vertices have texCoord (0,0) → needs UV generation
    EXPECT_TRUE(mesh.needsUVGeneration());
}

TEST(MeshUV, NeedsUVGeneration_NonZeroTexCoords) {
    auto mesh = makeNonZeroUVMesh();
    // At least one vertex has non-zero texCoord → does NOT need UV generation
    EXPECT_FALSE(mesh.needsUVGeneration());
}

// --- generatePlanarUVs ---

TEST(MeshUV, GeneratePlanarUVs_ProducesNonZeroUVs) {
    auto mesh = makeZeroUVMesh();
    ASSERT_TRUE(mesh.needsUVGeneration());

    mesh.generatePlanarUVs();

    // After generation, at least some vertices should have non-zero UVs
    EXPECT_FALSE(mesh.needsUVGeneration());
    EXPECT_TRUE(mesh.hasTexCoords());
}

TEST(MeshUV, GeneratePlanarUVs_XYPlane_CorrectMapping) {
    // XY quad (0,0,0) to (2,1,0):
    // XY area = 2*1 = 2, XZ area = 2*0 = 0, YZ area = 1*0 = 0
    // → should project on XY plane
    auto mesh = makeXYQuad();
    mesh.generatePlanarUVs();

    const auto& verts = mesh.vertices();
    // Vertex (0,0,0): u=0, v=0
    EXPECT_NEAR(verts[0].texCoord.x, 0.0f, 1e-5f);
    EXPECT_NEAR(verts[0].texCoord.y, 0.0f, 1e-5f);
    // Vertex (2,0,0): u=1, v=0
    EXPECT_NEAR(verts[1].texCoord.x, 1.0f, 1e-5f);
    EXPECT_NEAR(verts[1].texCoord.y, 0.0f, 1e-5f);
    // Vertex (2,1,0): u=1, v=1
    EXPECT_NEAR(verts[2].texCoord.x, 1.0f, 1e-5f);
    EXPECT_NEAR(verts[2].texCoord.y, 1.0f, 1e-5f);
    // Vertex (0,1,0): u=0, v=1
    EXPECT_NEAR(verts[3].texCoord.x, 0.0f, 1e-5f);
    EXPECT_NEAR(verts[3].texCoord.y, 1.0f, 1e-5f);
}

TEST(MeshUV, GeneratePlanarUVs_XZPlane_CorrectMapping) {
    // XZ quad (0,0,0) to (2,0,1):
    // XY area = 2*0 = 0, XZ area = 2*1 = 2, YZ area = 0*1 = 0
    // → should project on XZ plane
    auto mesh = makeXZQuad();
    mesh.generatePlanarUVs();

    const auto& verts = mesh.vertices();
    // Vertex (0,0,0): u=0, v=0
    EXPECT_NEAR(verts[0].texCoord.x, 0.0f, 1e-5f);
    EXPECT_NEAR(verts[0].texCoord.y, 0.0f, 1e-5f);
    // Vertex (2,0,0): u=1, v=0
    EXPECT_NEAR(verts[1].texCoord.x, 1.0f, 1e-5f);
    EXPECT_NEAR(verts[1].texCoord.y, 0.0f, 1e-5f);
    // Vertex (2,0,1): u=1, v=1
    EXPECT_NEAR(verts[2].texCoord.x, 1.0f, 1e-5f);
    EXPECT_NEAR(verts[2].texCoord.y, 1.0f, 1e-5f);
    // Vertex (0,0,1): u=0, v=1
    EXPECT_NEAR(verts[3].texCoord.x, 0.0f, 1e-5f);
    EXPECT_NEAR(verts[3].texCoord.y, 1.0f, 1e-5f);
}

TEST(MeshUV, GeneratePlanarUVs_GrainRotation_90Deg) {
    // Rotating by 90 degrees around center (0.5, 0.5):
    // (u, v) → (-(v-0.5) + 0.5, (u-0.5) + 0.5) = (1-v, u)
    // Corner (0,0) → (1, 0)
    // Corner (1,0) → (1, 1)
    // Corner (1,1) → (0, 1)
    // Corner (0,1) → (0, 0)
    auto mesh = makeXYQuad();
    mesh.generatePlanarUVs(90.0f);

    const auto& verts = mesh.vertices();
    // Vertex at (0,0,0) had uv=(0,0), after 90° rot: u=cos*(-0.5)-sin*(-0.5)+0.5
    // cos(90)=0, sin(90)=1: u = 0*(-0.5)-1*(-0.5)+0.5 = 0.5+0.5=1, v = 1*(-0.5)+0*(-0.5)+0.5=0
    EXPECT_NEAR(verts[0].texCoord.x, 1.0f, 1e-5f);
    EXPECT_NEAR(verts[0].texCoord.y, 0.0f, 1e-5f);
}

TEST(MeshUV, GeneratePlanarUVs_NoRotation_vs_ZeroRotation) {
    // generatePlanarUVs(0) should produce same result as generatePlanarUVs()
    auto mesh1 = makeXYQuad();
    auto mesh2 = makeXYQuad();

    mesh1.generatePlanarUVs();
    mesh2.generatePlanarUVs(0.0f);

    ASSERT_EQ(mesh1.vertexCount(), mesh2.vertexCount());
    for (size_t i = 0; i < mesh1.vertices().size(); ++i) {
        EXPECT_FLOAT_EQ(mesh1.vertices()[i].texCoord.x, mesh2.vertices()[i].texCoord.x);
        EXPECT_FLOAT_EQ(mesh1.vertices()[i].texCoord.y, mesh2.vertices()[i].texCoord.y);
    }
}

TEST(MeshUV, GeneratePlanarUVs_EmptyMesh_DoesNotCrash) {
    dw::Mesh mesh;
    // Should return early without crashing
    EXPECT_NO_THROW(mesh.generatePlanarUVs());
    EXPECT_EQ(mesh.vertexCount(), 0u);
}

TEST(MeshUV, GeneratePlanarUVs_GrainRotation_ChangesUVs) {
    auto meshNoRot = makeXYQuad();
    auto meshRotated = makeXYQuad();

    meshNoRot.generatePlanarUVs(0.0f);
    meshRotated.generatePlanarUVs(45.0f);

    // After 45° rotation, UVs should differ from no-rotation
    bool anyDifferent = false;
    for (size_t i = 0; i < meshNoRot.vertices().size(); ++i) {
        float du = meshNoRot.vertices()[i].texCoord.x - meshRotated.vertices()[i].texCoord.x;
        float dv = meshNoRot.vertices()[i].texCoord.y - meshRotated.vertices()[i].texCoord.y;
        if (std::fabs(du) > 1e-4f || std::fabs(dv) > 1e-4f) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent);
}
