// Digital Workshop - MaterialArchive Tests

#include <algorithm>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "core/materials/material.h"
#include "core/materials/material_archive.h"

namespace fs = std::filesystem;

namespace {

// Minimal valid 1x1 white PNG (27 bytes)
// Generated with Python: import struct, zlib
// This is a real minimal PNG that stb_image and libpng can read.
constexpr uint8_t kMinimalPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, // IHDR chunk (13 bytes)
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, // width=1, height=1
    0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, // bit depth=8, color=RGB, CRC
    0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41, // IDAT chunk (12 bytes)
    0x54, 0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00, // compressed pixel data
    0x00, 0x00, 0x02, 0x00, 0x01, 0xE2, 0x21, 0xBC, // CRC
    0x33, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, // IEND chunk
    0x44, 0xAE, 0x42, 0x60, 0x82                    // IEND CRC
};

// Fixture: manages a temp directory for test archives
class MaterialArchiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / "dw_material_archive_test";
        fs::create_directories(m_tempDir);

        // Write the minimal PNG to a file for use in tests
        m_texturePath = (m_tempDir / "test_texture.png").string();
        std::ofstream f(m_texturePath, std::ios::binary);
        f.write(reinterpret_cast<const char*>(kMinimalPng), sizeof(kMinimalPng));
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    std::string archivePath(const std::string& name) const {
        return (m_tempDir / (name + dw::MaterialArchive::Extension)).string();
    }

    dw::MaterialRecord makeMaterial(const std::string& name = "Red Oak") const {
        dw::MaterialRecord rec;
        rec.name = name;
        rec.category = dw::MaterialCategory::Hardwood;
        rec.jankaHardness = 1290.0f;
        rec.feedRate = 100.0f;
        rec.spindleSpeed = 18000.0f;
        rec.depthOfCut = 0.125f;
        rec.costPerBoardFoot = 4.50f;
        rec.grainDirectionDeg = 0.0f;
        return rec;
    }

    fs::path m_tempDir;
    std::string m_texturePath;
};

} // namespace

// --- create() ---

TEST_F(MaterialArchiveTest, Create_Succeeds) {
    auto path = archivePath("oak");
    auto result = dw::MaterialArchive::create(path, m_texturePath, makeMaterial());
    EXPECT_TRUE(result.success) << result.error;
    EXPECT_TRUE(fs::exists(path));
}

TEST_F(MaterialArchiveTest, Create_ReturnsExpectedFiles) {
    auto path = archivePath("oak");
    auto result = dw::MaterialArchive::create(path, m_texturePath, makeMaterial());
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.files.size(), 2u);
    EXPECT_NE(std::find(result.files.begin(), result.files.end(), "texture.png"),
              result.files.end());
    EXPECT_NE(std::find(result.files.begin(), result.files.end(), "metadata.json"),
              result.files.end());
}

TEST_F(MaterialArchiveTest, Create_MissingTextureFails) {
    auto path = archivePath("oak");
    auto result =
        dw::MaterialArchive::create(path, "/nonexistent/path/texture.png", makeMaterial());
    EXPECT_FALSE(result.success);
}

// --- load() ---

TEST_F(MaterialArchiveTest, Load_Succeeds) {
    auto path = archivePath("oak");
    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, makeMaterial()).success);

    auto data = dw::MaterialArchive::load(path);
    ASSERT_TRUE(data.has_value());
}

TEST_F(MaterialArchiveTest, Load_TextureDataNonEmpty) {
    auto path = archivePath("oak");
    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, makeMaterial()).success);

    auto data = dw::MaterialArchive::load(path);
    ASSERT_TRUE(data.has_value());
    EXPECT_FALSE(data->textureData.empty());
    // Must match the size of the PNG we wrote
    EXPECT_EQ(data->textureData.size(), sizeof(kMinimalPng));
}

TEST_F(MaterialArchiveTest, Load_MetadataRoundTrips) {
    auto path = archivePath("oak");
    auto original = makeMaterial("White Oak");
    original.category = dw::MaterialCategory::Hardwood;
    original.jankaHardness = 1360.0f;
    original.feedRate = 90.0f;
    original.spindleSpeed = 16000.0f;
    original.depthOfCut = 0.1f;
    original.costPerBoardFoot = 5.25f;
    original.grainDirectionDeg = 45.0f;

    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, original).success);

    auto data = dw::MaterialArchive::load(path);
    ASSERT_TRUE(data.has_value());

    const auto& meta = data->metadata;
    EXPECT_EQ(meta.name, "White Oak");
    EXPECT_EQ(meta.category, dw::MaterialCategory::Hardwood);
    EXPECT_FLOAT_EQ(meta.jankaHardness, 1360.0f);
    EXPECT_FLOAT_EQ(meta.feedRate, 90.0f);
    EXPECT_FLOAT_EQ(meta.spindleSpeed, 16000.0f);
    EXPECT_FLOAT_EQ(meta.depthOfCut, 0.1f);
    EXPECT_FLOAT_EQ(meta.costPerBoardFoot, 5.25f);
    EXPECT_FLOAT_EQ(meta.grainDirectionDeg, 45.0f);
}

TEST_F(MaterialArchiveTest, Load_CategoryRoundTrips) {
    struct TestCase {
        dw::MaterialCategory category;
        const char* name;
    };
    TestCase cases[] = {
        {dw::MaterialCategory::Hardwood, "hardwood"},
        {dw::MaterialCategory::Softwood, "softwood"},
        {dw::MaterialCategory::Domestic, "domestic"},
        {dw::MaterialCategory::Composite, "composite"},
    };

    for (const auto& tc : cases) {
        auto path = archivePath(std::string("cat_") + tc.name);
        auto rec = makeMaterial(tc.name);
        rec.category = tc.category;
        ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, rec).success);

        auto data = dw::MaterialArchive::load(path);
        ASSERT_TRUE(data.has_value()) << "Failed for category: " << tc.name;
        EXPECT_EQ(data->metadata.category, tc.category);
    }
}

TEST_F(MaterialArchiveTest, Load_NonExistentFails) {
    auto data = dw::MaterialArchive::load("/nonexistent/path/material.dwmat");
    EXPECT_FALSE(data.has_value());
}

// --- list() ---

TEST_F(MaterialArchiveTest, List_ReturnsExpectedEntries) {
    auto path = archivePath("oak");
    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, makeMaterial()).success);

    auto entries = dw::MaterialArchive::list(path);
    ASSERT_EQ(entries.size(), 2u);

    bool hasTexture = false;
    bool hasMetadata = false;
    for (const auto& e : entries) {
        if (e.path == "texture.png")
            hasTexture = true;
        if (e.path == "metadata.json")
            hasMetadata = true;
    }
    EXPECT_TRUE(hasTexture);
    EXPECT_TRUE(hasMetadata);
}

TEST_F(MaterialArchiveTest, List_SizesAreNonZero) {
    auto path = archivePath("oak");
    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, makeMaterial()).success);

    auto entries = dw::MaterialArchive::list(path);
    for (const auto& e : entries) {
        EXPECT_GT(e.uncompressedSize, 0u) << "Entry has zero size: " << e.path;
    }
}

TEST_F(MaterialArchiveTest, List_EmptyOnNonExistent) {
    auto entries = dw::MaterialArchive::list("/nonexistent/path/material.dwmat");
    EXPECT_TRUE(entries.empty());
}

// --- isValidArchive() ---

TEST_F(MaterialArchiveTest, IsValidArchive_ValidArchive) {
    auto path = archivePath("oak");
    ASSERT_TRUE(dw::MaterialArchive::create(path, m_texturePath, makeMaterial()).success);
    EXPECT_TRUE(dw::MaterialArchive::isValidArchive(path));
}

TEST_F(MaterialArchiveTest, IsValidArchive_NonExistentFile) {
    EXPECT_FALSE(dw::MaterialArchive::isValidArchive("/nonexistent/material.dwmat"));
}

TEST_F(MaterialArchiveTest, IsValidArchive_NotAZip) {
    auto path = (m_tempDir / "not_a_zip.dwmat").string();
    std::ofstream f(path);
    f << "this is not a zip file";
    f.close();
    EXPECT_FALSE(dw::MaterialArchive::isValidArchive(path));
}

TEST_F(MaterialArchiveTest, Extension_IsCorrect) {
    EXPECT_STREQ(dw::MaterialArchive::Extension, ".dwmat");
}
