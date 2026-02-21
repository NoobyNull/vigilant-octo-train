// Digital Workshop - Material Repository Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/material_repository.h"
#include "core/database/schema.h"

namespace {

// Fixture: in-memory DB with schema initialized
class MaterialRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::MaterialRepository>(m_db);
    }

    dw::MaterialRecord makeMaterial(const std::string& name, dw::MaterialCategory category) {
        dw::MaterialRecord rec;
        rec.name = name;
        rec.category = category;
        rec.archivePath = "/materials/" + name;
        rec.jankaHardness = 1000.0f;
        rec.feedRate = 100.0f;
        rec.spindleSpeed = 18000.0f;
        rec.depthOfCut = 0.125f;
        rec.costPerBoardFoot = 5.50f;
        rec.grainDirectionDeg = 45.0f;
        rec.thumbnailPath = "/thumbnails/" + name + ".png";
        return rec;
    }

    dw::Database m_db;
    std::unique_ptr<dw::MaterialRepository> m_repo;
};

} // namespace

// --- Insert ---

TEST_F(MaterialRepoTest, Insert_ReturnsId) {
    auto rec = makeMaterial("Oak", dw::MaterialCategory::Hardwood);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}

TEST_F(MaterialRepoTest, Insert_MultipleMaterials) {
    auto oak = makeMaterial("Oak", dw::MaterialCategory::Hardwood);
    auto pine = makeMaterial("Pine", dw::MaterialCategory::Softwood);

    auto id1 = m_repo->insert(oak);
    auto id2 = m_repo->insert(pine);

    EXPECT_TRUE(id1.has_value());
    EXPECT_TRUE(id2.has_value());
    EXPECT_NE(id1.value(), id2.value());
}

// --- FindById ---

TEST_F(MaterialRepoTest, FindById_Found) {
    auto rec = makeMaterial("Walnut", dw::MaterialCategory::Hardwood);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Walnut");
    EXPECT_EQ(found->category, dw::MaterialCategory::Hardwood);
    EXPECT_FLOAT_EQ(found->jankaHardness, 1000.0f);
    EXPECT_FLOAT_EQ(found->feedRate, 100.0f);
    EXPECT_FLOAT_EQ(found->spindleSpeed, 18000.0f);
    EXPECT_FLOAT_EQ(found->depthOfCut, 0.125f);
    EXPECT_FLOAT_EQ(found->costPerBoardFoot, 5.50f);
    EXPECT_FLOAT_EQ(found->grainDirectionDeg, 45.0f);
}

TEST_F(MaterialRepoTest, FindById_NotFound) {
    auto found = m_repo->findById(999);
    EXPECT_FALSE(found.has_value());
}

// --- FindAll ---

TEST_F(MaterialRepoTest, FindAll_Empty) {
    auto all = m_repo->findAll();
    EXPECT_TRUE(all.empty());
}

TEST_F(MaterialRepoTest, FindAll_OrderedByName) {
    m_repo->insert(makeMaterial("Walnut", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Cherry", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Pine", dw::MaterialCategory::Softwood));

    auto all = m_repo->findAll();
    ASSERT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].name, "Cherry");  // Alphabetical order
    EXPECT_EQ(all[1].name, "Pine");
    EXPECT_EQ(all[2].name, "Walnut");
}

// --- FindByCategory ---

TEST_F(MaterialRepoTest, FindByCategory_Hardwood) {
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Maple", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Pine", dw::MaterialCategory::Softwood));

    auto hardwoods = m_repo->findByCategory(dw::MaterialCategory::Hardwood);
    ASSERT_EQ(hardwoods.size(), 2);
    EXPECT_EQ(hardwoods[0].name, "Maple");  // Alphabetical
    EXPECT_EQ(hardwoods[1].name, "Oak");
}

TEST_F(MaterialRepoTest, FindByCategory_Softwood) {
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Cedar", dw::MaterialCategory::Softwood));
    m_repo->insert(makeMaterial("Fir", dw::MaterialCategory::Softwood));

    auto softwoods = m_repo->findByCategory(dw::MaterialCategory::Softwood);
    ASSERT_EQ(softwoods.size(), 2);
    EXPECT_EQ(softwoods[0].name, "Cedar");
    EXPECT_EQ(softwoods[1].name, "Fir");
}

TEST_F(MaterialRepoTest, FindByCategory_Composite) {
    m_repo->insert(makeMaterial("MDF", dw::MaterialCategory::Composite));
    m_repo->insert(makeMaterial("Plywood", dw::MaterialCategory::Composite));
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));

    auto composites = m_repo->findByCategory(dw::MaterialCategory::Composite);
    ASSERT_EQ(composites.size(), 2);
}

TEST_F(MaterialRepoTest, FindByCategory_NoneFound) {
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));

    auto domestics = m_repo->findByCategory(dw::MaterialCategory::Domestic);
    EXPECT_TRUE(domestics.empty());
}

// --- FindByName ---

TEST_F(MaterialRepoTest, FindByName_ExactMatch) {
    m_repo->insert(makeMaterial("Red Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("White Oak", dw::MaterialCategory::Hardwood));

    auto results = m_repo->findByName("Red Oak");
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "Red Oak");
}

TEST_F(MaterialRepoTest, FindByName_PartialMatch) {
    m_repo->insert(makeMaterial("Red Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("White Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Maple", dw::MaterialCategory::Hardwood));

    auto results = m_repo->findByName("Oak");
    ASSERT_EQ(results.size(), 2);
}

TEST_F(MaterialRepoTest, FindByName_CaseInsensitive) {
    m_repo->insert(makeMaterial("Red Oak", dw::MaterialCategory::Hardwood));

    // SQLite LIKE is case-insensitive by default
    auto results = m_repo->findByName("red oak");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(MaterialRepoTest, FindByName_NotFound) {
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));

    auto results = m_repo->findByName("Teak");
    EXPECT_TRUE(results.empty());
}

// --- Update ---

TEST_F(MaterialRepoTest, Update_Success) {
    auto rec = makeMaterial("Oak", dw::MaterialCategory::Hardwood);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());

    found->name = "Red Oak";
    found->jankaHardness = 1290.0f;
    found->costPerBoardFoot = 6.75f;

    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Red Oak");
    EXPECT_FLOAT_EQ(updated->jankaHardness, 1290.0f);
    EXPECT_FLOAT_EQ(updated->costPerBoardFoot, 6.75f);
}

TEST_F(MaterialRepoTest, Update_CategoryChange) {
    auto rec = makeMaterial("Unknown Wood", dw::MaterialCategory::Domestic);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());

    found->category = dw::MaterialCategory::Hardwood;
    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->category, dw::MaterialCategory::Hardwood);
}

// --- Remove ---

TEST_F(MaterialRepoTest, Remove_Success) {
    auto rec = makeMaterial("Pine", dw::MaterialCategory::Softwood);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_FALSE(m_repo->findById(id.value()).has_value());
}

TEST_F(MaterialRepoTest, Remove_NonExistent) {
    EXPECT_FALSE(m_repo->remove(999));
}

// --- Count ---

TEST_F(MaterialRepoTest, Count_Empty) {
    EXPECT_EQ(m_repo->count(), 0);
}

TEST_F(MaterialRepoTest, Count_Multiple) {
    m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));
    m_repo->insert(makeMaterial("Pine", dw::MaterialCategory::Softwood));
    m_repo->insert(makeMaterial("MDF", dw::MaterialCategory::Composite));

    EXPECT_EQ(m_repo->count(), 3);
}

TEST_F(MaterialRepoTest, Count_AfterRemove) {
    auto id1 = m_repo->insert(makeMaterial("Oak", dw::MaterialCategory::Hardwood));
    auto id2 = m_repo->insert(makeMaterial("Pine", dw::MaterialCategory::Softwood));

    EXPECT_EQ(m_repo->count(), 2);

    m_repo->remove(id1.value());
    EXPECT_EQ(m_repo->count(), 1);

    m_repo->remove(id2.value());
    EXPECT_EQ(m_repo->count(), 0);
}
