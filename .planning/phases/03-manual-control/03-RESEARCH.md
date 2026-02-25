# Phase 3: Manual Control - Research

**Researched:** 2026-02-24
**Domain:** CNC machine manual control via Dear ImGui + GRBL protocol
**Confidence:** HIGH

## Summary

Phase 3 adds interactive machine control: jog buttons (CUI-04), keyboard jog (CUI-05), continuous jog (CUI-06), homing (CUI-07), work zero setting (CUI-08), WCS selection (CUI-09), and MDI console (CUI-10). The existing CncController already has the low-level primitives needed — `jogCancel()`, the `m_pendingStringCmds` queue for arbitrary G-code, real-time command dispatch, and 5Hz status polling. The main work is: (1) adding a public `sendCommand()` method to CncController for MDI and jog commands, (2) building new UI panels for jog controls, WCS management, and MDI console, (3) wiring keyboard shortcuts with focus-aware routing, and (4) integrating everything into the CNC workspace mode established in Phase 2.

**Primary recommendation:** Build three new UI components — CncJogPanel (jog + homing), CncWcsPanel (zero-set + G54-G59 selector), and CncConsolePanel (MDI input + response display) — and add a `sendCommand(string)` method to CncController that queues arbitrary G-code via the existing thread-safe string command queue.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
No specific locked decisions — user deferred all implementation decisions to Claude.

### Claude's Discretion
**Jog Interaction:**
- Button layout and arrangement
- Step size options (0.1, 1, 10, 100mm required per CUI-04)
- Keyboard shortcut mapping (arrows + page up/down per CUI-05)
- Continuous jog feel and cancel behavior (CUI-06)
- Whether jog lives in its own panel or section of a larger CNC control panel

**Zero-Setting Workflow:**
- Per-axis buttons vs combined controls
- Whether to confirm before zeroing (safety consideration from Phase 1 context: safety paramount)
- G10 L20 implementation details
- WCS selector presentation (dropdown, tabs, buttons)

**MDI Console:**
- Single-line input with history (required per CUI-10)
- Response display format and scrollback
- Whether console is embedded in the CNC panel or a separate dockable panel

**Panel Organization:**
- How to organize jog, WCS, and console within the CNC workspace mode established in Phase 2
- Whether these are separate panels or sections of one unified CNC control panel

### Guiding Principles (from Phase 1 context)
- Safety is paramount — dangerous operations (like zeroing) should have appropriate guards
- Ease of use is secondary priority
- CNCjs is the visual reference for the overall CNC UI feel

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CUI-04 | Jog controls with XYZ +/- buttons, configurable step sizes (0.1, 1, 10, 100mm) | GRBL $J= jog command format, CncController.sendCommand(), step size radio buttons |
| CUI-05 | Keyboard jog shortcuts (arrow keys, page up/down for Z) with focus-aware input routing | ImGui::IsKeyPressed + WantTextInput guard, SDL key handling in UIManager |
| CUI-06 | Continuous jog support using GRBL $J= jog commands with cancel on release | GRBL $J= with high distance + jogCancel() (0x85) on key release |
| CUI-07 | Home cycle button that sends $H and tracks homing state | CncController.sendCommand("$H"), MachineState::Home tracking |
| CUI-08 | Set work zero buttons (X, Y, Z, All) using G10 L20 | G10 L20 P{n} X0 Y0 Z0 format, safety confirmation dialog |
| CUI-09 | Work coordinate system selector (G54-G59) with display of stored offsets | G54-G59 commands, $# to query offsets, parser for [G54:x,y,z] responses |
| CUI-10 | MDI console with single-line G-code input, command history (up/down arrow), and response display | CncController.sendCommand(), onRawLine callback, circular history buffer |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Dear ImGui | 1.90+ (docking) | All UI rendering, input handling, text fields | Already in project, docking branch for panels |
| SDL2 | 2.x | Window management, keyboard event routing | Already in project, provides key events |
| GRBL 1.1 protocol | 1.1 | Jog commands ($J=), homing ($H), zero-set (G10 L20), WCS (G54-G59), offset query ($#) | Target firmware protocol |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| FontAwesome 6 | (bundled) | Icons for jog arrows, home, play/stop buttons | Already loaded in font atlas |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Separate panels per function | Single monolithic CNC control panel | Separate panels allow user to dock/rearrange; monolithic is simpler but inflexible. Separate panels preferred for CNCjs-like feel. |
| ImGui keyboard handling | Raw SDL event processing | ImGui already wraps SDL events and provides WantTextInput guard. Use ImGui's abstraction. |

## Architecture Patterns

### Recommended Project Structure
```
src/
├── core/cnc/
│   ├── cnc_controller.h/.cpp   # Add sendCommand() public method
│   └── cnc_types.h             # Add WcsOffsets struct if needed
├── ui/panels/
│   ├── cnc_jog_panel.h/.cpp    # Jog buttons + homing (CUI-04, CUI-05, CUI-06, CUI-07)
│   ├── cnc_wcs_panel.h/.cpp    # Zero-set + WCS selector (CUI-08, CUI-09)
│   └── cnc_console_panel.h/.cpp # MDI console (CUI-10)
├── managers/
│   └── ui_manager.h/.cpp       # Register new panels, visibility, workspace mode update
└── app/
    └── application_wiring.cpp  # Wire new panels to CncController
```

### Pattern 1: Thread-Safe Command Dispatch
**What:** UI panels call `CncController::sendCommand(string)` which queues the command via `m_pendingStringCmds` (mutex-protected). The IO thread dispatches queued commands in `dispatchPendingCommands()`.
**When to use:** Any time UI needs to send G-code or system commands to GRBL.
**Example:**
```cpp
// In CncController (new public method):
void CncController::sendCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(m_cmdStringMutex);
    m_pendingStringCmds.push_back(cmd + "\n");
}

// In UI panel:
m_cnc->sendCommand("$J=G91 G21 X10 F1000"); // Jog X +10mm at 1000mm/min
```

### Pattern 2: GRBL $J= Jog Command Format
**What:** GRBL 1.1 jog commands use `$J=G91 G21 {axis}{distance} F{feedrate}` for incremental jog. Continuous jog uses a large distance that gets cancelled with 0x85 (jog cancel) on key release.
**When to use:** All jog operations.
**Example:**
```cpp
// Step jog (incremental):
std::string cmd = "$J=G91 G21 X" + std::to_string(stepSize) + " F1000";
m_cnc->sendCommand(cmd);

// Continuous jog (start):
std::string cmd = "$J=G91 G21 X10000 F2000"; // Large distance
m_cnc->sendCommand(cmd);

// Continuous jog (stop on key release):
m_cnc->jogCancel(); // Sends 0x85 real-time command
```

### Pattern 3: Panel Callback Wiring
**What:** New panels receive a `CncController*` pointer via setter injection, same as GCodePanel. CNC callbacks are extended in `application_wiring.cpp` to also notify new panels.
**When to use:** All new CNC UI panels.
**Example:**
```cpp
// In application_wiring.cpp:
auto* jp = m_uiManager->cncJogPanel();
if (jp) jp->setCncController(m_cncController.get());

// Extend existing callback:
cncCb.onStatusUpdate = [gcp, csp, jp, wp](const MachineStatus& status) {
    gcp->onGrblStatus(status);
    if (csp) csp->onStatusUpdate(status);
    if (jp) jp->onStatusUpdate(status);
    if (wp) wp->onStatusUpdate(status);
};
```

### Pattern 4: Focus-Aware Keyboard Jog
**What:** Keyboard jog shortcuts must NOT fire when user is typing in a text field (MDI console, etc). ImGui's `WantTextInput` flag handles this. Check it before processing keyboard jog.
**When to use:** Keyboard jog handler in UIManager or dedicated input handler.
**Example:**
```cpp
// In keyboard handler (UIManager or panel):
if (!ImGui::GetIO().WantTextInput && m_workspaceMode == WorkspaceMode::CNC) {
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        jogAxis(0, +stepSize); // X+
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        jogAxis(0, -stepSize); // X-
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        jogAxis(1, +stepSize); // Y+
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        jogAxis(1, -stepSize); // Y-
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
        jogAxis(2, +stepSize); // Z+
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
        jogAxis(2, -stepSize); // Z-
}
```

### Pattern 5: Continuous Jog with Key Release Detection
**What:** For continuous jog, send a $J= command with large distance on key press, send jogCancel (0x85) on key release. ImGui tracks key states — use `IsKeyDown()` for held state and detect release as the frame where `IsKeyDown()` goes false after being true.
**When to use:** CUI-06 continuous jog.
**Example:**
```cpp
// Track which axis is being continuously jogged
if (m_continuousJogAxis >= 0 && !ImGui::IsKeyDown(m_continuousJogKey)) {
    m_cnc->jogCancel();
    m_continuousJogAxis = -1;
}
```

### Anti-Patterns to Avoid
- **Sending G-code directly on UI thread:** Always use sendCommand() which queues via mutex. Never call m_port.write() from UI.
- **Jog during streaming:** Must guard jog commands — GRBL rejects $J= during Run state. Check machine state before sending jog.
- **Polling for WCS offsets every frame:** Query $# once on connect and after G10 L20 commands, not continuously.
- **Blocking UI for GRBL response:** sendCommand() is fire-and-forget. Use the onRawLine callback to capture responses asynchronously.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Key repeat handling | Custom timers for key repeat | ImGui::IsKeyPressed(key, true) with repeat | ImGui handles OS key repeat intervals correctly |
| Text input with history | Custom text buffer + history | ImGui::InputText with ImGuiInputTextFlags_CallbackHistory | Built-in callback for up/down arrow history navigation |
| Confirmation dialogs | Custom modal windows | ImGui::OpenPopup + BeginPopupModal | Standard ImGui pattern, already used elsewhere in codebase |
| Focus management | Manual focus tracking | ImGui::SetKeyboardFocusHere / IsItemFocused | ImGui manages focus stack correctly |

**Key insight:** ImGui's `InputText` with `ImGuiInputTextFlags_CallbackHistory` handles the MDI command history natively — it fires a callback on Up/Down arrow keys where you provide the history entry. No need for custom key handling.

## Common Pitfalls

### Pitfall 1: Jog Commands During Alarm State
**What goes wrong:** Sending $J= when GRBL is in Alarm state results in error:9 (G-code locked during alarm).
**Why it happens:** User clicks jog button while machine has an active alarm.
**How to avoid:** Check `m_status.state != MachineState::Alarm` before sending jog commands. Show disabled state on buttons.
**Warning signs:** Error:9 responses in console after jog attempts.

### Pitfall 2: Continuous Jog Stuck Running
**What goes wrong:** If the jog cancel (0x85) is missed (e.g., window loses focus during key release), the machine continues moving indefinitely.
**Why it happens:** Key release event lost due to window focus change or application event loop issues.
**How to avoid:** (1) Cancel all continuous jogs when CNC workspace loses focus. (2) Cancel on window unfocus events. (3) Use a watchdog — if key state can't be read for >100ms, cancel.
**Warning signs:** Machine moves farther than expected on continuous jog.

### Pitfall 3: G10 L20 Without Correct WCS Number
**What goes wrong:** G10 L20 P0 zeroes the current WCS, but if user thinks they're zeroing G55 while G54 is active, wrong offsets get modified.
**Why it happens:** P parameter confusion: P0 means "current", P1=G54, P2=G55, etc.
**How to avoid:** Always use explicit P number matching the displayed WCS. Show which WCS is active prominently.
**Warning signs:** Positions jump unexpectedly after zeroing.

### Pitfall 4: $# Response Parsing Complexity
**What goes wrong:** The `$#` command returns multiple lines of WCS offset data (`[G54:0.000,0.000,0.000]`, `[G55:...]`, etc.). If the response parser isn't set up to capture multi-line responses, offsets never populate.
**Why it happens:** Current `processResponse()` handles `[MSG:...]` but not `[G54:...]` format.
**How to avoid:** Extend `processResponse()` to parse `[G54:...]` through `[G59:]` lines and store in a WCS offsets structure.
**Warning signs:** WCS offset display always shows 0,0,0 for all coordinate systems.

### Pitfall 5: MDI Command Sent During Streaming
**What goes wrong:** Sending arbitrary G-code via MDI while a job is streaming corrupts the character-counting buffer tracking.
**Why it happens:** Character-counting protocol assumes only the streaming engine sends G-code lines.
**How to avoid:** Disable MDI send button during streaming. Only allow real-time commands (!, ~, 0x18) during active stream.
**Warning signs:** "Buffer overflow" or desynchronized ok/error counts.

## Code Examples

### GRBL Jog Command Format
```cpp
// GRBL 1.1 jog format: $J=G91 G21 {axis}{distance} F{feedrate}
// G91 = incremental mode (relative to current position)
// G21 = millimeters
// Must include feed rate F parameter

// Step jog examples:
"$J=G91 G21 X10 F1000"     // X +10mm at 1000mm/min
"$J=G91 G21 X-0.1 F500"    // X -0.1mm at 500mm/min
"$J=G91 G21 Z5 F800"       // Z +5mm at 800mm/min

// Continuous jog (large distance, cancelled with 0x85):
"$J=G91 G21 X10000 F2000"  // Will be cancelled before completing
```

### G10 L20 Zero-Set Format
```cpp
// G10 L20 P{n} {axes} — Set work offset so current position becomes specified value
// P0 = current WCS, P1 = G54, P2 = G55, ..., P6 = G59
// Typically set to 0 (making current position the zero point)

"G10 L20 P1 X0"           // Set X zero in G54
"G10 L20 P1 Y0"           // Set Y zero in G54
"G10 L20 P1 Z0"           // Set Z zero in G54
"G10 L20 P1 X0 Y0 Z0"    // Set all axes zero in G54
```

### WCS Selection and Offset Query
```cpp
// Select work coordinate system:
"G54"  // Select WCS 1 (default)
"G55"  // Select WCS 2
"G56"  // Select WCS 3
// ... through G59

// Query all offsets:
"$#"
// Returns lines like:
// [G54:0.000,0.000,0.000]
// [G55:10.000,20.000,0.000]
// ...
// [G59:0.000,0.000,0.000]
// [G28:0.000,0.000,0.000]  (home position)
// [G30:0.000,0.000,0.000]
// [G92:0.000,0.000,0.000]
// [TLO:0.000]              (tool length offset)
// [PRB:0.000,0.000,0.000:0] (probe result)
```

### ImGui InputText with History Callback
```cpp
// MDI console with command history
struct MdiState {
    char inputBuf[256] = "";
    std::vector<std::string> history;
    int historyPos = -1; // -1 = new line, 0..N-1 = browsing history
};

static int mdiCallback(ImGuiInputTextCallbackData* data) {
    auto* state = static_cast<MdiState*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (state->historyPos < (int)state->history.size() - 1)
                state->historyPos++;
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (state->historyPos > -1)
                state->historyPos--;
        }
        const char* entry = (state->historyPos >= 0)
            ? state->history[state->history.size() - 1 - state->historyPos].c_str()
            : "";
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, entry);
    }
    return 0;
}

// In render:
if (ImGui::InputText("##mdi", state.inputBuf, sizeof(state.inputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
        mdiCallback, &state)) {
    std::string cmd(state.inputBuf);
    if (!cmd.empty()) {
        m_cnc->sendCommand(cmd);
        state.history.push_back(cmd);
        state.historyPos = -1;
        state.inputBuf[0] = '\0';
    }
    ImGui::SetKeyboardFocusHere(-1); // Keep focus on input
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Serial byte-at-a-time jog | GRBL $J= jog command (1.1+) | GRBL 1.1 (2017) | Proper motion planning for jog, supports cancel |
| G92 for work zero | G10 L20 for work zero | GRBL best practice | G92 is volatile, G10 L20 writes to EEPROM/flash |
| No jog cancel | 0x85 real-time jog cancel | GRBL 1.1 (2017) | Immediate stop for continuous jog |

**Deprecated/outdated:**
- G92 for setting work zero: Use G10 L20 instead. G92 is volatile and resets on power cycle.
- Simple serial "step" commands: Use $J= which goes through GRBL's motion planner for smooth acceleration.

## Open Questions

1. **Jog feed rate selection**
   - What we know: GRBL $J= requires a feed rate (F parameter). CNCjs typically uses 1000mm/min for step jog.
   - What's unclear: Should feed rate be user-configurable, or is a sensible default sufficient?
   - Recommendation: Use fixed defaults per step size (smaller steps = slower feed). 0.1mm -> 500mm/min, 1mm -> 1000mm/min, 10mm -> 2000mm/min, 100mm -> 3000mm/min. Continuous jog at 2000mm/min. This can be made configurable later.

2. **WCS offset refresh timing**
   - What we know: $# returns all WCS offsets but is not a real-time command — it goes through the planner buffer.
   - What's unclear: How often to refresh. Too frequent = clutters response stream.
   - Recommendation: Query on connect, after each G10 L20 command, and after each WCS switch (G54-G59). Do not poll continuously.

3. **Keyboard shortcut collision with existing shortcuts**
   - What we know: UIManager already uses arrow keys for nothing, but Ctrl+O, Ctrl+S etc are used. CUI-05 wants arrow keys for jog.
   - What's unclear: Whether arrow keys conflict with ImGui widget navigation.
   - Recommendation: Only activate jog shortcuts when in CNC workspace mode AND no text input has focus (WantTextInput check). This is already the pattern used for Ctrl+shortcuts.

## Sources

### Primary (HIGH confidence)
- Codebase analysis: `src/core/cnc/cnc_controller.h/.cpp` — existing command dispatch, jogCancel(), string command queue
- Codebase analysis: `src/ui/panels/cnc_status_panel.h/.cpp` — Phase 2 panel pattern, callback wiring
- Codebase analysis: `src/managers/ui_manager.h/.cpp` — workspace mode, keyboard shortcut handling
- Codebase analysis: `src/app/application_wiring.cpp` — callback wiring pattern for CncController
- GRBL 1.1 protocol documentation — $J= jog format, G10 L20, $# offset query, real-time commands

### Secondary (MEDIUM confidence)
- Dear ImGui InputText CallbackHistory — verified from ImGui API (common pattern, widely documented)
- CNCjs UI patterns — visual reference per user context decisions

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all libraries already in project, no new dependencies
- Architecture: HIGH — direct extension of established patterns from Phase 1 and Phase 2
- Pitfalls: HIGH — GRBL protocol edge cases well-documented, codebase patterns clear
- GRBL protocol: HIGH — $J=, G10 L20, $#, G54-G59 are all GRBL 1.1 standard commands

**Research date:** 2026-02-24
**Valid until:** 2026-03-24 (stable domain, no moving targets)
