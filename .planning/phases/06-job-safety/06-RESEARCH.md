# Phase 6: Job Safety - Research

**Researched:** 2026-02-24
**Domain:** GRBL CNC safety controls (pause/resume/E-stop, modal state reconstruction, sensor display, pre-flight checks)
**Confidence:** HIGH

## Summary

Phase 6 implements the safety control layer for the CNC controller. The existing codebase already has solid foundations: thread-safe real-time command dispatch via atomic bitmask (Phase 1), status polling at 5Hz, character-counting streaming with soft-reset-on-error (Phase 1 fixes), and a callback-driven UI update pattern via MainThreadQueue. The primary work is: (1) adding safety control buttons to the UI with correct state gating, (2) building a G-code modal state scanner for safe resume-from-line, (3) parsing the Pn: field from GRBL status reports for sensor display, and (4) implementing pre-flight validation before streaming.

The most complex and safety-critical component is SAF-04/SAF-05 (resume from line N with modal state reconstruction). This requires scanning all G-code from line 0 to the resume point, tracking all modal states (G90/G91, G54-G59, M3/M4/M5, M7/M8/M9, F/S values, G20/G21), and generating a preamble that restores machine state before resuming. Getting this wrong means the machine resumes in an incorrect state, potentially crashing the tool.

**Primary recommendation:** Implement safety controls in three waves: (1) core safety commands + Pn: parsing, (2) pre-flight checks + safety control panel UI, (3) modal state reconstruction + resume-from-line. The modal state scanner should be a standalone, thoroughly tested utility class separate from the UI.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Safety is paramount (established in Phase 1 context)
- E-stop must be the most prominent, impossible-to-miss control
- Resume operations must verify machine state before continuing
- Pause sends feed hold (!), Resume sends cycle start (~)
- E-stop sends soft reset (0x18) -- requires confirmation dialog when job is running
- Button states must reflect machine state (e.g., Resume only enabled in Hold state)
- User specifies line number to resume from
- System scans G-code from line 0 to resume point, building modal state preamble
- Preamble includes: G90/G91, coordinate system, spindle, coolant, feed rate, units
- Show endstop, probe, and door states from GRBL Pn: field (read-only display)
- Before streaming: verify connection active, no alarm state
- Optional warnings: tool selected, material selected
- Don't block streaming on optional checks -- warn only

### Claude's Discretion
- E-stop button size, color, placement (should be prominent)
- Resume dialog layout (line number input, state preview)
- Whether pre-flight is a modal dialog or inline warnings
- Sensor display format (icons, text, LED-style indicators)
- How to present the modal state preamble to the operator for review before resume

### Deferred Ideas (OUT OF SCOPE)
- Arc mid-point resume (G2/G3) -- edge case, warn and skip for now
- Automatic resume after power loss -- would require persistent state storage, future work
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SAF-01 | Pause button sends feed hold (!) with visual state change | feedHold() already routes through IO thread via atomic bitmask; UI needs state-aware button |
| SAF-02 | Resume button sends cycle start (~) only when in Hold state | cycleStart() exists; UI must gate on MachineState::Hold; m_held atomic tracks hold state |
| SAF-03 | E-stop button sends soft reset (0x18) with confirmation dialog for running jobs | softReset() exists; need ImGui confirmation popup; CRITICAL-3 from PITFALLS.md: never label "E-Stop" |
| SAF-04 | Safe job resume: user can specify line number to resume from | New feature: line selection UI + startStream() called with subset of program lines prepended by preamble |
| SAF-05 | Resume builds modal state preamble by scanning G-code from line 0 to resume point | New GCodeModalScanner utility class; must track G90/G91, G54-G59, M3/M4/M5, M7/M8/M9, F, S, G20/G21 |
| SAF-06 | Sensor status display shows endstop, probe, and door states from GRBL Pn: field | parseStatusReport() needs Pn: field parsing; MachineStatus.inputPins field exists but is never populated |
| SAF-07 | Pre-flight check before streaming: verify connection, no alarm, tool/material selected (optional) | New validation logic before startStream(); check m_connected, lastStatus().state != Alarm |
</phase_requirements>

## Standard Stack

### Core (Existing)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Dear ImGui (docking) | Current | All UI rendering, buttons, dialogs, popups | Already in use throughout all panels |
| C++17 STL | N/A | Threading, atomics, data structures | Project standard |
| GRBL v1.1 protocol | 1.1 | Real-time commands, status reporting | Hardware protocol |

### Supporting (Existing)
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| MainThreadQueue | Internal | UI thread callback dispatch | All controller-to-UI communication |
| EventBus | Internal | Decoupled component events | Cross-panel notifications |
| CncCallbacks | Internal | Controller event subscription | Status updates, errors, alarms |

### No New Dependencies Needed
This phase uses only existing libraries and patterns. No new external dependencies are required.

## Architecture Patterns

### Existing CNC Panel Pattern
All CNC panels follow the same pattern established in Phases 2-5:
```
src/ui/panels/cnc_{name}_panel.h/.cpp
```
- Inherit from `Panel`
- Receive `CncController*` via setter or callbacks via MainThreadQueue
- Store status locally, render from local copy (no mutex needed -- all on main thread)
- Wired in `application_wiring.cpp`

### Pattern 1: Safety Control Panel (New)
**What:** A new `CncSafetyPanel` that houses Pause/Resume/Abort buttons and sensor status.
**When to use:** Consolidates all safety controls in one always-visible panel.
**Structure:**
```cpp
class CncSafetyPanel : public Panel {
    void render() override;
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);
    void setCncController(CncController* ctrl);
private:
    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;
    bool m_showAbortConfirm = false; // For confirmation dialog
};
```

### Pattern 2: G-Code Modal State Scanner (New Core Utility)
**What:** A standalone class that scans G-code lines and tracks modal state at any point.
**When to use:** Resume-from-line feature (SAF-04/SAF-05).
**Where:** `src/core/gcode/gcode_modal_scanner.h/.cpp` (alongside existing gcode_parser/gcode_analyzer)
```cpp
struct ModalState {
    std::string distanceMode;       // G90 or G91
    std::string coordinateSystem;   // G54-G59
    std::string units;              // G20 or G21
    std::string spindleState;       // M3, M4, or M5
    std::string coolantState;       // M7, M8, or M9
    float feedRate = 0.0f;          // F value
    float spindleSpeed = 0.0f;      // S value
    // Generate preamble lines to restore this state
    std::vector<std::string> toPreamble() const;
};

class GCodeModalScanner {
public:
    // Scan lines 0..endLine (exclusive), return accumulated modal state
    static ModalState scanToLine(const std::vector<std::string>& program, int endLine);
};
```

### Pattern 3: Pre-flight Validation
**What:** Validation check before streaming begins.
**When to use:** Before any call to `startStream()`.
**Structure:** A free function or static method that returns a list of issues:
```cpp
struct PreflightIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string message;
};

std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected
);
```
Errors block streaming; warnings are shown but don't block.

### Pattern 4: Pn: Pin State Bitmask
**What:** Parse GRBL Pn: field into a bitmask for sensor display.
**Standard encoding:**
```
Pn: field characters -> bit positions:
X = X limit    Y = Y limit    Z = Z limit
P = Probe      D = Door       H = Hold       R = Reset       S = Cycle Start
```
Parse into `MachineStatus::inputPins` as a bitmask. Define constants:
```cpp
namespace cnc {
    constexpr u32 PIN_X_LIMIT = 1 << 0;
    constexpr u32 PIN_Y_LIMIT = 1 << 1;
    constexpr u32 PIN_Z_LIMIT = 1 << 2;
    constexpr u32 PIN_PROBE   = 1 << 3;
    constexpr u32 PIN_DOOR    = 1 << 4;
    constexpr u32 PIN_HOLD    = 1 << 5;
    constexpr u32 PIN_RESET   = 1 << 6;
    constexpr u32 PIN_START   = 1 << 7;
}
```

### Anti-Patterns to Avoid
- **Labeling software button "E-Stop":** Per PITFALLS.md CRITICAL-3, never label a software button as E-Stop. Use "Abort" or "Soft Reset" and include a note about hardware E-stop.
- **Resuming without state verification:** Per PITFALLS.md CRITICAL-4, always verify position and modal state before resume. Don't just send cycle start blindly.
- **Building modal scanner that handles arcs mid-point:** Per deferred decisions, G2/G3 mid-arc resume is out of scope. Warn and skip.
- **Blocking on optional pre-flight checks:** Tool/material selection warnings must not block streaming.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| G-code parsing | Full G-code parser for modal scanner | Simple line-by-line regex/string scan | Only need to track modal G/M codes and F/S values, not full motion planning |
| Confirmation dialog | Custom modal window system | `ImGui::OpenPopup()` + `ImGui::BeginPopupModal()` | Standard ImGui pattern, consistent with other dialogs in the app |
| Button state management | Custom state machine for button enable/disable | Direct check of `MachineState` enum from `lastStatus()` | State is already tracked by the controller; just read it |

## Common Pitfalls

### Pitfall 1: Resume Sends Cycle Start Without Checking Hold State
**What goes wrong:** User clicks Resume while machine is in Idle or Run state. Cycle start (~) in non-Hold state has undefined behavior or no effect, confusing the user.
**Why it happens:** UI doesn't gate the Resume button on machine state.
**How to avoid:** Only enable Resume button when `lastStatus().state == MachineState::Hold`. Grey out otherwise.
**Warning signs:** Resume button clickable when machine is idle.

### Pitfall 2: Modal State Scanner Misses G-code Variants
**What goes wrong:** Scanner doesn't recognize `g90` (lowercase), `G90G91` (multiple codes on one line), or `G90 ; comment` (inline comments).
**Why it happens:** Naive string matching instead of proper token extraction.
**How to avoid:** Normalize lines (uppercase, strip comments) before scanning. Handle multiple G/M codes per line.
**Warning signs:** Modal state preamble missing expected codes.

### Pitfall 3: Abort Confirmation Dialog Blocks Critical Action
**What goes wrong:** In an emergency, user has to click through a confirmation dialog before abort takes effect. Precious seconds lost.
**Why it happens:** Over-applying safety dialog to all abort scenarios.
**How to avoid:** Only show confirmation when a job is actively running (streaming = true). If machine is in Hold or Idle, send soft reset immediately without dialog.
**Warning signs:** User complaints about slow abort response.

### Pitfall 4: Pn: Field Absent From Status Reports
**What goes wrong:** Sensor display shows nothing because GRBL only sends Pn: when at least one pin is active. Code assumes Pn: is always present.
**Why it happens:** GRBL omits Pn: field entirely when no input pins are active (bandwidth optimization).
**How to avoid:** Default all pin states to "inactive". Only update when Pn: field is present. Clear pins at the start of each status parse.
**Warning signs:** Sensor display stuck on last state or showing stale data.

### Pitfall 5: Pre-flight Check Race Condition
**What goes wrong:** Pre-flight passes but state changes between check and stream start (e.g., alarm triggers between check and startStream call).
**Why it happens:** Non-atomic check-then-act pattern.
**How to avoid:** Check state immediately before calling startStream(). Keep the window between check and start as small as possible. The startStream() method itself should also reject if in alarm state.
**Warning signs:** Streaming starts despite alarm being active.

### Pitfall 6: Resume-from-Line Off-by-One
**What goes wrong:** User says "resume from line 100" but the scanner scans through line 100 (inclusive) or stops at line 99 (missing the state set by line 99).
**Why it happens:** Ambiguity in whether "line N" means "start executing at line N" (scan 0..N-1) or "resume after line N" (scan 0..N).
**How to avoid:** Define clearly: "Resume from line N" means line N is the FIRST line executed. Scanner scans lines 0 through N-1 (inclusive) to build preamble. Document and test this boundary.
**Warning signs:** Preamble includes the resume line's commands, or misses the line immediately before.

## Code Examples

### Pn: Field Parsing (to add in parseStatusReport)
```cpp
// Inside the field parsing loop in parseStatusReport():
} else if (key == "Pn") {
    status.inputPins = 0;
    for (char c : val) {
        switch (c) {
        case 'X': status.inputPins |= cnc::PIN_X_LIMIT; break;
        case 'Y': status.inputPins |= cnc::PIN_Y_LIMIT; break;
        case 'Z': status.inputPins |= cnc::PIN_Z_LIMIT; break;
        case 'P': status.inputPins |= cnc::PIN_PROBE; break;
        case 'D': status.inputPins |= cnc::PIN_DOOR; break;
        case 'H': status.inputPins |= cnc::PIN_HOLD; break;
        case 'R': status.inputPins |= cnc::PIN_RESET; break;
        case 'S': status.inputPins |= cnc::PIN_START; break;
        }
    }
}
```

### ImGui Confirmation Popup (Abort Dialog Pattern)
```cpp
// In render():
if (m_showAbortConfirm)
    ImGui::OpenPopup("Abort Job?");

if (ImGui::BeginPopupModal("Abort Job?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("A job is currently running. Sending a soft reset will\n"
                "stop all motion and require re-homing.\n\n"
                "Note: This is a SOFTWARE abort. Ensure your hardware\n"
                "E-stop is accessible for true emergencies.");
    ImGui::Separator();
    if (ImGui::Button("Abort Job", ImVec2(120, 0))) {
        m_cnc->softReset();
        m_showAbortConfirm = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_showAbortConfirm = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
```

### Modal State Scanner Core Logic
```cpp
ModalState GCodeModalScanner::scanToLine(const std::vector<std::string>& program, int endLine) {
    ModalState state;
    // Defaults (GRBL power-on defaults per $RST=*)
    state.distanceMode = "G90";
    state.coordinateSystem = "G54";
    state.units = "G21";
    state.spindleState = "M5";
    state.coolantState = "M9";

    for (int i = 0; i < endLine && i < static_cast<int>(program.size()); ++i) {
        std::string line = program[i];
        // Strip comments: (...) and ;...
        // Uppercase for matching
        // Extract G/M codes and F/S values
        // Update state tracking
    }
    return state;
}

std::vector<std::string> ModalState::toPreamble() const {
    std::vector<std::string> lines;
    lines.push_back(units);              // G20 or G21 -- units first
    lines.push_back(coordinateSystem);   // G54-G59
    lines.push_back(distanceMode);       // G90 or G91
    if (feedRate > 0.0f)
        lines.push_back("F" + std::to_string(static_cast<int>(feedRate)));
    if (spindleSpeed > 0.0f)
        lines.push_back("S" + std::to_string(static_cast<int>(spindleSpeed)));
    lines.push_back(spindleState);       // M3/M4/M5
    lines.push_back(coolantState);       // M7/M8/M9
    return lines;
}
```

### Pre-flight Check Implementation
```cpp
std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected)
{
    std::vector<PreflightIssue> issues;

    if (!ctrl.isConnected())
        issues.push_back({PreflightIssue::Error, "Not connected to CNC controller"});

    if (ctrl.lastStatus().state == MachineState::Alarm)
        issues.push_back({PreflightIssue::Error, "Machine is in ALARM state -- clear alarm first"});

    if (ctrl.isInErrorState())
        issues.push_back({PreflightIssue::Error, "Previous streaming error not acknowledged"});

    if (!hasToolSelected)
        issues.push_back({PreflightIssue::Warning, "No tool selected -- feed rate recommendations unavailable"});

    if (!hasMaterialSelected)
        issues.push_back({PreflightIssue::Warning, "No material selected -- cutting parameters not validated"});

    return issues;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct port writes from UI thread | Atomic bitmask command queue (Phase 1 fix) | Phase 1 | Thread-safe real-time commands already work |
| No error recovery during streaming | Soft reset on streaming error (Phase 1 fix) | Phase 1 | Error state + acknowledgment pattern exists |
| Simple streaming start | Character-counting with buffer tracking | Phase 5 | Need to extend with pre-flight validation |

## Open Questions

1. **Feed hold during G2/G3 arc execution**
   - What we know: GRBL handles feed hold mid-arc correctly at the firmware level
   - What's unclear: Whether the reported position during hold mid-arc is exactly on the arc path
   - Recommendation: Trust GRBL's position reporting; don't try to correct for arc geometry

2. **Spindle spin-up delay after resume**
   - What we know: GRBL with parking enabled can restore spindle/coolant automatically
   - What's unclear: Whether we need an explicit delay for spindle to reach speed before cutting
   - Recommendation: For v1, rely on GRBL's built-in parking/restore behavior. Document as a known limitation if parking is disabled.

3. **G-code line numbering (N-words)**
   - What we know: Some G-code files use N-word line numbers (N100 G1 X10...)
   - What's unclear: Whether "resume from line N" should mean file line or N-word line
   - Recommendation: Use file line numbers (0-indexed array position). N-words are unreliable and optional.

## Sources

### Primary (HIGH confidence)
- Existing codebase: `src/core/cnc/cnc_controller.h/.cpp`, `cnc_types.h` -- direct code inspection
- GRBL v1.1 Interface documentation (status report format, Pn: field, real-time commands)
- `.planning/research/PITFALLS.md` -- CRITICAL-3 (E-stop labeling), CRITICAL-4 (unsafe resume)

### Secondary (MEDIUM confidence)
- GRBL source code (gnea/grbl) for modal state defaults on power-up
- ImGui documentation for popup/modal dialog patterns

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- using only existing project libraries
- Architecture: HIGH -- following established panel patterns from Phases 2-5
- Pitfalls: HIGH -- directly verified against PITFALLS.md and codebase inspection
- Modal state reconstruction: MEDIUM -- logic is well-understood but edge cases in G-code parsing need thorough testing

**Research date:** 2026-02-24
**Valid until:** 2026-03-24 (30 days -- stable domain, no external dependency changes expected)
