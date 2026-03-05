#include <gtest/gtest.h>
#include "core/utils/board_foot.h"
#include <cmath>

using namespace dw;

static constexpr double kEps = 1e-4;

// ============================================================
// calcBoardFeet
// ============================================================

TEST(BoardFoot, Standard4x4Board) {
    // 1" thick x 12" wide x 12" long = 1 BF
    // In mm: 25.4 x 304.8 x 304.8
    EXPECT_NEAR(calcBoardFeet(25.4, 304.8, 304.8), 1.0, kEps);
}

TEST(BoardFoot, FullSheet4x8at3_4) {
    // 3/4" thick x 4'x8' sheet = 32 BF
    // In mm: 19.05 x 1219.2 x 2438.4
    EXPECT_NEAR(calcBoardFeet(19.05, 1219.2, 2438.4), 32.0, kEps);
}

TEST(BoardFoot, ZeroWidth) {
    EXPECT_NEAR(calcBoardFeet(25.4, 0.0, 304.8), 0.0, kEps);
}

TEST(BoardFoot, ZeroHeight) {
    EXPECT_NEAR(calcBoardFeet(25.4, 304.8, 0.0), 0.0, kEps);
}

TEST(BoardFoot, ZeroThickness) {
    EXPECT_NEAR(calcBoardFeet(0.0, 304.8, 304.8), 0.0, kEps);
}

// ============================================================
// calcCostPerBoardFoot
// ============================================================

TEST(CostPerBoardFoot, SheetPricing) {
    // $50/sheet, 4x8 at 3/4" -> 32 BF -> $50/32 = $1.5625/BF
    EXPECT_NEAR(
        calcCostPerBoardFoot(50.0, "sheet", 19.05, 1219.2, 2438.4),
        1.5625, kEps);
}

TEST(CostPerBoardFoot, BoardFootPricing) {
    // $8.50/BF -> returns 8.50 directly
    EXPECT_NEAR(
        calcCostPerBoardFoot(8.50, "board foot", 25.4, 304.8, 304.8),
        8.50, kEps);
}

TEST(CostPerBoardFoot, BoardFootPricingIgnoresDimensions) {
    // "board foot" pricing returns price directly regardless of dims
    EXPECT_NEAR(
        calcCostPerBoardFoot(8.50, "board foot", 0.0, 0.0, 0.0),
        8.50, kEps);
}

TEST(CostPerBoardFoot, LinearFootPricing) {
    // $5/LF for a 1" thick x 6" wide board
    // BF per LF = (T_in * W_in) / 12 = (1 * 6) / 12 = 0.5
    // Cost/BF = $5 / 0.5 = $10/BF
    // Dimensions in mm: T=25.4, W=152.4, H=304.8 (1 foot for reference)
    EXPECT_NEAR(
        calcCostPerBoardFoot(5.0, "linear foot", 25.4, 152.4, 304.8),
        10.0, kEps);
}

TEST(CostPerBoardFoot, EachPricing) {
    // $25/each for 2"x2"x12" piece
    // BF = (2*2*12)/144 = 0.3333
    // Cost/BF = $25/0.3333 = $75/BF
    // In mm: T=50.8, W=50.8, H=304.8
    EXPECT_NEAR(
        calcCostPerBoardFoot(25.0, "each", 50.8, 50.8, 304.8),
        75.0, 0.5); // slightly relaxed for rounding
}

TEST(CostPerBoardFoot, SheetMissingDimensions) {
    // Sheet pricing with zero width -> can't calculate
    EXPECT_NEAR(
        calcCostPerBoardFoot(50.0, "sheet", 19.05, 0.0, 2438.4),
        0.0, kEps);
}

// ============================================================
// formatBoardFeet
// ============================================================

TEST(FormatBoardFeet, OneBF) {
    EXPECT_EQ(formatBoardFeet(1.0), "1.00 BF");
}

TEST(FormatBoardFeet, ThirtyTwoBF) {
    EXPECT_EQ(formatBoardFeet(32.0), "32.00 BF");
}

TEST(FormatBoardFeet, ZeroBF) {
    EXPECT_EQ(formatBoardFeet(0.0), "-- BF");
}

// ============================================================
// formatCostPerBoardFoot
// ============================================================

TEST(FormatCostPerBoardFoot, Normal) {
    EXPECT_EQ(formatCostPerBoardFoot(8.50), "$8.50/BF");
}

TEST(FormatCostPerBoardFoot, Zero) {
    EXPECT_EQ(formatCostPerBoardFoot(0.0), "--");
}
