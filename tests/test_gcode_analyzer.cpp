// Digital Workshop - G-code Analyzer Tests

#include <gtest/gtest.h>

#include "core/gcode/gcode_analyzer.h"
#include "core/gcode/gcode_parser.h"

#include <cmath>

TEST(GcodeAnalyzer, EmptyProgram) {
    dw::gcode::Program program;
    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_EQ(stats.commandCount, 0);
    EXPECT_EQ(stats.lineCount, 0);
    EXPECT_FLOAT_EQ(stats.totalPathLength, 0.0f);
    EXPECT_FLOAT_EQ(stats.cuttingPathLength, 0.0f);
    EXPECT_FLOAT_EQ(stats.rapidPathLength, 0.0f);
    EXPECT_FLOAT_EQ(stats.estimatedTime, 0.0f);
    EXPECT_EQ(stats.toolChangeCount, 0);
}

TEST(GcodeAnalyzer, SingleRapidMove) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10 Y0 Z0\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_EQ(stats.commandCount, 1);
    EXPECT_NEAR(stats.totalPathLength, 10.0f, 0.01f);
    EXPECT_NEAR(stats.rapidPathLength, 10.0f, 0.01f);
    EXPECT_FLOAT_EQ(stats.cuttingPathLength, 0.0f);
}

TEST(GcodeAnalyzer, SingleCuttingMove) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X0 Y10 F1000\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_EQ(stats.commandCount, 1);
    EXPECT_NEAR(stats.totalPathLength, 10.0f, 0.01f);
    EXPECT_NEAR(stats.cuttingPathLength, 10.0f, 0.01f);
    EXPECT_FLOAT_EQ(stats.rapidPathLength, 0.0f);
}

TEST(GcodeAnalyzer, DiagonalMove_Length) {
    // 3-4-5 triangle: move from origin to (3,4,0)
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X3 Y4 F1000\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_NEAR(stats.totalPathLength, 5.0f, 0.01f);
}

TEST(GcodeAnalyzer, MixedRapidAndCutting) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10\n"
                                "G1 X20 F1000\n"
                                "G0 X30\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_EQ(stats.commandCount, 3);
    EXPECT_NEAR(stats.totalPathLength, 30.0f, 0.01f);
    EXPECT_NEAR(stats.rapidPathLength, 20.0f, 0.01f);   // G0: 10 + 10
    EXPECT_NEAR(stats.cuttingPathLength, 10.0f, 0.01f); // G1: 10
}

TEST(GcodeAnalyzer, EstimatedTime) {
    // G1 X1000 F1000 — trapezoidal time is slightly longer than naive (1.0 min)
    // because of accel/decel phases, but close for a long move
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X1000 F1000\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    // Must be >= naive time of 1.0 min
    EXPECT_GE(stats.estimatedTime, 1.0f);
    // But within 5% for a long move
    EXPECT_LT(stats.estimatedTime, 1.05f);
}

TEST(GcodeAnalyzer, EstimatedTime_DefaultFeedRate) {
    // G1 without F uses default feed rate (500 mm/min set below)
    // Trapezoidal time >= naive time of 0.2 min
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X100\n");

    dw::gcode::Analyzer analyzer;
    analyzer.setDefaultFeedRate(500.0f);
    auto stats = analyzer.analyze(program);

    // 100mm at 500mm/min naive = 0.2 min; trapezoidal slightly more
    EXPECT_GE(stats.estimatedTime, 0.2f);
    EXPECT_LT(stats.estimatedTime, 0.25f);
}

TEST(GcodeAnalyzer, ToolChangeCount) {
    dw::gcode::Parser parser;
    auto program = parser.parse("M3 S12000\n"
                                "G1 X10 F500\n"
                                "M5\n"
                                "M6\n"
                                "M3 S10000\n"
                                "G1 X20 F500\n"
                                "M5\n"
                                "M6\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_EQ(stats.toolChangeCount, 2);
}

TEST(GcodeAnalyzer, Bounds) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X-5 Y-10\n"
                                "G1 X20 Y30 Z5 F1000\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    EXPECT_LE(stats.boundsMin.x, -5.0f);
    EXPECT_LE(stats.boundsMin.y, -10.0f);
    EXPECT_GE(stats.boundsMax.x, 20.0f);
    EXPECT_GE(stats.boundsMax.y, 30.0f);
    EXPECT_GE(stats.boundsMax.z, 5.0f);
}

TEST(GcodeAnalyzer, CommandCount_SkipsUnknown) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10\n"
                                "G1 X20 F500\n"
                                "; comment line\n");

    dw::gcode::Analyzer analyzer;
    auto stats = analyzer.analyze(program);

    // Only G0 and G1 should be counted
    EXPECT_EQ(stats.commandCount, 2);
}
