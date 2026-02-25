#pragma once

#include <string>
#include <vector>

namespace dw {

// Modal state accumulated by scanning G-code lines from program start.
// Used for resume-from-line: generates a preamble that restores machine state.
struct ModalState {
    std::string distanceMode = "G90";       // G90 (absolute) or G91 (incremental)
    std::string coordinateSystem = "G54";   // G54 through G59
    std::string units = "G21";              // G20 (inch) or G21 (mm)
    std::string spindleState = "M5";        // M3 (CW), M4 (CCW), M5 (off)
    std::string coolantState = "M9";        // M7 (mist), M8 (flood), M9 (off)
    float feedRate = 0.0f;                  // F value
    float spindleSpeed = 0.0f;              // S value

    // Generate G-code preamble to restore this modal state.
    // Order: units, coordinate system, distance mode, feed rate, spindle speed,
    //        spindle state, coolant state.
    std::vector<std::string> toPreamble() const;
};

// Scans a G-code program line-by-line and accumulates modal state.
class GCodeModalScanner {
  public:
    // Scan lines [0, endLine) and return accumulated modal state.
    // If endLine exceeds program size, scans the entire program.
    // If endLine is 0, returns default state.
    static ModalState scanToLine(const std::vector<std::string>& program, int endLine);
};

} // namespace dw
