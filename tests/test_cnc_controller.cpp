// Tests for CncController — parsing and buffer logic (no hardware required)
#include <gtest/gtest.h>

#include "core/cnc/cnc_controller.h"
#include "core/cnc/cnc_types.h"

using namespace dw;

// --- Status report parsing ---

TEST(CncController, ParseStateIdle) {
    EXPECT_EQ(CncController::parseState("Idle"), MachineState::Idle);
}

TEST(CncController, ParseStateRun) {
    EXPECT_EQ(CncController::parseState("Run"), MachineState::Run);
}

TEST(CncController, ParseStateHoldWithSubstate) {
    EXPECT_EQ(CncController::parseState("Hold:0"), MachineState::Hold);
    EXPECT_EQ(CncController::parseState("Hold:1"), MachineState::Hold);
}

TEST(CncController, ParseStateAlarm) {
    EXPECT_EQ(CncController::parseState("Alarm"), MachineState::Alarm);
}

TEST(CncController, ParseStateUnknown) {
    EXPECT_EQ(CncController::parseState("Bogus"), MachineState::Unknown);
}

TEST(CncController, ParseStatusReportBasic) {
    auto status = CncController::parseStatusReport("<Idle|MPos:1.000,2.000,3.000|FS:500,1000>");

    EXPECT_EQ(status.state, MachineState::Idle);
    EXPECT_FLOAT_EQ(status.machinePos.x, 1.0f);
    EXPECT_FLOAT_EQ(status.machinePos.y, 2.0f);
    EXPECT_FLOAT_EQ(status.machinePos.z, 3.0f);
    EXPECT_FLOAT_EQ(status.feedRate, 500.0f);
    EXPECT_FLOAT_EQ(status.spindleSpeed, 1000.0f);
}

TEST(CncController, ParseStatusReportWithWPos) {
    auto status =
        CncController::parseStatusReport("<Run|WPos:10.500,-5.200,0.000|FS:800,0>");

    EXPECT_EQ(status.state, MachineState::Run);
    EXPECT_FLOAT_EQ(status.workPos.x, 10.5f);
    EXPECT_FLOAT_EQ(status.workPos.y, -5.2f);
    EXPECT_FLOAT_EQ(status.workPos.z, 0.0f);
}

TEST(CncController, ParseStatusReportWithOverrides) {
    auto status = CncController::parseStatusReport(
        "<Idle|MPos:0.000,0.000,0.000|FS:0,0|Ov:120,100,80>");

    EXPECT_EQ(status.feedOverride, 120);
    EXPECT_EQ(status.rapidOverride, 100);
    EXPECT_EQ(status.spindleOverride, 80);
}

TEST(CncController, ParseStatusReportFeedOnly) {
    auto status =
        CncController::parseStatusReport("<Run|MPos:0.000,0.000,0.000|F:1500>");

    EXPECT_FLOAT_EQ(status.feedRate, 1500.0f);
}

TEST(CncController, ParseStatusReportEmpty) {
    auto status = CncController::parseStatusReport("<>");
    EXPECT_EQ(status.state, MachineState::Unknown);
}

TEST(CncController, ParseStatusReportMalformed) {
    auto status = CncController::parseStatusReport("not a status report");
    EXPECT_EQ(status.state, MachineState::Unknown);
}

TEST(CncController, ParseStatusReportWithWCO) {
    auto status = CncController::parseStatusReport(
        "<Idle|MPos:10.000,20.000,5.000|WCO:1.000,2.000,0.500>");

    EXPECT_FLOAT_EQ(status.machinePos.x, 10.0f);
    EXPECT_FLOAT_EQ(status.workPos.x, 9.0f);
    EXPECT_FLOAT_EQ(status.workPos.y, 18.0f);
    EXPECT_FLOAT_EQ(status.workPos.z, 4.5f);
}

// --- Alarm/error descriptions ---

TEST(GrblTypes, AlarmDescriptions) {
    EXPECT_STREQ(alarmDescription(1), "Hard limit triggered. Machine position lost -- re-home required");
    EXPECT_STREQ(alarmDescription(2), "G-code motion target exceeds machine travel (soft limit)");
    EXPECT_STREQ(alarmDescription(999), "Unknown alarm");
}

TEST(GrblTypes, ErrorDescriptions) {
    EXPECT_STREQ(errorDescription(1), "G-code word consists of a letter with no value");
    EXPECT_STREQ(errorDescription(22), "Feed rate has not yet been set or is undefined");
    EXPECT_STREQ(errorDescription(4), "Negative value received for an expected positive value");
    EXPECT_STREQ(errorDescription(37), "G43.1 dynamic tool length offset cannot apply an offset to an axis other than configured");
    EXPECT_STREQ(errorDescription(999), "Unknown error");
}

// --- Pin parsing (Pn: field) ---

TEST(CncController, ParseStatusReportWithPnField) {
    auto status = CncController::parseStatusReport(
        "<Hold:0|MPos:0.000,0.000,0.000|Pn:XZP>");

    EXPECT_EQ(status.state, MachineState::Hold);
    EXPECT_TRUE(status.inputPins & cnc::PIN_X_LIMIT);
    EXPECT_FALSE(status.inputPins & cnc::PIN_Y_LIMIT);
    EXPECT_TRUE(status.inputPins & cnc::PIN_Z_LIMIT);
    EXPECT_TRUE(status.inputPins & cnc::PIN_PROBE);
    EXPECT_FALSE(status.inputPins & cnc::PIN_DOOR);
}

TEST(CncController, ParseStatusReportPnAllPins) {
    auto status = CncController::parseStatusReport(
        "<Idle|MPos:0.000,0.000,0.000|Pn:XYZPDHR>");

    EXPECT_TRUE(status.inputPins & cnc::PIN_X_LIMIT);
    EXPECT_TRUE(status.inputPins & cnc::PIN_Y_LIMIT);
    EXPECT_TRUE(status.inputPins & cnc::PIN_Z_LIMIT);
    EXPECT_TRUE(status.inputPins & cnc::PIN_PROBE);
    EXPECT_TRUE(status.inputPins & cnc::PIN_DOOR);
    EXPECT_TRUE(status.inputPins & cnc::PIN_HOLD);
    EXPECT_TRUE(status.inputPins & cnc::PIN_RESET);
    // Note: S (start) conflicts with S in spindle speed, but Pn: field handles it correctly
}

TEST(CncController, ParseStatusReportNoPnFieldDefaultsToZero) {
    auto status = CncController::parseStatusReport(
        "<Idle|MPos:0.000,0.000,0.000|FS:0,0>");

    EXPECT_EQ(status.inputPins, 0u);
}

TEST(CncController, ParseStatusReportPnDoorOnly) {
    auto status = CncController::parseStatusReport(
        "<Door:0|MPos:0.000,0.000,0.000|Pn:D>");

    EXPECT_EQ(status.state, MachineState::Door);
    EXPECT_TRUE(status.inputPins & cnc::PIN_DOOR);
    EXPECT_FALSE(status.inputPins & cnc::PIN_X_LIMIT);
}

// --- Pin constants ---

TEST(CncTypes, PinConstantsAreBitmask) {
    // Each pin should be a unique bit
    EXPECT_EQ(cnc::PIN_X_LIMIT, 1u << 0);
    EXPECT_EQ(cnc::PIN_Y_LIMIT, 1u << 1);
    EXPECT_EQ(cnc::PIN_Z_LIMIT, 1u << 2);
    EXPECT_EQ(cnc::PIN_PROBE,   1u << 3);
    EXPECT_EQ(cnc::PIN_DOOR,    1u << 4);
    EXPECT_EQ(cnc::PIN_HOLD,    1u << 5);
    EXPECT_EQ(cnc::PIN_RESET,   1u << 6);
    EXPECT_EQ(cnc::PIN_START,   1u << 7);
}

// --- RX buffer size ---

TEST(CncTypes, RxBufferSize) {
    EXPECT_EQ(cnc::RX_BUFFER_SIZE, 128);
}
