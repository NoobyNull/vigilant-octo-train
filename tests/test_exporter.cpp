// Digital Workshop - Model Exporter Tests (roundtrip: export → re-import)

#include <gtest/gtest.h>

#include "core/export/model_exporter.h"
#include "core/loaders/obj_loader.h"
#include "core/loaders/stl_loader.h"
#include "core/utils/file_utils.h"

#include <filesystem>

namespace {

class ExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tmpDir = std::filesystem::temp_directory_path() / "dw_test_exporter";
        std::filesystem::create_directories(m_tmpDir);
    }

    void TearDown() override { std::filesystem::remove_all(m_tmpDir); }

    // Build a simple triangle mesh for export
    dw::Mesh makeTriangle() {
        std::vector<dw::Vertex> verts = {
            dw::Vertex(dw::Vec3(0.0f, 0.0f, 0.0f), dw::Vec3(0, 0, 1)),
            dw::Vertex(dw::Vec3(1.0f, 0.0f, 0.0f), dw::Vec3(0, 0, 1)),
            dw::Vertex(dw::Vec3(0.0f, 1.0f, 0.0f), dw::Vec3(0, 0, 1)),
        };
        std::vector<dw::u32> indices = {0, 1, 2};
        return dw::Mesh(std::move(verts), std::move(indices));
    }

    // Build a quad (2 triangles)
    dw::Mesh makeQuad() {
        std::vector<dw::Vertex> verts = {
            dw::Vertex(dw::Vec3(0.0f, 0.0f, 0.0f), dw::Vec3(0, 0, 1)),
            dw::Vertex(dw::Vec3(1.0f, 0.0f, 0.0f), dw::Vec3(0, 0, 1)),
            dw::Vertex(dw::Vec3(1.0f, 1.0f, 0.0f), dw::Vec3(0, 0, 1)),
            dw::Vertex(dw::Vec3(0.0f, 1.0f, 0.0f), dw::Vec3(0, 0, 1)),
        };
        std::vector<dw::u32> indices = {0, 1, 2, 0, 2, 3};
        return dw::Mesh(std::move(verts), std::move(indices));
    }

    dw::Path m_tmpDir;
};

}  // namespace

// --- STL Binary export + re-import ---

TEST_F(ExporterTest, STLBinary_Roundtrip) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "test.stl";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path, dw::ExportFormat::STL_Binary);
    ASSERT_TRUE(result.success) << "Export error: " << result.error;
    EXPECT_TRUE(dw::file::exists(path));

    // Re-import
    dw::STLLoader loader;
    auto imported = loader.load(path);
    ASSERT_TRUE(imported.success()) << "Import error: " << imported.error;
    EXPECT_EQ(imported.mesh->triangleCount(), 1u);
    EXPECT_EQ(imported.mesh->vertexCount(), 3u);
}

TEST_F(ExporterTest, STLBinary_MultiTriangle) {
    auto mesh = makeQuad();
    auto path = m_tmpDir / "quad.stl";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path, dw::ExportFormat::STL_Binary);
    ASSERT_TRUE(result.success) << result.error;

    dw::STLLoader loader;
    auto imported = loader.load(path);
    ASSERT_TRUE(imported.success()) << imported.error;
    EXPECT_EQ(imported.mesh->triangleCount(), 2u);
}

// --- STL ASCII export + re-import ---

TEST_F(ExporterTest, STLASCII_Roundtrip) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "test_ascii.stl";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path, dw::ExportFormat::STL_ASCII);
    ASSERT_TRUE(result.success) << result.error;

    // Verify file is text (starts with "solid")
    auto text = dw::file::readText(path);
    ASSERT_TRUE(text.has_value());
    EXPECT_TRUE(text->substr(0, 5) == "solid");

    // Re-import
    dw::STLLoader loader;
    auto imported = loader.load(path);
    ASSERT_TRUE(imported.success()) << imported.error;
    EXPECT_EQ(imported.mesh->triangleCount(), 1u);
}

// --- OBJ export + re-import ---

TEST_F(ExporterTest, OBJ_Roundtrip) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "test.obj";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path, dw::ExportFormat::OBJ);
    ASSERT_TRUE(result.success) << result.error;

    // Verify it's text
    auto text = dw::file::readText(path);
    ASSERT_TRUE(text.has_value());
    EXPECT_TRUE(text->find("v ") != std::string::npos);
    EXPECT_TRUE(text->find("f ") != std::string::npos);

    // Re-import
    dw::OBJLoader loader;
    auto imported = loader.load(path);
    ASSERT_TRUE(imported.success()) << imported.error;
    EXPECT_EQ(imported.mesh->triangleCount(), 1u);
}

// --- Auto-detect format from extension ---

TEST_F(ExporterTest, AutoDetect_STL) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "auto.stl";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path);
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_TRUE(dw::file::exists(path));
}

TEST_F(ExporterTest, AutoDetect_OBJ) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "auto.obj";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(mesh, path);
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_TRUE(dw::file::exists(path));
}

// --- Empty mesh export ---

TEST_F(ExporterTest, EmptyMesh) {
    dw::Mesh empty;
    auto path = m_tmpDir / "empty.stl";

    dw::ModelExporter exporter;
    auto result = exporter.exportMesh(empty, path, dw::ExportFormat::STL_Binary);
    // Should either fail gracefully or write an empty STL
    // Just verify no crash
    (void)result;
}

// --- Vertex data preservation ---

TEST_F(ExporterTest, STLBinary_PreservesGeometry) {
    auto mesh = makeTriangle();
    auto path = m_tmpDir / "precision.stl";

    dw::ModelExporter exporter;
    exporter.exportMesh(mesh, path, dw::ExportFormat::STL_Binary);

    dw::STLLoader loader;
    auto imported = loader.load(path);
    ASSERT_TRUE(imported.success());

    // Check vertex positions are preserved within float precision
    const auto& origVerts = mesh.vertices();
    const auto& impVerts = imported.mesh->vertices();

    ASSERT_EQ(impVerts.size(), origVerts.size());
    for (size_t i = 0; i < origVerts.size(); ++i) {
        EXPECT_NEAR(impVerts[i].position.x, origVerts[i].position.x, 1e-5f);
        EXPECT_NEAR(impVerts[i].position.y, origVerts[i].position.y, 1e-5f);
        EXPECT_NEAR(impVerts[i].position.z, origVerts[i].position.z, 1e-5f);
    }
}
