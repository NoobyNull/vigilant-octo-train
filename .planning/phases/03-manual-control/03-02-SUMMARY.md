# Plan 03-02 Summary: CncConsolePanel (MDI)

**Status:** Complete
**Completed:** 2026-02-24

## What Was Built

CncConsolePanel — an MDI (Manual Data Input) console for sending G-code commands and viewing color-coded GRBL responses.

### Key Features
- **Command Input:** InputText with EnterReturnsTrue, sends commands via CncController::sendCommand()
- **Command History:** Up/Down arrow keys navigate through last 100 commands via ImGuiInputTextFlags_CallbackHistory
- **Color-Coded Display:** Sent lines (cyan), received (white), errors (red), info/status (yellow)
- **Auto-Scroll:** Scrollable child region with automatic scroll-to-bottom on new lines
- **Streaming Guard:** Input disabled during active job with tooltip explanation
- **Line Buffer:** Deque capped at 500 lines to bound memory

### Key Files

| File | Purpose |
|------|---------|
| `src/ui/panels/cnc_console_panel.h` | Panel class with ConsoleLine deque, history vector |
| `src/ui/panels/cnc_console_panel.cpp` | Full implementation (~180 lines) |

### Design Decisions
- Forward-declared ImGuiInputTextCallbackData in header to avoid imgui.h include in header
- Used deque for line buffer (efficient front removal when capped)
- Connected to onRawLine, onError, and onAlarm callbacks for comprehensive GRBL output display
- Connection state changes shown as Info-type lines

### Self-Check: PASSED
- [x] InputText sends commands through CncController
- [x] Up/Down arrow command history works
- [x] Color-coded line display (4 types)
- [x] Auto-scroll on new output
- [x] Disabled during streaming

## Commits
- `feat: add MDI console panel for direct G-code command entry (CUI-10)`
