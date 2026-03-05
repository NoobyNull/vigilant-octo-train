#include <gtest/gtest.h>
#include "core/utils/unit_conversion.h"
#include <cmath>

using namespace dw;

// Helper for floating-point comparison
static constexpr double kEps = 1e-6;

// ============================================================
// Basic conversions
// ============================================================

TEST(UnitConversion, MmToInches) {
    EXPECT_NEAR(mmToInches(25.4), 1.0, kEps);
    EXPECT_NEAR(mmToInches(0.0), 0.0, kEps);
    EXPECT_NEAR(mmToInches(304.8), 12.0, kEps);
}

TEST(UnitConversion, InchesToMm) {
    EXPECT_NEAR(inchesToMm(1.0), 25.4, kEps);
    EXPECT_NEAR(inchesToMm(0.0), 0.0, kEps);
    EXPECT_NEAR(inchesToMm(12.0), 304.8, kEps);
}

TEST(UnitConversion, FeetToMm) {
    EXPECT_NEAR(feetToMm(1.0), 304.8, kEps);
    EXPECT_NEAR(feetToMm(0.0), 0.0, kEps);
}

TEST(UnitConversion, CmToMm) {
    EXPECT_NEAR(cmToMm(1.0), 10.0, kEps);
    EXPECT_NEAR(cmToMm(0.0), 0.0, kEps);
}

TEST(UnitConversion, MToMm) {
    EXPECT_NEAR(mToMm(1.0), 1000.0, kEps);
    EXPECT_NEAR(mToMm(0.0), 0.0, kEps);
}

// ============================================================
// parseDimension — bare numbers
// ============================================================

TEST(ParseDimension, BareNumberMetric) {
    auto result = parseDimension("25.4", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 25.4, kEps);
}

TEST(ParseDimension, BareNumberSAE) {
    // Bare number in SAE mode = inches -> mm
    auto result = parseDimension("1", false);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 25.4, kEps);
}

// ============================================================
// parseDimension — suffixed numbers
// ============================================================

TEST(ParseDimension, SuffixedInchesInMetric) {
    auto result = parseDimension("33\"", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 838.2, kEps);
}

TEST(ParseDimension, SuffixedMetersInSAE) {
    auto result = parseDimension("3.3M", false);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 3300.0, kEps);
}

TEST(ParseDimension, SuffixedFeet) {
    auto result = parseDimension("1'", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 304.8, kEps);
}

TEST(ParseDimension, SuffixedCm) {
    auto result = parseDimension("2cm", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 20.0, kEps);
}

TEST(ParseDimension, SuffixedMm) {
    auto result = parseDimension("500mm", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 500.0, kEps);
}

TEST(ParseDimension, CaseInsensitiveLowerM) {
    auto result = parseDimension("3.3m", false);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 3300.0, kEps);
}

TEST(ParseDimension, CaseInsensitiveUpperMM) {
    auto result = parseDimension("10MM", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 10.0, kEps);
}

// ============================================================
// parseDimension — whitespace tolerance
// ============================================================

TEST(ParseDimension, WhitespaceBeforeSuffix) {
    auto result = parseDimension("33 \"", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 838.2, kEps);
}

TEST(ParseDimension, LeadingTrailingWhitespace) {
    auto result = parseDimension(" 25.4 mm ", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(*result, 25.4, kEps);
}

// ============================================================
// parseDimension — invalid input
// ============================================================

TEST(ParseDimension, InvalidLetters) {
    EXPECT_FALSE(parseDimension("abc", true).has_value());
}

TEST(ParseDimension, EmptyString) {
    EXPECT_FALSE(parseDimension("", true).has_value());
}

TEST(ParseDimension, DoubleDash) {
    EXPECT_FALSE(parseDimension("--5", true).has_value());
}

// ============================================================
// formatDimension — metric
// ============================================================

TEST(FormatDimension, MetricBasic) {
    EXPECT_EQ(formatDimension(25.4, true), "25.4 mm");
}

// ============================================================
// formatDimension — SAE
// ============================================================

TEST(FormatDimension, SAEUnder12Inches) {
    // 25.4mm = 1" — under 12 inches, show as inches
    EXPECT_EQ(formatDimension(25.4, false), "1\"");
}

TEST(FormatDimension, SAE18Inches) {
    // 457.2mm = 18" = 1'6"
    EXPECT_EQ(formatDimension(457.2, false), "1'6\"");
}

TEST(FormatDimension, SAEExactFeet) {
    // 304.8mm = 12" = 1'
    EXPECT_EQ(formatDimension(304.8, false), "1'");
}

TEST(FormatDimension, SAEFractionalInches) {
    // 38.1mm = 1.5" — under 12 inches, show as decimal inches
    EXPECT_EQ(formatDimension(38.1, false), "1.5\"");
}

// ============================================================
// formatDimensionCompact — metric
// ============================================================

TEST(FormatDimensionCompact, MetricWholeNumber) {
    EXPECT_EQ(formatDimensionCompact(1220.0, true), "1220");
}

TEST(FormatDimensionCompact, MetricDecimal) {
    EXPECT_EQ(formatDimensionCompact(25.4, true), "25.4");
}

// ============================================================
// formatDimensionCompact — SAE
// ============================================================

TEST(FormatDimensionCompact, SAEFeet) {
    // 2438.4mm = 96" = 8'
    EXPECT_EQ(formatDimensionCompact(2438.4, false), "8'");
}

TEST(FormatDimensionCompact, SAE4Feet) {
    // 1219.2mm = 48" = 4'
    EXPECT_EQ(formatDimensionCompact(1219.2, false), "4'");
}
