// Digital Workshop - OBJ Loader Tests

#include <gtest/gtest.h>

#include "core/loaders/obj_loader.h"

namespace {

dw::ByteBuffer toBuffer(const std::string& str) {
    return dw::ByteBuffer(str.begin(), str.end());
}

}  // namespace

TEST(OBJLoader, LoadFromBuffer_SingleTriangle) {
    std::string obj =
        "# Simple triangle\n"
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3\n";

    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(toBuffer(obj));

    ASSERT_TRUE(result.success()) << "Error: " << result.error;
    ASSERT_NE(result.mesh, nullptr);
    EXPECT_EQ(result.mesh->triangleCount(), 1u);
    EXPECT_EQ(result.mesh->vertexCount(), 3u);

    // Verify positions
    const auto& verts = result.mesh->vertices();
    EXPECT_FLOAT_EQ(verts[0].position.x, 0.0f);
    EXPECT_FLOAT_EQ(verts[0].position.y, 0.0f);
    EXPECT_FLOAT_EQ(verts[0].position.z, 0.0f);

    EXPECT_FLOAT_EQ(verts[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(verts[1].position.y, 0.0f);

    EXPECT_FLOAT_EQ(verts[2].position.y, 1.0f);
}

TEST(OBJLoader, LoadFromBuffer_Quad) {
    // A quad face should be triangulated into 2 triangles
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 1.0 1.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3 4\n";

    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(toBuffer(obj));

    ASSERT_TRUE(result.success()) << "Error: " << result.error;
    EXPECT_EQ(result.mesh->triangleCount(), 2u);
    EXPECT_EQ(result.mesh->vertexCount(), 4u);
}

TEST(OBJLoader, LoadFromBuffer_WithNormals) {
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1//1 2//1 3//1\n";

    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(toBuffer(obj));

    ASSERT_TRUE(result.success()) << "Error: " << result.error;
    EXPECT_EQ(result.mesh->triangleCount(), 1u);

    // Normals should be set from vn data
    const auto& verts = result.mesh->vertices();
    EXPECT_FLOAT_EQ(verts[0].normal.z, 1.0f);
}

TEST(OBJLoader, LoadFromBuffer_EmptyData) {
    dw::ByteBuffer empty;
    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(empty);

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.mesh, nullptr);
    EXPECT_FALSE(result.error.empty());
}

TEST(OBJLoader, LoadFromBuffer_NoFaces) {
    // Vertices but no faces should be an error
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n";

    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(toBuffer(obj));

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.mesh, nullptr);
}

TEST(OBJLoader, LoadFromBuffer_CommentsIgnored) {
    std::string obj =
        "# This is a comment\n"
        "v 0.0 0.0 0.0\n"
        "# Another comment\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3\n";

    dw::OBJLoader loader;
    auto result = loader.loadFromBuffer(toBuffer(obj));

    ASSERT_TRUE(result.success()) << "Error: " << result.error;
    EXPECT_EQ(result.mesh->triangleCount(), 1u);
}

TEST(OBJLoader, SupportsExtension) {
    dw::OBJLoader loader;
    EXPECT_TRUE(loader.supports("obj"));
    EXPECT_TRUE(loader.supports("OBJ"));
    EXPECT_FALSE(loader.supports("stl"));
    EXPECT_FALSE(loader.supports(""));
}
