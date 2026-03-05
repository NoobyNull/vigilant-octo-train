#include <gtest/gtest.h>

#include "core/project/costing_engine.h"
#include "core/database/rate_category_repository.h"

using namespace dw;

class CostingEngineTest : public ::testing::Test {
  protected:
    CostingEngine engine;
};

TEST_F(CostingEngineTest, AddEntry_AssignsId) {
    CostingEntry e;
    e.name = "Test item";
    e.category = CostCat::Material;
    engine.addEntry(e);

    const auto& entries = engine.entries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_FALSE(entries[0].id.empty()) << "Engine should assign a non-empty ID";
}

TEST_F(CostingEngineTest, RemoveEntry_ById) {
    CostingEntry e1;
    e1.name = "Item A";
    engine.addEntry(e1);

    CostingEntry e2;
    e2.name = "Item B";
    engine.addEntry(e2);

    ASSERT_EQ(engine.entries().size(), 2u);

    // Remove the first entry by its assigned ID
    std::string firstId = engine.entries()[0].id;
    engine.removeEntry(firstId);

    ASSERT_EQ(engine.entries().size(), 1u);
    EXPECT_EQ(engine.entries()[0].name, "Item B");
}

TEST_F(CostingEngineTest, FindEntry_ById) {
    CostingEntry e;
    e.name = "Findable";
    e.category = CostCat::Labor;
    engine.addEntry(e);

    std::string id = engine.entries()[0].id;
    CostingEntry* found = engine.findEntry(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Findable");
}

TEST_F(CostingEngineTest, CreateMaterialEntry_SnapshotsPrice) {
    auto entry = CostingEngine::createMaterialEntry(
        "Plywood 4x8", 42, "1220x2440x19mm", 45.0, 2.0, "sheet");

    EXPECT_EQ(entry.category, CostCat::Material);
    EXPECT_EQ(entry.snapshot.dbId, 42);
    EXPECT_DOUBLE_EQ(entry.snapshot.priceAtCapture, 45.0);
    EXPECT_DOUBLE_EQ(entry.estimatedTotal, 90.0);  // 2 * 45
    EXPECT_EQ(entry.snapshot.dimensions, "1220x2440x19mm");
}

TEST_F(CostingEngineTest, CreateMaterialEntry_CategoryIsCorrect) {
    auto entry = CostingEngine::createMaterialEntry(
        "MDF", 1, "600x400x12mm", 15.0, 1.0, "sheet");

    EXPECT_STREQ(entry.category.c_str(), "material");
}

TEST_F(CostingEngineTest, CreateLaborEntry) {
    auto entry = CostingEngine::createLaborEntry("Assembly", 25.0, 3.5);

    EXPECT_EQ(entry.category, CostCat::Labor);
    EXPECT_EQ(entry.unit, "hour");
    EXPECT_DOUBLE_EQ(entry.estimatedTotal, 87.50);
    EXPECT_DOUBLE_EQ(entry.quantity, 3.5);
    EXPECT_DOUBLE_EQ(entry.unitRate, 25.0);
}

TEST_F(CostingEngineTest, CreateOverheadEntry) {
    auto entry = CostingEngine::createOverheadEntry("Shop rent", 50.0, "Monthly allocation");

    EXPECT_EQ(entry.category, CostCat::Overhead);
    EXPECT_DOUBLE_EQ(entry.estimatedTotal, 50.0);
    EXPECT_EQ(entry.notes, "Monthly allocation");
}

TEST_F(CostingEngineTest, Recalculate_UpdatesEstimatedTotals) {
    CostingEntry e;
    e.name = "Widget";
    e.category = CostCat::Material;
    e.quantity = 3.0;
    e.unitRate = 10.0;
    e.estimatedTotal = 0.0;  // Not yet calculated
    e.id = "manual-id";
    engine.setEntries({e});

    engine.recalculate();

    EXPECT_DOUBLE_EQ(engine.entries()[0].estimatedTotal, 30.0);
}

TEST_F(CostingEngineTest, TotalEstimated_SumsAllEntries) {
    auto mat = CostingEngine::createMaterialEntry("Plywood", 1, "4x8", 40.0, 2.0, "sheet");
    auto labor = CostingEngine::createLaborEntry("Assembly", 20.0, 2.0);
    auto overhead = CostingEngine::createOverheadEntry("Shipping", 15.0);

    engine.addEntry(mat);
    engine.addEntry(labor);
    engine.addEntry(overhead);

    // 80 + 40 + 15 = 135
    EXPECT_DOUBLE_EQ(engine.totalEstimated(), 135.0);
}

TEST_F(CostingEngineTest, TotalActual_SumsActuals) {
    CostingEntry e1;
    e1.name = "Item A";
    e1.actualTotal = 50.0;
    e1.id = "a";
    CostingEntry e2;
    e2.name = "Item B";
    e2.actualTotal = 30.0;
    e2.id = "b";

    engine.setEntries({e1, e2});

    EXPECT_DOUBLE_EQ(engine.totalActual(), 80.0);
}

TEST_F(CostingEngineTest, Variance_ActualMinusEstimated) {
    CostingEntry e;
    e.name = "Widget";
    e.estimatedTotal = 100.0;
    e.actualTotal = 120.0;
    e.id = "v";
    engine.setEntries({e});

    EXPECT_DOUBLE_EQ(engine.variance(), 20.0);  // 120 - 100
}

TEST_F(CostingEngineTest, CategoryTotals_GroupsByCategory) {
    auto mat = CostingEngine::createMaterialEntry("Wood", 1, "4x8", 50.0, 1.0, "sheet");
    auto labor = CostingEngine::createLaborEntry("Finishing", 30.0, 2.0);
    auto overhead = CostingEngine::createOverheadEntry("Delivery", 25.0);

    engine.addEntry(mat);
    engine.addEntry(labor);
    engine.addEntry(overhead);

    auto totals = engine.categoryTotals();

    // Should have 3 categories
    ASSERT_EQ(totals.size(), 3u);

    // Find material total
    bool foundMaterial = false;
    bool foundLabor = false;
    bool foundOverhead = false;
    for (const auto& ct : totals) {
        if (ct.category == CostCat::Material) {
            EXPECT_DOUBLE_EQ(ct.estimatedTotal, 50.0);
            EXPECT_EQ(ct.entryCount, 1);
            foundMaterial = true;
        } else if (ct.category == CostCat::Labor) {
            EXPECT_DOUBLE_EQ(ct.estimatedTotal, 60.0);
            EXPECT_EQ(ct.entryCount, 1);
            foundLabor = true;
        } else if (ct.category == CostCat::Overhead) {
            EXPECT_DOUBLE_EQ(ct.estimatedTotal, 25.0);
            EXPECT_EQ(ct.entryCount, 1);
            foundOverhead = true;
        }
    }
    EXPECT_TRUE(foundMaterial);
    EXPECT_TRUE(foundLabor);
    EXPECT_TRUE(foundOverhead);
}

TEST_F(CostingEngineTest, ApplyRateCategories_GeneratesEntries) {
    RateCategory rc1;
    rc1.id = 1;
    rc1.name = "Finishing supplies";
    rc1.ratePerCuUnit = 2.0;
    rc1.notes = "Sandpaper, stain";

    RateCategory rc2;
    rc2.id = 2;
    rc2.name = "Router bits";
    rc2.ratePerCuUnit = 1.5;
    rc2.notes = "Wear cost";

    engine.applyRateCategories({rc1, rc2}, 10.0);

    const auto& entries = engine.entries();
    ASSERT_EQ(entries.size(), 2u);

    // Check first rate entry
    bool foundFinishing = false;
    bool foundRouter = false;
    for (const auto& e : entries) {
        if (e.name == "Finishing supplies") {
            EXPECT_DOUBLE_EQ(e.estimatedTotal, 20.0);  // 2.0 * 10
            EXPECT_EQ(e.category, CostCat::Consumable);
            foundFinishing = true;
        } else if (e.name == "Router bits") {
            EXPECT_DOUBLE_EQ(e.estimatedTotal, 15.0);  // 1.5 * 10
            foundRouter = true;
        }
    }
    EXPECT_TRUE(foundFinishing);
    EXPECT_TRUE(foundRouter);
}

TEST_F(CostingEngineTest, ApplyRateCategories_ReplacesExisting) {
    RateCategory rc1;
    rc1.id = 1;
    rc1.name = "Finishing";
    rc1.ratePerCuUnit = 2.0;
    rc1.notes = "";

    // Apply once
    engine.applyRateCategories({rc1}, 10.0);
    ASSERT_EQ(engine.entries().size(), 1u);

    // Apply again with different rate
    RateCategory rc2;
    rc2.id = 1;
    rc2.name = "Finishing";
    rc2.ratePerCuUnit = 3.0;
    rc2.notes = "";

    engine.applyRateCategories({rc2}, 10.0);

    // Should still be 1 entry, not 2
    ASSERT_EQ(engine.entries().size(), 1u);
    EXPECT_DOUBLE_EQ(engine.entries()[0].estimatedTotal, 30.0);  // 3.0 * 10
}
