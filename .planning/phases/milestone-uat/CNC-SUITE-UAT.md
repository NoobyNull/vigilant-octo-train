---
status: testing
phase: CNC-Controller-Suite (Phases 1-8)
source: 02-01-SUMMARY.md, 02-02-SUMMARY.md, 03-01-SUMMARY.md, 03-02-SUMMARY.md, 03-03-SUMMARY.md, 03-04-SUMMARY.md, 04-01-SUMMARY.md, 04-02-SUMMARY.md, 05-01-SUMMARY.md, 05-02-SUMMARY.md, 06-03-SUMMARY.md, 06-04-SUMMARY.md, 07-CONTEXT.md, 08-SUMMARY.md
started: 2026-02-25T00:00:00Z
updated: 2026-02-25T00:00:00Z
---

## Current Test

number: 1
name: CNC Workspace Mode Switching
expected: |
  Pressing Ctrl+2 switches to CNC workspace mode. CNC panels (Status, Jog, Console, etc.) become visible.
  Pressing Ctrl+1 switches back to Model workspace mode with Library and Properties panels.
  View menu shows workspace mode toggles.
awaiting: user response

## Tests

### 1. CNC Workspace Mode Switching
expected: Pressing Ctrl+2 switches to CNC workspace mode with CNC panels visible. Ctrl+1 returns to Model mode.
result: [pending]

### 2. CNC Status Panel (DRO)
expected: In CNC mode, Status panel shows color-coded machine state banner, large XYZ work position digits, machine position secondary readout, feed rate and spindle RPM. Shows "Disconnected" placeholder when no CNC connected.
result: [pending]

### 3. Jog Control Panel
expected: CNCjs-style jog cross layout with X+/X-/Y+/Y-/Z+/Z- buttons. Step size radio buttons (0.1, 1, 10, 100mm). Home All and Unlock buttons. Disabled/placeholder when disconnected.
result: [pending]

### 4. MDI Console Panel
expected: Command input field at bottom with Enter to send. Output area with color-coded lines (cyan=sent, white=response, red=error, yellow=info). Up/Down arrow history navigation.
result: [pending]

### 5. Work Zero / WCS Panel
expected: Zero X, Y, Z, All buttons. G54-G59 coordinate system selector with active highlight. Stored offsets table showing all 6 WCS offset values.
result: [pending]

### 6. Tool & Material Panel
expected: Tool selector dropdown populated from tool database. Material selector dropdown. When both selected, auto-calculated feeds/speeds display (RPM, Feed rate, Plunge rate, Stepdown, Stepover, Chip load).
result: [pending]

### 7. Job Progress Panel
expected: Shows elapsed time (HH:MM:SS), current/total line count, progress bar with percentage. Remaining time estimate appears after initial data collected.
result: [pending]

### 8. Safety Panel
expected: Pause button (amber), Resume button (green), Abort button (red) -- all state-gated (disabled when not applicable). Safety warning text. Sensor pin display (X/Y/Z limits, Probe, Door). Resume-from-line button with dialog.
result: [pending]

### 9. Firmware Settings Panel
expected: In CNC mode, Settings panel shows grouped GRBL settings table with collapsible sections (General, Motion, Limits & Homing, Spindle, Steps/mm, Feed Rates, Acceleration, Max Travel). Toolbar with Read/Write/Backup/Restore. Machine Tuning tab with focused per-axis view.
result: [pending]

### 10. Macros Panel
expected: Macro list with Play/Edit/Delete buttons per macro. 3 built-in macros present (Homing Cycle, Probe Z, Return to Zero) marked as non-deletable. Add Macro button. Inline edit area with name, G-code editor, shortcut fields.
result: [pending]

### 11. View Menu Integration
expected: All CNC panels listed in View menu with toggles: CNC Status, Jog Control, MDI Console, Work Zero/WCS, Tool & Material, Job Progress, Safety, Firmware Settings, Macros.
result: [pending]

### 12. Build & Tests Pass
expected: `cmake --build build` completes without errors. `./build/tests/dw_tests` reports 715 tests passing with 0 failures.
result: [pending]

## Summary

total: 12
passed: 0
issues: 0
pending: 12
skipped: 0

## Gaps

[none yet]
