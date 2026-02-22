// Digital Workshop - G-code Parser Tests

#include <gtest/gtest.h>

#include "core/gcode/gcode_parser.h"
#include "core/gcode/gcode_types.h"

#include <cmath>

TEST(GcodeParser, ParseSimpleG0) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10 Y20 Z5\n");

    ASSERT_EQ(program.commands.size(), 1u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::G0);
    EXPECT_TRUE(program.commands[0].hasX());
    EXPECT_TRUE(program.commands[0].hasY());
    EXPECT_TRUE(program.commands[0].hasZ());
    EXPECT_FLOAT_EQ(program.commands[0].x, 10.0f);
    EXPECT_FLOAT_EQ(program.commands[0].y, 20.0f);
    EXPECT_FLOAT_EQ(program.commands[0].z, 5.0f);
}

TEST(GcodeParser, ParseSimpleG1) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X50 Y-10.5 F1000\n");

    ASSERT_EQ(program.commands.size(), 1u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::G1);
    EXPECT_FLOAT_EQ(program.commands[0].x, 50.0f);
    EXPECT_FLOAT_EQ(program.commands[0].y, -10.5f);
    EXPECT_TRUE(program.commands[0].hasF());
    EXPECT_FLOAT_EQ(program.commands[0].f, 1000.0f);
}

TEST(GcodeParser, ParseTwoMotionCommands) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10 Y20\nG1 X30 Y40 F500\n");

    EXPECT_EQ(program.commands.size(), 2u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::G0);
    EXPECT_EQ(program.commands[1].type, dw::gcode::CommandType::G1);
}

TEST(GcodeParser, ParseWithFeedRate) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G1 X10 F500\n");

    ASSERT_EQ(program.commands.size(), 1u);
    EXPECT_TRUE(program.commands[0].hasF());
    EXPECT_FLOAT_EQ(program.commands[0].f, 500.0f);
}

TEST(GcodeParser, CommentLinesSkipped) {
    dw::gcode::Parser parser;
    auto program = parser.parse("; This is a comment\n"
                                "G0 X10\n"
                                "; Another comment\n"
                                "(parenthetical comment)\n");

    // Only G0 should be parsed; comments are ignored
    ASSERT_EQ(program.commands.size(), 1u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::G0);
}

TEST(GcodeParser, EmptyInputProducesEmptyResult) {
    dw::gcode::Parser parser;
    auto program = parser.parse("");

    EXPECT_TRUE(program.commands.empty());
    EXPECT_TRUE(program.path.empty());
}

TEST(GcodeParser, PathSegmentsGenerated) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10 Y0\n"
                                "G1 X20 Y10 F500\n");

    ASSERT_EQ(program.commands.size(), 2u);
    ASSERT_EQ(program.path.size(), 2u);

    // First segment: G0 rapid from (0,0,0) to (10,0,0)
    EXPECT_TRUE(program.path[0].isRapid);
    EXPECT_FLOAT_EQ(program.path[0].start.x, 0.0f);
    EXPECT_FLOAT_EQ(program.path[0].end.x, 10.0f);

    // Second segment: G1 from (10,0,0) to (20,10,0)
    EXPECT_FALSE(program.path[1].isRapid);
    EXPECT_FLOAT_EQ(program.path[1].start.x, 10.0f);
    EXPECT_FLOAT_EQ(program.path[1].end.x, 20.0f);
    EXPECT_FLOAT_EQ(program.path[1].end.y, 10.0f);
}

TEST(GcodeParser, UnitSetting) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G20\n");

    EXPECT_EQ(program.units, dw::gcode::Units::Inches);

    auto program2 = parser.parse("G21\n");
    EXPECT_EQ(program2.units, dw::gcode::Units::Millimeters);
}

TEST(GcodeParser, AbsolutePositioning) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G90\n"
                                "G0 X10\n"
                                "G0 X20\n");

    EXPECT_EQ(program.positioning, dw::gcode::PositioningMode::Absolute);
    ASSERT_EQ(program.path.size(), 2u);
    // In absolute mode, second move goes to X=20 (not X=10+20)
    EXPECT_FLOAT_EQ(program.path[1].end.x, 20.0f);
}

TEST(GcodeParser, RelativePositioning) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G91\n"
                                "G0 X10\n"
                                "G0 X5\n");

    EXPECT_EQ(program.positioning, dw::gcode::PositioningMode::Relative);
    ASSERT_EQ(program.path.size(), 2u);
    // In relative mode, second move goes to X=10+5=15
    EXPECT_FLOAT_EQ(program.path[1].end.x, 15.0f);
}

TEST(GcodeParser, MCommands) {
    dw::gcode::Parser parser;
    auto program = parser.parse("M3 S12000\n"
                                "M5\n");

    ASSERT_EQ(program.commands.size(), 2u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::M3);
    EXPECT_TRUE(program.commands[0].hasS());
    EXPECT_FLOAT_EQ(program.commands[0].s, 12000.0f);
    EXPECT_EQ(program.commands[1].type, dw::gcode::CommandType::M5);
}

TEST(GcodeParser, InlineCommentStripped) {
    dw::gcode::Parser parser;
    auto program = parser.parse("G0 X10 ; move to start\n");

    ASSERT_EQ(program.commands.size(), 1u);
    EXPECT_EQ(program.commands[0].type, dw::gcode::CommandType::G0);
    EXPECT_FLOAT_EQ(program.commands[0].x, 10.0f);
}
