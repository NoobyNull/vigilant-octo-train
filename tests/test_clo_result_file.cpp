#include <gtest/gtest.h>
#include <filesystem>

#include "core/optimizer/clo_result_file.h"

using namespace dw;
using namespace dw::optimizer;

namespace {

// Helper: create a temp directory for tests
Path createTempDir() {
    auto tmp = std::filesystem::temp_directory_path() / "dw_test_clo";
    std::filesystem::create_directories(tmp);
    return tmp;
}

// Helper: build a minimal MultiStockResult with one group
MultiStockResult makeTestResult() {
    MultiStockResult result;

    MultiStockResult::GroupResult gr;
    gr.materialId = 1;
    gr.materialName = "Plywood";
    gr.usedSheet = Sheet(2440.0f, 1220.0f, 45.0f);
    gr.usedSheet.name = "4x8 Plywood";

    // Build a simple CutPlan with 2 sheets
    static Part testPart;
    testPart.id = 1;
    testPart.materialId = 1;
    testPart.name = "Side Panel";
    testPart.width = 600.0f;
    testPart.height = 400.0f;
    testPart.quantity = 1;
    testPart.canRotate = true;

    SheetResult sr;
    sr.sheetIndex = 0;
    Placement pl;
    pl.part = &testPart;
    pl.partIndex = 0;
    pl.instanceIndex = 0;
    pl.x = 5.0f;
    pl.y = 5.0f;
    pl.rotated = false;
    sr.placements.push_back(pl);
    sr.usedArea = 240000.0f;
    sr.wasteArea = 2440.0f * 1220.0f - 240000.0f;
    gr.plan.sheets.push_back(sr);

    SheetResult sr2;
    sr2.sheetIndex = 1;
    Placement pl2;
    pl2.part = &testPart;
    pl2.partIndex = 0;
    pl2.instanceIndex = 1;
    pl2.x = 10.0f;
    pl2.y = 10.0f;
    pl2.rotated = true;
    sr2.placements.push_back(pl2);
    sr2.usedArea = 240000.0f;
    sr2.wasteArea = 2440.0f * 1220.0f - 240000.0f;
    gr.plan.sheets.push_back(sr2);

    gr.plan.sheetsUsed = 2;
    gr.plan.totalUsedArea = 480000.0f;
    gr.plan.totalWasteArea = 2.0f * (2440.0f * 1220.0f) - 480000.0f;
    gr.plan.totalCost = 90.0f;

    // Waste breakdown
    gr.waste.totalScrapArea = 500000.0f;
    gr.waste.totalKerfArea = 3600.0f;
    gr.waste.totalUnusableArea = 100.0f;
    gr.waste.scrapValue = 15.0f;
    gr.waste.kerfValue = 1.08f;
    gr.waste.unusableValue = 0.03f;
    gr.waste.totalWasteValue = 16.11f;

    ScrapPiece sp1;
    sp1.x = 600.0f;
    sp1.y = 0.0f;
    sp1.width = 1840.0f;
    sp1.height = 1220.0f;
    sp1.sheetIndex = 0;
    gr.waste.scrapPieces.push_back(sp1);

    ScrapPiece sp2;
    sp2.x = 0.0f;
    sp2.y = 400.0f;
    sp2.width = 600.0f;
    sp2.height = 820.0f;
    sp2.sheetIndex = 0;
    gr.waste.scrapPieces.push_back(sp2);

    result.groups.push_back(gr);
    result.totalCost = 90.0f;
    result.totalSheetsUsed = 2;

    return result;
}

std::vector<Part> makeTestParts() {
    Part p;
    p.id = 1;
    p.materialId = 1;
    p.name = "Side Panel";
    p.width = 600.0f;
    p.height = 400.0f;
    p.quantity = 2;
    p.canRotate = true;
    return {p};
}

class CloResultFileTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_dir = createTempDir();
        m_file.setDirectory(m_dir);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(m_dir, ec);
    }

    Path m_dir;
    CloResultFile m_file;
};

} // anonymous namespace

TEST_F(CloResultFileTest, SaveAndLoad) {
    auto result = makeTestResult();
    auto parts = makeTestParts();

    ASSERT_TRUE(m_file.save("Test Plan", result, parts,
                            "guillotine", true, 3.0f, 5.0f, {42}));

    // Find the saved file
    auto listing = m_file.list();
    ASSERT_EQ(listing.size(), 1u);

    auto loaded = m_file.load(listing[0].filePath);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->name, "Test Plan");
    EXPECT_EQ(loaded->algorithm, "guillotine");
    EXPECT_EQ(loaded->allowRotation, true);
    EXPECT_NEAR(loaded->kerf, 3.0f, 0.01f);
    EXPECT_NEAR(loaded->margin, 5.0f, 0.01f);

    // Check result structure
    ASSERT_EQ(loaded->result.groups.size(), 1u);
    auto& g = loaded->result.groups[0];
    EXPECT_EQ(g.materialId, 1);
    EXPECT_EQ(g.materialName, "Plywood");
    EXPECT_EQ(g.plan.sheetsUsed, 2);
    EXPECT_NEAR(g.usedSheet.width, 2440.0f, 0.1f);
    EXPECT_NEAR(g.usedSheet.cost, 45.0f, 0.01f);

    // Verify placements
    ASSERT_EQ(g.plan.sheets.size(), 2u);
    ASSERT_EQ(g.plan.sheets[0].placements.size(), 1u);
    EXPECT_NEAR(g.plan.sheets[0].placements[0].x, 5.0f, 0.01f);
    EXPECT_EQ(g.plan.sheets[1].placements[0].rotated, true);

    // Verify totals
    EXPECT_NEAR(loaded->result.totalCost, 90.0f, 0.01f);
    EXPECT_EQ(loaded->result.totalSheetsUsed, 2);
}

TEST_F(CloResultFileTest, StockSizeIdsPreserved) {
    auto result = makeTestResult();
    auto parts = makeTestParts();

    ASSERT_TRUE(m_file.save("Stock IDs Test", result, parts,
                            "guillotine", true, 3.0f, 5.0f, {42}));

    auto listing = m_file.list();
    auto loaded = m_file.load(listing[0].filePath);
    ASSERT_TRUE(loaded.has_value());

    ASSERT_EQ(loaded->stockSizeIds.size(), 1u);
    EXPECT_EQ(loaded->stockSizeIds[0], 42);
}

TEST_F(CloResultFileTest, WasteBreakdownPreserved) {
    auto result = makeTestResult();
    auto parts = makeTestParts();

    ASSERT_TRUE(m_file.save("Waste Test", result, parts,
                            "guillotine", true, 3.0f, 5.0f, {42}));

    auto listing = m_file.list();
    auto loaded = m_file.load(listing[0].filePath);
    ASSERT_TRUE(loaded.has_value());

    auto& w = loaded->result.groups[0].waste;
    EXPECT_EQ(w.scrapPieces.size(), 2u);
    EXPECT_NEAR(w.totalScrapArea, 500000.0f, 1.0f);
    EXPECT_NEAR(w.totalKerfArea, 3600.0f, 1.0f);
    EXPECT_NEAR(w.totalUnusableArea, 100.0f, 1.0f);
    EXPECT_NEAR(w.scrapValue, 15.0f, 0.01f);
    EXPECT_NEAR(w.kerfValue, 1.08f, 0.01f);
    EXPECT_NEAR(w.unusableValue, 0.03f, 0.01f);
    EXPECT_NEAR(w.totalWasteValue, 16.11f, 0.01f);

    // Check scrap piece dimensions
    EXPECT_NEAR(w.scrapPieces[0].width, 1840.0f, 0.1f);
    EXPECT_NEAR(w.scrapPieces[0].height, 1220.0f, 0.1f);
    EXPECT_EQ(w.scrapPieces[0].sheetIndex, 0);
}

TEST_F(CloResultFileTest, PartsWithMaterialId) {
    auto result = makeTestResult();
    std::vector<Part> parts;
    Part p1;
    p1.id = 1;
    p1.materialId = 5;
    p1.name = "Part A";
    p1.width = 300.0f;
    p1.height = 200.0f;
    parts.push_back(p1);

    Part p2;
    p2.id = 2;
    p2.materialId = 8;
    p2.name = "Part B";
    p2.width = 400.0f;
    p2.height = 300.0f;
    parts.push_back(p2);

    ASSERT_TRUE(m_file.save("Material Parts", result, parts,
                            "guillotine", true, 3.0f, 5.0f, {42}));

    auto listing = m_file.list();
    auto loaded = m_file.load(listing[0].filePath);
    ASSERT_TRUE(loaded.has_value());

    ASSERT_EQ(loaded->parts.size(), 2u);
    EXPECT_EQ(loaded->parts[0].materialId, 5);
    EXPECT_EQ(loaded->parts[1].materialId, 8);
}

TEST_F(CloResultFileTest, ListResults) {
    auto result = makeTestResult();
    auto parts = makeTestParts();

    ASSERT_TRUE(m_file.save("Plan Alpha", result, parts,
                            "guillotine", true, 3.0f, 5.0f, {42}));
    ASSERT_TRUE(m_file.save("Plan Beta", result, parts,
                            "ffd", false, 2.0f, 3.0f, {17}));

    auto listing = m_file.list();
    ASSERT_EQ(listing.size(), 2u);

    // Both should have correct metadata
    bool foundAlpha = false, foundBeta = false;
    for (const auto& meta : listing) {
        if (meta.name == "Plan Alpha") {
            foundAlpha = true;
            EXPECT_EQ(meta.totalSheetsUsed, 2);
            EXPECT_NEAR(meta.totalCost, 90.0f, 0.01f);
            EXPECT_EQ(meta.groupCount, 1);
        }
        if (meta.name == "Plan Beta") {
            foundBeta = true;
        }
    }
    EXPECT_TRUE(foundAlpha);
    EXPECT_TRUE(foundBeta);
}

TEST_F(CloResultFileTest, EmptyResult) {
    MultiStockResult empty;
    std::vector<Part> parts;

    ASSERT_TRUE(m_file.save("Empty", empty, parts,
                            "guillotine", true, 3.0f, 5.0f, {}));

    auto listing = m_file.list();
    auto loaded = m_file.load(listing[0].filePath);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_TRUE(loaded->result.groups.empty());
    EXPECT_TRUE(loaded->stockSizeIds.empty());
}
