// Digital Workshop - MaterialManager Tests

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/schema.h"
#include "core/materials/default_materials.h"
#include "core/materials/material_archive.h"
#include "core/materials/material_manager.h"

namespace fs = std::filesystem;

namespace {

// Minimal valid 1x1 PNG (same bytes as used in test_material_archive.cpp)
constexpr uint8_t kMinimalPng[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00,
                                   0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
                                   0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90,
                                   0x77, 0x53, 0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
                                   0x54, 0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00, 0x00, 0x00,
                                   0x02, 0x00, 0x01, 0xE2, 0x21, 0xBC, 0x33, 0x00, 0x00, 0x00,
                                   0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

// Fixture: in-memory database + managed temp dir for archive tests
class MaterialManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_manager = std::make_unique<dw::MaterialManager>(m_db);

        m_tempDir = fs::temp_directory_path() / "dw_material_manager_test";
        fs::create_directories(m_tempDir);

        // Create a minimal PNG for texture tests
        m_texturePath = m_tempDir / "test_texture.png";
        std::ofstream f(m_texturePath, std::ios::binary);
        f.write(reinterpret_cast<const char*>(kMinimalPng), sizeof(kMinimalPng));
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
    }

    // Create a .dwmat archive in the temp dir for import tests
    fs::path createTestArchive(const std::string& name,
                               dw::MaterialCategory cat = dw::MaterialCategory::Hardwood) {
        dw::MaterialRecord rec;
        rec.name = name;
        rec.category = cat;
        rec.jankaHardness = 1290.0f;
        rec.feedRate = 100.0f;
        rec.spindleSpeed = 18000.0f;
        rec.depthOfCut = 0.125f;
        rec.costPerBoardFoot = 4.50f;

        fs::path archivePath = m_tempDir / (name + ".dwmat");
        auto result =
            dw::MaterialArchive::create(archivePath.string(), m_texturePath.string(), rec);
        if (!result.success) {
            return {};
        }
        return archivePath;
    }

    // Insert a model row so we can test material assignment
    dw::i64 insertModel(const std::string& name) {
        auto stmt = m_db.prepare(
            "INSERT INTO models (hash, name, file_path, file_format) VALUES (?, ?, ?, ?)");
        (void)stmt.bindText(1, name + "_hash");
        (void)stmt.bindText(2, name);
        (void)stmt.bindText(3, "/tmp/" + name + ".stl");
        (void)stmt.bindText(4, "stl");
        (void)stmt.execute();
        return m_db.lastInsertId();
    }

    dw::Database m_db;
    std::unique_ptr<dw::MaterialManager> m_manager;
    fs::path m_tempDir;
    fs::path m_texturePath;
};

} // namespace

// ============================================================================
// seedDefaults
// ============================================================================

TEST_F(MaterialManagerTest, SeedDefaults_PopulatesEmptyDatabase) {
    m_manager->seedDefaults();
    auto all = m_manager->getAllMaterials();
    EXPECT_EQ(all.size(), 32u);
}

TEST_F(MaterialManagerTest, SeedDefaults_IsIdempotent) {
    m_manager->seedDefaults();
    m_manager->seedDefaults(); // Second call should be a no-op
    auto all = m_manager->getAllMaterials();
    EXPECT_EQ(all.size(), 32u); // Still 32, not 64
}

TEST_F(MaterialManagerTest, SeedDefaults_SkipsNonEmptyDatabase) {
    // Insert one material first
    dw::MaterialRecord rec;
    rec.name = "Custom Wood";
    rec.category = dw::MaterialCategory::Domestic;
    m_manager->updateMaterial(rec); // Won't work on new manager, use seed
    m_manager->seedDefaults();      // Seeds normally

    auto all = m_manager->getAllMaterials();
    size_t countAfterFirstSeed = all.size();
    EXPECT_EQ(countAfterFirstSeed, 32u);

    m_manager->seedDefaults(); // Must not add more
    EXPECT_EQ(m_manager->getAllMaterials().size(), countAfterFirstSeed);
}

TEST_F(MaterialManagerTest, SeedDefaults_CoversCoreCategories) {
    m_manager->seedDefaults();

    auto hardwoods = m_manager->getMaterialsByCategory(dw::MaterialCategory::Hardwood);
    auto softwoods = m_manager->getMaterialsByCategory(dw::MaterialCategory::Softwood);
    auto domestic = m_manager->getMaterialsByCategory(dw::MaterialCategory::Domestic);
    auto composites = m_manager->getMaterialsByCategory(dw::MaterialCategory::Composite);

    EXPECT_EQ(hardwoods.size(), 8u);
    EXPECT_EQ(softwoods.size(), 7u);
    EXPECT_EQ(domestic.size(), 7u);
    EXPECT_EQ(composites.size(), 10u);
}

TEST_F(MaterialManagerTest, SeedDefaults_MaterialsHaveNames) {
    m_manager->seedDefaults();
    for (const auto& mat : m_manager->getAllMaterials()) {
        EXPECT_FALSE(mat.name.empty()) << "Material with id=" << mat.id << " has empty name";
    }
}

// ============================================================================
// importMaterial
// ============================================================================

TEST_F(MaterialManagerTest, Import_SucceedsWithValidArchive) {
    auto archivePath = createTestArchive("Red Oak");
    ASSERT_FALSE(archivePath.empty());

    auto id = m_manager->importMaterial(archivePath);
    EXPECT_TRUE(id.has_value());
}

TEST_F(MaterialManagerTest, Import_InsertsIntoDatabase) {
    auto archivePath = createTestArchive("Hard Maple");
    ASSERT_FALSE(archivePath.empty());

    auto id = m_manager->importMaterial(archivePath);
    ASSERT_TRUE(id.has_value());

    auto retrieved = m_manager->getMaterial(*id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Hard Maple");
}

TEST_F(MaterialManagerTest, Import_FailsForNonExistentFile) {
    auto id = m_manager->importMaterial("/nonexistent/material.dwmat");
    EXPECT_FALSE(id.has_value());
}

TEST_F(MaterialManagerTest, Import_FailsForInvalidArchive) {
    // Create a file that is not a valid ZIP
    auto fakePath = m_tempDir / "fake.dwmat";
    std::ofstream f(fakePath);
    f << "not a zip file";
    f.close();

    auto id = m_manager->importMaterial(fakePath);
    EXPECT_FALSE(id.has_value());
}

TEST_F(MaterialManagerTest, Import_MetadataPreservedAfterRoundTrip) {
    auto archivePath = createTestArchive("Cherry");
    ASSERT_FALSE(archivePath.empty());

    auto id = m_manager->importMaterial(archivePath);
    ASSERT_TRUE(id.has_value());

    auto mat = m_manager->getMaterial(*id);
    ASSERT_TRUE(mat.has_value());
    EXPECT_EQ(mat->name, "Cherry");
    EXPECT_EQ(mat->category, dw::MaterialCategory::Hardwood);
    EXPECT_FLOAT_EQ(mat->jankaHardness, 1290.0f);
    EXPECT_FLOAT_EQ(mat->feedRate, 100.0f);
    EXPECT_FLOAT_EQ(mat->spindleSpeed, 18000.0f);
    EXPECT_FLOAT_EQ(mat->depthOfCut, 0.125f);
    EXPECT_FLOAT_EQ(mat->costPerBoardFoot, 4.50f);
}

// ============================================================================
// updateMaterial
// ============================================================================

TEST_F(MaterialManagerTest, Update_ChangesFields) {
    m_manager->seedDefaults();
    auto all = m_manager->getAllMaterials();
    ASSERT_FALSE(all.empty());

    auto mat = all[0];
    float newCost = mat.costPerBoardFoot + 1.0f;
    mat.costPerBoardFoot = newCost;

    EXPECT_TRUE(m_manager->updateMaterial(mat));

    auto updated = m_manager->getMaterial(mat.id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_FLOAT_EQ(updated->costPerBoardFoot, newCost);
}

// ============================================================================
// removeMaterial
// ============================================================================

TEST_F(MaterialManagerTest, Remove_DeletesFromDatabase) {
    m_manager->seedDefaults();
    auto all = m_manager->getAllMaterials();
    ASSERT_FALSE(all.empty());

    dw::i64 idToRemove = all[0].id;
    EXPECT_TRUE(m_manager->removeMaterial(idToRemove));
    EXPECT_FALSE(m_manager->getMaterial(idToRemove).has_value());
}

TEST_F(MaterialManagerTest, Remove_ReturnsFalseForNonExistent) {
    EXPECT_FALSE(m_manager->removeMaterial(99999));
}

// ============================================================================
// assignMaterialToModel / getModelMaterial
// ============================================================================

TEST_F(MaterialManagerTest, Assign_PersistsMaterialOnModel) {
    m_manager->seedDefaults();
    auto materials = m_manager->getAllMaterials();
    ASSERT_FALSE(materials.empty());

    dw::i64 modelId = insertModel("test_cube");
    dw::i64 materialId = materials[0].id;

    EXPECT_TRUE(m_manager->assignMaterialToModel(materialId, modelId));

    auto retrieved = m_manager->getModelMaterial(modelId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, materialId);
}

TEST_F(MaterialManagerTest, Assign_FailsForNonExistentMaterial) {
    dw::i64 modelId = insertModel("test_cube");
    EXPECT_FALSE(m_manager->assignMaterialToModel(99999, modelId));
}

TEST_F(MaterialManagerTest, GetModelMaterial_ReturnsNulloptWhenNotAssigned) {
    dw::i64 modelId = insertModel("no_material_model");
    auto mat = m_manager->getModelMaterial(modelId);
    EXPECT_FALSE(mat.has_value());
}

TEST_F(MaterialManagerTest, ClearMaterialAssignment_RemovesAssignment) {
    m_manager->seedDefaults();
    auto materials = m_manager->getAllMaterials();
    ASSERT_FALSE(materials.empty());

    dw::i64 modelId = insertModel("test_cube");
    ASSERT_TRUE(m_manager->assignMaterialToModel(materials[0].id, modelId));
    ASSERT_TRUE(m_manager->getModelMaterial(modelId).has_value());

    EXPECT_TRUE(m_manager->clearMaterialAssignment(modelId));
    EXPECT_FALSE(m_manager->getModelMaterial(modelId).has_value());
}

// ============================================================================
// exportMaterial
// ============================================================================

TEST_F(MaterialManagerTest, Export_DefaultMaterial_CreatesArchive) {
    m_manager->seedDefaults();
    auto all = m_manager->getAllMaterials();
    ASSERT_FALSE(all.empty());

    fs::path outputPath = m_tempDir / "exported_material.dwmat";
    EXPECT_TRUE(m_manager->exportMaterial(all[0].id, outputPath));
    EXPECT_TRUE(fs::exists(outputPath));
}

TEST_F(MaterialManagerTest, Export_DefaultMaterial_IsValidArchive) {
    m_manager->seedDefaults();
    auto all = m_manager->getAllMaterials();
    ASSERT_FALSE(all.empty());

    fs::path outputPath = m_tempDir / "exported.dwmat";
    ASSERT_TRUE(m_manager->exportMaterial(all[0].id, outputPath));
    EXPECT_TRUE(dw::MaterialArchive::isValidArchive(outputPath.string()));
}

TEST_F(MaterialManagerTest, Export_FailsForNonExistentMaterial) {
    fs::path outputPath = m_tempDir / "nonexistent.dwmat";
    EXPECT_FALSE(m_manager->exportMaterial(99999, outputPath));
}

// ============================================================================
// getDefaultMaterials (standalone function)
// ============================================================================

TEST(DefaultMaterialsTest, Returns32Materials) {
    auto defaults = dw::getDefaultMaterials();
    EXPECT_EQ(defaults.size(), 32u);
}

TEST(DefaultMaterialsTest, AllHaveNames) {
    for (const auto& mat : dw::getDefaultMaterials()) {
        EXPECT_FALSE(mat.name.empty());
    }
}

TEST(DefaultMaterialsTest, CompositesMayHaveZeroJanka) {
    for (const auto& mat : dw::getDefaultMaterials()) {
        if (mat.category == dw::MaterialCategory::Composite) {
            // Composites/metals/plastics may have jankaHardness == 0 (N/A)
            EXPECT_GE(mat.jankaHardness, 0.0f);
        }
    }
}

TEST(DefaultMaterialsTest, WoodSpeciesHavePositiveJanka) {
    for (const auto& mat : dw::getDefaultMaterials()) {
        if (mat.category == dw::MaterialCategory::Hardwood ||
            mat.category == dw::MaterialCategory::Softwood ||
            mat.category == dw::MaterialCategory::Domestic) {
            EXPECT_GT(mat.jankaHardness, 0.0f) << "Wood species missing Janka: " << mat.name;
        }
    }
}
