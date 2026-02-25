# Plan 03-01 Summary: sendCommand() + CncJogPanel

**Status:** Complete
**Completed:** 2026-02-24

## What Was Built

Added `sendCommand()` public API to CncController and created CncJogPanel for XYZ jog control with step sizes, homing, and unlock.

### Key Features
- **sendCommand():** Thread-safe method on CncController that queues arbitrary G-code/system commands for IO thread dispatch via mutex-protected string queue
- **Jog Buttons:** CNCjs-style cross layout with X+/X-/Y+/Y- and separate Z+/Z- buttons
- **Step Size Selector:** Radio buttons for 0.1, 1, 10, 100mm increments with matched feed rates (500-3000 mm/min)
- **Homing:** Home All button sending $H, disabled during alarm state
- **Unlock:** Lock-open button sending $X for alarm clear
- **Keyboard Jog:** Arrow keys for X/Y, Page Up/Down for Z; Shift+key for continuous jog (large-distance $J= cancelled with 0x85 on key release)
- **Disconnected State:** Clean placeholder with unlink icon

### Key Files

| File | Purpose |
|------|---------|
| `src/core/cnc/cnc_controller.h` | Added sendCommand() declaration |
| `src/core/cnc/cnc_controller.cpp` | sendCommand() implementation |
| `src/ui/panels/cnc_jog_panel.h` | Panel class with step sizes, keyboard jog state |
| `src/ui/panels/cnc_jog_panel.cpp` | Full rendering implementation (~210 lines) |
| `src/ui/icons.h` | Added arrow, home, lock-open icons |

### Design Decisions
- Used snprintf for icon+text button labels (Icons::Home is const char*, not string literal)
- Shift+arrow for continuous jog (CNCjs-inspired) rather than hold-to-continuous
- Continuous jog uses large-distance $J= command cancelled with jogCancel() (0x85) on release
- Feed rates scale with step size for intuitive feel

### Self-Check: PASSED
- [x] sendCommand() queues commands thread-safely
- [x] Jog buttons send correctly formatted $J= commands
- [x] Step size selector with 4 increments
- [x] Home/Unlock buttons with state guards
- [x] Keyboard jog with arrow keys and continuous jog

## Commits
- `feat: add sendCommand() to CncController for queuing G-code commands`
- `feat: add CncJogPanel with XYZ jog buttons, step sizes, homing, keyboard jog (CUI-04/05/06/07)`
