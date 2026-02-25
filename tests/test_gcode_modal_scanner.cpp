// Digital Workshop - G-code Modal State Scanner Tests

#include <gtest/gtest.h>

#include "core/gcode/gcode_modal_scanner.h"

using namespace dw;

// Test 1: Empty program returns defaults
TEST(GCodeModalScanner, EmptyProgramReturnsDefaults) {
    std::vector<std::string> program;
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.distanceMode, "G90");
    EXPECT_EQ(state.coordinateSystem, "G54");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M5");
    EXPECT_EQ(state.coolantState, "M9");
    EXPECT_FLOAT_EQ(state.feedRate, 0.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 0.0f);
}

// Test 2: Simple program with all modal codes set
TEST(GCodeModalScanner, AllModalCodesSet) {
    std::vector<std::string> program = {
        "G20",
        "G55",
        "G91",
        "M3",
        "M7",
        "F1500",
        "S12000",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.units, "G20");
    EXPECT_EQ(state.coordinateSystem, "G55");
    EXPECT_EQ(state.distanceMode, "G91");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_EQ(state.coolantState, "M7");
    EXPECT_FLOAT_EQ(state.feedRate, 1500.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 12000.0f);
}

// Test 3: Mid-program scan (endLine < program.size())
TEST(GCodeModalScanner, MidProgramScan) {
    std::vector<std::string> program = {
        "G90",
        "G21",
        "M3 S10000",
        "G1 X10 F500",
        "G91",       // Line 4 - should not be included when scanning to line 4
        "M5",        // Line 5
    };
    auto state = GCodeModalScanner::scanToLine(program, 4);

    EXPECT_EQ(state.distanceMode, "G90");  // Not G91
    EXPECT_EQ(state.spindleState, "M3");   // Not M5
    EXPECT_FLOAT_EQ(state.feedRate, 500.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 10000.0f);
}

// Test 4: Lowercase g-code
TEST(GCodeModalScanner, LowercaseHandled) {
    std::vector<std::string> program = {
        "g90 g21",
        "m3 s8000",
        "g1 x10 f400",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.distanceMode, "G90");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.feedRate, 400.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 8000.0f);
}

// Test 5: Comments stripped
TEST(GCodeModalScanner, CommentsStripped) {
    std::vector<std::string> program = {
        "(Setup) G20 (inches)",
        "G91 ; incremental mode",
        "; this is a full line comment",
        "(this is also a comment)",
        "M3 S5000 (spindle on)",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.units, "G20");
    EXPECT_EQ(state.distanceMode, "G91");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.spindleSpeed, 5000.0f);
}

// Test 6: Multiple codes per line
TEST(GCodeModalScanner, MultipleCodesPerLine) {
    std::vector<std::string> program = {
        "G90 G21 M3 S12000",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.distanceMode, "G90");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.spindleSpeed, 12000.0f);
}

// Test 7: F and S values from motion commands
TEST(GCodeModalScanner, FeedAndSpindleFromMotion) {
    std::vector<std::string> program = {
        "G1 X10 F500",
        "S12000 M3",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_FLOAT_EQ(state.feedRate, 500.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 12000.0f);
    EXPECT_EQ(state.spindleState, "M3");
}

// Test 8: All coordinate systems
TEST(GCodeModalScanner, CoordinateSystems) {
    auto testWcs = [](const std::string& code) {
        std::vector<std::string> program = {code};
        auto state = GCodeModalScanner::scanToLine(program, 100);
        return state.coordinateSystem;
    };

    EXPECT_EQ(testWcs("G54"), "G54");
    EXPECT_EQ(testWcs("G55"), "G55");
    EXPECT_EQ(testWcs("G56"), "G56");
    EXPECT_EQ(testWcs("G57"), "G57");
    EXPECT_EQ(testWcs("G58"), "G58");
    EXPECT_EQ(testWcs("G59"), "G59");
}

// Test 9: toPreamble generates correct restore sequence
TEST(GCodeModalScanner, ToPreambleCorrectOrder) {
    ModalState state;
    state.units = "G20";
    state.coordinateSystem = "G55";
    state.distanceMode = "G91";
    state.feedRate = 1000.0f;
    state.spindleSpeed = 18000.0f;
    state.spindleState = "M3";
    state.coolantState = "M8";

    auto preamble = state.toPreamble();

    ASSERT_EQ(preamble.size(), 7u);
    EXPECT_EQ(preamble[0], "G20");     // Units first
    EXPECT_EQ(preamble[1], "G55");     // Coordinate system
    EXPECT_EQ(preamble[2], "G91");     // Distance mode
    EXPECT_EQ(preamble[3], "F1000");   // Feed rate
    EXPECT_EQ(preamble[4], "S18000");  // Spindle speed
    EXPECT_EQ(preamble[5], "M3");      // Spindle state
    EXPECT_EQ(preamble[6], "M8");      // Coolant state
}

// Test 9b: toPreamble omits F and S when zero
TEST(GCodeModalScanner, ToPreambleOmitsZeroFeedAndSpeed) {
    ModalState state; // Defaults: F=0, S=0
    auto preamble = state.toPreamble();

    // Should have 5 lines: units, wcs, distance, spindle, coolant (no F or S)
    ASSERT_EQ(preamble.size(), 5u);
    EXPECT_EQ(preamble[0], "G21");
    EXPECT_EQ(preamble[1], "G54");
    EXPECT_EQ(preamble[2], "G90");
    EXPECT_EQ(preamble[3], "M5");
    EXPECT_EQ(preamble[4], "M9");
}

// Test 10: Arc commands don't affect modal tracking
TEST(GCodeModalScanner, ArcCommandsDontAffectModalState) {
    std::vector<std::string> program = {
        "G90 G21",
        "M3 S10000",
        "G1 X10 Y10 F500",
        "G2 X20 Y20 I5 J5",  // Arc command
        "G3 X30 Y30 I-5 J-5", // Arc command
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.distanceMode, "G90");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.feedRate, 500.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 10000.0f);
}

// Test 11: Blank lines and comment-only lines safely skipped
TEST(GCodeModalScanner, BlankAndCommentLines) {
    std::vector<std::string> program = {
        "",
        "   ",
        "; comment only",
        "(comment only)",
        "G20",
        "",
        "M3",
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.units, "G20");
    EXPECT_EQ(state.spindleState, "M3");
}

// Test 12: endLine=0 returns defaults
TEST(GCodeModalScanner, EndLineZeroReturnsDefaults) {
    std::vector<std::string> program = {"G20", "G91", "M3"};
    auto state = GCodeModalScanner::scanToLine(program, 0);

    EXPECT_EQ(state.distanceMode, "G90");
    EXPECT_EQ(state.coordinateSystem, "G54");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M5");
    EXPECT_EQ(state.coolantState, "M9");
    EXPECT_FLOAT_EQ(state.feedRate, 0.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 0.0f);
}

// Test 13: endLine > program.size() scans entire program without error
TEST(GCodeModalScanner, EndLineBeyondProgramSize) {
    std::vector<std::string> program = {"G20", "G91", "M3 S8000 F600"};
    auto state = GCodeModalScanner::scanToLine(program, 10000);

    EXPECT_EQ(state.units, "G20");
    EXPECT_EQ(state.distanceMode, "G91");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.feedRate, 600.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 8000.0f);
}

// Test 14: Last value wins when same modal group is set multiple times
TEST(GCodeModalScanner, LastValueWins) {
    std::vector<std::string> program = {
        "G20",
        "G21",  // Override to mm
        "M3",
        "M4",   // Override to CCW
        "F500",
        "F800", // Override feed
    };
    auto state = GCodeModalScanner::scanToLine(program, 100);

    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.spindleState, "M4");
    EXPECT_FLOAT_EQ(state.feedRate, 800.0f);
}

// Test 15: Realistic CNC program
TEST(GCodeModalScanner, RealisticProgram) {
    std::vector<std::string> program = {
        "(Header)",
        "G90 G54 G21",
        "S18000 M3",
        "G0 X0 Y0 Z10",
        "G0 X50 Y25",
        "G1 Z-5 F300",
        "G1 X100 Y50 F1200",
        "G1 X150 Y25",
        "G91",           // Switch to incremental for a section
        "G1 X10 Y10",
        "G90",           // Back to absolute
        "G0 Z10",
        "M5",
        "M9",
        "M2",
    };

    // Scan to line 8 (before G91 switch back to G90)
    auto state = GCodeModalScanner::scanToLine(program, 9);
    EXPECT_EQ(state.distanceMode, "G91");
    EXPECT_EQ(state.units, "G21");
    EXPECT_EQ(state.coordinateSystem, "G54");
    EXPECT_EQ(state.spindleState, "M3");
    EXPECT_FLOAT_EQ(state.feedRate, 1200.0f);
    EXPECT_FLOAT_EQ(state.spindleSpeed, 18000.0f);

    // Scan to end
    auto endState = GCodeModalScanner::scanToLine(program, 100);
    EXPECT_EQ(endState.distanceMode, "G90");
    EXPECT_EQ(endState.spindleState, "M5");
    EXPECT_EQ(endState.coolantState, "M9");
}
