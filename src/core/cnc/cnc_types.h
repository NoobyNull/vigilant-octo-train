#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../types.h"

namespace dw {

// GRBL machine state
enum class MachineState {
    Unknown,
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep
};

// Parsed GRBL status report from `?` query
struct MachineStatus {
    MachineState state = MachineState::Unknown;
    Vec3 machinePos{0.0f};
    Vec3 workPos{0.0f};
    f32 feedRate = 0.0f;
    f32 spindleSpeed = 0.0f;
    int feedOverride = 100;
    int rapidOverride = 100;
    int spindleOverride = 100;
    u32 inputPins = 0;
};

// Work coordinate system offsets parsed from $# response
struct WcsOffsets {
    Vec3 g54{0.0f};
    Vec3 g55{0.0f};
    Vec3 g56{0.0f};
    Vec3 g57{0.0f};
    Vec3 g58{0.0f};
    Vec3 g59{0.0f};
    Vec3 g28{0.0f};  // G28 home position
    Vec3 g30{0.0f};  // G30 home position
    Vec3 g92{0.0f};  // G92 offset
    float tlo = 0.0f; // Tool length offset

    // Get offset by WCS index (0=G54, 1=G55, ..., 5=G59)
    Vec3 getByIndex(int idx) const {
        switch (idx) {
        case 0: return g54;
        case 1: return g55;
        case 2: return g56;
        case 3: return g57;
        case 4: return g58;
        case 5: return g59;
        default: return Vec3{0.0f};
        }
    }
};

// Result of sending a single line
struct LineAck {
    int lineIndex = -1;      // Index in the gcode program
    bool ok = false;          // true = "ok", false = "error:N"
    int errorCode = 0;        // GRBL error code (0 if ok)
    std::string errorMessage; // Human-readable error description
};

// Streaming progress
struct StreamProgress {
    int totalLines = 0;
    int ackedLines = 0;
    int errorCount = 0;
    f32 elapsedSeconds = 0.0f;
};

// Detailed streaming error report (when error occurs during character-counting streaming)
struct StreamingError {
    int lineIndex = -1;           // Which program line failed
    int errorCode = 0;            // GRBL error code
    std::string errorMessage;     // Human-readable description
    std::string failedLine;       // The actual G-code line that failed
    int linesInFlight = 0;        // How many lines were buffered in GRBL when error occurred
};

// GRBL alarm descriptions
inline const char* alarmDescription(int code) {
    switch (code) {
    case 1: return "Hard limit triggered";
    case 2: return "Soft limit alarm";
    case 3: return "Abort during cycle";
    case 4: return "Probe fail";
    case 5: return "Probe fail";
    case 6: return "Homing fail (reset during cycle)";
    case 7: return "Homing fail (door opened)";
    case 8: return "Homing fail (pull-off failed)";
    case 9: return "Homing fail (approach failed)";
    case 10: return "Homing fail (limit not found)";
    default: return "Unknown alarm";
    }
}

// GRBL error descriptions
inline const char* errorDescription(int code) {
    switch (code) {
    case 1: return "G-code word consists of only a letter";
    case 2: return "Numeric value format is not valid";
    case 3: return "Grbl '$' system command was not recognized";
    case 9: return "G-code locked out during alarm";
    case 14: return "Build info or startup line exceeded line length";
    case 15: return "Jog target exceeds machine travel";
    case 20: return "Unsupported or invalid G-code command";
    case 22: return "Feed rate has not yet been set or is undefined";
    case 23: return "G-code command requires an integer value";
    case 24: return "Multiple G-code commands using axis words";
    case 25: return "Duplicate G-code word found in block";
    case 33: return "Invalid target, arc is 0 or exceeds tolerance";
    default: return "Unknown error";
    }
}

// GRBL real-time command bytes
namespace cnc {

constexpr u8 CMD_SOFT_RESET = 0x18;
constexpr u8 CMD_STATUS_QUERY = '?';
constexpr u8 CMD_CYCLE_START = '~';
constexpr u8 CMD_FEED_HOLD = '!';

// Feed override
constexpr u8 CMD_FEED_100_PERCENT = 0x90;
constexpr u8 CMD_FEED_PLUS_10 = 0x91;
constexpr u8 CMD_FEED_MINUS_10 = 0x92;
constexpr u8 CMD_FEED_PLUS_1 = 0x93;
constexpr u8 CMD_FEED_MINUS_1 = 0x94;

// Rapid override
constexpr u8 CMD_RAPID_100_PERCENT = 0x95;
constexpr u8 CMD_RAPID_50_PERCENT = 0x96;
constexpr u8 CMD_RAPID_25_PERCENT = 0x97;

// Spindle override
constexpr u8 CMD_SPINDLE_100_PERCENT = 0x99;
constexpr u8 CMD_SPINDLE_PLUS_10 = 0x9A;
constexpr u8 CMD_SPINDLE_MINUS_10 = 0x9B;
constexpr u8 CMD_SPINDLE_PLUS_1 = 0x9C;
constexpr u8 CMD_SPINDLE_MINUS_1 = 0x9D;

constexpr int RX_BUFFER_SIZE = 128; // GRBL serial RX buffer

} // namespace cnc

// Callbacks for CncController events (all called on main thread via MainThreadQueue)
struct CncCallbacks {
    std::function<void(bool connected, const std::string& version)> onConnectionChanged;
    std::function<void(const MachineStatus& status)> onStatusUpdate;
    std::function<void(const LineAck& ack)> onLineAcked;
    std::function<void(const StreamProgress& progress)> onProgressUpdate;
    std::function<void(int alarmCode, const std::string& desc)> onAlarm;
    std::function<void(const std::string& message)> onError;
    std::function<void(const StreamingError& error)> onStreamingError;
    std::function<void(const std::string& line, bool isSent)> onRawLine;
};

} // namespace dw
