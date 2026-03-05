#include <gtest/gtest.h>
#include "core/optimizer/waste_breakdown.h"
#include "core/optimizer/cut_optimizer.h"

using namespace dw;
using namespace dw::optimizer;

namespace {

// Helper: create a CutPlan with a single placement on one sheet
CutPlan makeSinglePartPlan(f32 partW, f32 partH, f32 sheetW, f32 sheetH) {
    Part p(partW, partH, 1);

    SheetResult sr;
    sr.sheetIndex = 0;
    Placement pl;
    pl.part = &p; // Will be fixed below
    pl.partIndex = 0;
    pl.instanceIndex = 0;
    pl.x = 0.0f;
    pl.y = 0.0f;
    pl.rotated = false;
    sr.placements.push_back(pl);
    sr.usedArea = partW * partH;
    sr.wasteArea = (sheetW * sheetH) - (partW * partH);

    CutPlan plan;
    plan.sheets.push_back(sr);
    plan.totalUsedArea = sr.usedArea;
    plan.totalWasteArea = sr.wasteArea;
    plan.sheetsUsed = 1;
    plan.totalCost = 0.0f;
    return plan;
}

} // anonymous namespace

TEST(WasteBreakdown, SinglePartOnSheet) {
    // Use the real optimizer to get a valid CutPlan with correct part pointers
    auto opt = CutOptimizer::create(Algorithm::Guillotine);
    opt->setKerf(0.0f);
    opt->setMargin(0.0f);
    opt->setAllowRotation(false);

    std::vector<Part> parts = {Part(500.0f, 500.0f, 1)};
    Sheet sheet(1000.0f, 1000.0f, 0.0f);
    CutPlan plan = opt->optimize(parts, {sheet});

    ASSERT_EQ(plan.sheetsUsed, 1);

    WasteBreakdown wb = computeWasteBreakdown(plan, sheet, 0.0f);

    // Scrap pieces should exist (sheet area - part area = 750000 sq mm)
    EXPECT_FALSE(wb.scrapPieces.empty());
    EXPECT_GT(wb.totalScrapArea, 0.0f);
    EXPECT_FLOAT_EQ(wb.totalKerfArea, 0.0f);

    // Total scrap + used should equal sheet area
    f32 sheetArea = sheet.area();
    f32 usedArea = plan.totalUsedArea;
    EXPECT_NEAR(wb.totalScrapArea + wb.totalUnusableArea + usedArea, sheetArea, 1.0f);
}

TEST(WasteBreakdown, WithKerf) {
    auto opt = CutOptimizer::create(Algorithm::Guillotine);
    opt->setKerf(3.0f);
    opt->setMargin(0.0f);
    opt->setAllowRotation(false);

    std::vector<Part> parts = {Part(500.0f, 500.0f, 1)};
    Sheet sheet(1000.0f, 1000.0f, 0.0f);
    CutPlan plan = opt->optimize(parts, {sheet});

    ASSERT_EQ(plan.sheetsUsed, 1);

    WasteBreakdown wb = computeWasteBreakdown(plan, sheet, 3.0f);

    // With kerf, there should be kerf loss area
    EXPECT_GT(wb.totalKerfArea, 0.0f);
}

TEST(WasteBreakdown, DollarValues) {
    auto opt = CutOptimizer::create(Algorithm::Guillotine);
    opt->setKerf(0.0f);
    opt->setMargin(0.0f);
    opt->setAllowRotation(false);

    std::vector<Part> parts = {Part(500.0f, 500.0f, 1)};
    Sheet sheet(1000.0f, 1000.0f, 100.0f); // $100 sheet
    CutPlan plan = opt->optimize(parts, {sheet});

    ASSERT_EQ(plan.sheetsUsed, 1);

    WasteBreakdown wb = computeWasteBreakdown(plan, sheet, 0.0f);

    // Dollar values should be proportional to area ratios
    EXPECT_GT(wb.scrapValue, 0.0f);
    EXPECT_GT(wb.totalWasteValue, 0.0f);

    // scrapValue should be approximately (scrapArea / sheetArea) * cost
    f32 expectedRate = sheet.cost / sheet.area();
    EXPECT_NEAR(wb.scrapValue, wb.totalScrapArea * expectedRate, 0.01f);
}

TEST(WasteBreakdown, MultipleSheets) {
    // Construct a 2-sheet CutPlan directly to test waste breakdown across sheets
    // Each sheet 1000x1000, one 500x500 part placed on each
    static Part p1(500.0f, 500.0f, 1);
    static Part p2(400.0f, 400.0f, 1);

    CutPlan plan;

    // Sheet 0: one 500x500 part
    SheetResult sr0;
    sr0.sheetIndex = 0;
    Placement pl0;
    pl0.part = &p1;
    pl0.partIndex = 0;
    pl0.instanceIndex = 0;
    pl0.x = 0.0f;
    pl0.y = 0.0f;
    pl0.rotated = false;
    sr0.placements.push_back(pl0);
    sr0.usedArea = 500.0f * 500.0f;
    sr0.wasteArea = 1000.0f * 1000.0f - sr0.usedArea;
    plan.sheets.push_back(sr0);

    // Sheet 1: one 400x400 part
    SheetResult sr1;
    sr1.sheetIndex = 1;
    Placement pl1;
    pl1.part = &p2;
    pl1.partIndex = 1;
    pl1.instanceIndex = 0;
    pl1.x = 0.0f;
    pl1.y = 0.0f;
    pl1.rotated = false;
    sr1.placements.push_back(pl1);
    sr1.usedArea = 400.0f * 400.0f;
    sr1.wasteArea = 1000.0f * 1000.0f - sr1.usedArea;
    plan.sheets.push_back(sr1);

    plan.totalUsedArea = sr0.usedArea + sr1.usedArea;
    plan.totalWasteArea = sr0.wasteArea + sr1.wasteArea;
    plan.sheetsUsed = 2;
    plan.totalCost = 100.0f;

    Sheet sheet(1000.0f, 1000.0f, 50.0f);
    WasteBreakdown wb = computeWasteBreakdown(plan, sheet, 0.0f);

    // Scrap pieces from both sheets should be collected
    EXPECT_FALSE(wb.scrapPieces.empty());
    // Should have pieces from different sheet indices
    bool hasSheet0 = false, hasSheet1 = false;
    for (const auto& sp : wb.scrapPieces) {
        if (sp.sheetIndex == 0) hasSheet0 = true;
        if (sp.sheetIndex == 1) hasSheet1 = true;
    }
    EXPECT_TRUE(hasSheet0);
    EXPECT_TRUE(hasSheet1);
}

TEST(WasteBreakdown, EmptyPlan) {
    CutPlan plan; // Empty
    Sheet sheet(1000.0f, 1000.0f, 50.0f);

    WasteBreakdown wb = computeWasteBreakdown(plan, sheet, 3.0f);

    EXPECT_TRUE(wb.scrapPieces.empty());
    EXPECT_FLOAT_EQ(wb.totalScrapArea, 0.0f);
    EXPECT_FLOAT_EQ(wb.totalKerfArea, 0.0f);
    EXPECT_FLOAT_EQ(wb.totalUnusableArea, 0.0f);
    EXPECT_FLOAT_EQ(wb.scrapValue, 0.0f);
    EXPECT_FLOAT_EQ(wb.totalWasteValue, 0.0f);
}
