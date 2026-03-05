// Digital Workshop - StockSize Repository Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/material_repository.h"
#include "core/database/schema.h"
#include "core/database/stock_size_repository.h"

namespace {

// Fixture: in-memory DB with schema initialized, one material for FK
class StockSizeRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::StockSizeRepository>(m_db);
        m_materialRepo = std::make_unique<dw::MaterialRepository>(m_db);

        // Create a material for FK references
        dw::MaterialRecord mat;
        mat.name = "Test Oak";
        mat.category = dw::MaterialCategory::Hardwood;
        auto id = m_materialRepo->insert(mat);
        ASSERT_TRUE(id.has_value());
        m_materialId = id.value();

        // Create a second material
        dw::MaterialRecord mat2;
        mat2.name = "Test Pine";
        mat2.category = dw::MaterialCategory::Softwood;
        auto id2 = m_materialRepo->insert(mat2);
        ASSERT_TRUE(id2.has_value());
        m_materialId2 = id2.value();
    }

    dw::StockSize makeSize(const std::string& name,
                           dw::i64 matId,
                           dw::f64 w = 1220.0,
                           dw::f64 h = 2440.0,
                           dw::f64 t = 19.0) {
        dw::StockSize s;
        s.materialId = matId;
        s.name = name;
        s.widthMm = w;
        s.heightMm = h;
        s.thicknessMm = t;
        s.pricePerUnit = 45.0;
        s.unitLabel = "sheet";
        return s;
    }

    dw::Database m_db;
    std::unique_ptr<dw::StockSizeRepository> m_repo;
    std::unique_ptr<dw::MaterialRepository> m_materialRepo;
    dw::i64 m_materialId = 0;
    dw::i64 m_materialId2 = 0;
};

} // namespace

// --- Insert ---

TEST_F(StockSizeRepoTest, Insert_ReturnsId) {
    auto s = makeSize("4x8 Sheet", m_materialId);
    auto id = m_repo->insert(s);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}

TEST_F(StockSizeRepoTest, Insert_WithAllFields) {
    dw::StockSize s;
    s.materialId = m_materialId;
    s.name = "Custom Plywood Sheet";
    s.widthMm = 1220.0;
    s.heightMm = 2440.0;
    s.thicknessMm = 18.0;
    s.pricePerUnit = 52.99;
    s.unitLabel = "sheet";

    auto id = m_repo->insert(s);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Custom Plywood Sheet");
    EXPECT_DOUBLE_EQ(found->widthMm, 1220.0);
    EXPECT_DOUBLE_EQ(found->heightMm, 2440.0);
    EXPECT_DOUBLE_EQ(found->thicknessMm, 18.0);
    EXPECT_DOUBLE_EQ(found->pricePerUnit, 52.99);
    EXPECT_EQ(found->unitLabel, "sheet");
    EXPECT_EQ(found->materialId, m_materialId);
}

TEST_F(StockSizeRepoTest, Insert_WithNullableWidthHeight) {
    // Rate-card style lumber: thickness + price only, no width/height
    dw::StockSize s;
    s.materialId = m_materialId;
    s.name = "4/4 Oak Lumber";
    s.widthMm = 0.0;   // Will be stored as NULL
    s.heightMm = 0.0;   // Will be stored as NULL
    s.thicknessMm = 25.4; // ~1 inch
    s.pricePerUnit = 8.50;
    s.unitLabel = "board foot";

    auto id = m_repo->insert(s);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(found->widthMm, 0.0);
    EXPECT_DOUBLE_EQ(found->heightMm, 0.0);
    EXPECT_DOUBLE_EQ(found->thicknessMm, 25.4);
    EXPECT_EQ(found->unitLabel, "board foot");
}

// --- FindById ---

TEST_F(StockSizeRepoTest, FindById_Found) {
    auto s = makeSize("FindMe", m_materialId);
    auto id = m_repo->insert(s);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "FindMe");
    EXPECT_EQ(found->materialId, m_materialId);
}

TEST_F(StockSizeRepoTest, FindById_NotFound) {
    auto found = m_repo->findById(999);
    EXPECT_FALSE(found.has_value());
}

// --- FindByMaterial ---

TEST_F(StockSizeRepoTest, FindByMaterial_ReturnsOnlyMatchingMaterial) {
    m_repo->insert(makeSize("Oak Sheet A", m_materialId));
    m_repo->insert(makeSize("Oak Sheet B", m_materialId));
    m_repo->insert(makeSize("Pine Sheet", m_materialId2));

    auto oakSizes = m_repo->findByMaterial(m_materialId);
    ASSERT_EQ(oakSizes.size(), 2u);
    for (const auto& s : oakSizes) {
        EXPECT_EQ(s.materialId, m_materialId);
    }

    auto pineSizes = m_repo->findByMaterial(m_materialId2);
    ASSERT_EQ(pineSizes.size(), 1u);
    EXPECT_EQ(pineSizes[0].name, "Pine Sheet");
}

TEST_F(StockSizeRepoTest, FindByMaterial_Empty) {
    auto results = m_repo->findByMaterial(999);
    EXPECT_TRUE(results.empty());
}

// --- FindAll ---

TEST_F(StockSizeRepoTest, FindAll_Multiple) {
    m_repo->insert(makeSize("Sheet A", m_materialId));
    m_repo->insert(makeSize("Sheet B", m_materialId));
    m_repo->insert(makeSize("Board C", m_materialId2));

    auto all = m_repo->findAll();
    EXPECT_EQ(all.size(), 3u);
}

// --- Update ---

TEST_F(StockSizeRepoTest, Update_ChangesFields) {
    auto s = makeSize("Original", m_materialId);
    auto id = m_repo->insert(s);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());

    found->name = "Updated Name";
    found->pricePerUnit = 99.99;
    found->thicknessMm = 25.0;
    found->unitLabel = "board foot";

    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Name");
    EXPECT_DOUBLE_EQ(updated->pricePerUnit, 99.99);
    EXPECT_DOUBLE_EQ(updated->thicknessMm, 25.0);
    EXPECT_EQ(updated->unitLabel, "board foot");
}

// --- Remove ---

TEST_F(StockSizeRepoTest, Remove_ById) {
    auto id = m_repo->insert(makeSize("To Remove", m_materialId));
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(m_repo->count(), 1);

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_EQ(m_repo->count(), 0);
}

TEST_F(StockSizeRepoTest, RemoveByMaterial_DeletesAllForMaterial) {
    m_repo->insert(makeSize("Oak A", m_materialId));
    m_repo->insert(makeSize("Oak B", m_materialId));
    m_repo->insert(makeSize("Pine A", m_materialId2));

    EXPECT_EQ(m_repo->count(), 3);
    EXPECT_TRUE(m_repo->removeByMaterial(m_materialId));
    EXPECT_EQ(m_repo->count(), 1);

    auto remaining = m_repo->findAll();
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0].materialId, m_materialId2);
}

// --- Count ---

TEST_F(StockSizeRepoTest, Count) {
    EXPECT_EQ(m_repo->count(), 0);
    m_repo->insert(makeSize("A", m_materialId));
    EXPECT_EQ(m_repo->count(), 1);
    m_repo->insert(makeSize("B", m_materialId));
    EXPECT_EQ(m_repo->count(), 2);
}

TEST_F(StockSizeRepoTest, CountByMaterial) {
    m_repo->insert(makeSize("Oak A", m_materialId));
    m_repo->insert(makeSize("Oak B", m_materialId));
    m_repo->insert(makeSize("Pine A", m_materialId2));

    EXPECT_EQ(m_repo->countByMaterial(m_materialId), 2);
    EXPECT_EQ(m_repo->countByMaterial(m_materialId2), 1);
    EXPECT_EQ(m_repo->countByMaterial(999), 0);
}
