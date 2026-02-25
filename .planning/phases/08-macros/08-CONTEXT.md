# Phase 8: Macros - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Macro storage in SQLite, sequential G-code execution, and built-in macros for common operations (homing, Z-probe, return to zero). Requirements: FWC-08, FWC-09, FWC-10.

</domain>

<decisions>
## Implementation Decisions

### Macro Storage (FWC-08)
- SQLite table: id, name, gcode_content, keyboard_shortcut (optional), sort_order
- Macros are sender-side only — GRBL has no on-controller macro system
- User can create, edit, delete, reorder macros

### Macro Execution (FWC-09)
- Send G-code lines sequentially via CncController::sendCommand()
- Execution visible in the MDI console (Phase 3)
- Respect machine state guards — don't execute macros during streaming or alarm

### Built-in Macros (FWC-10)
- Homing: $H
- Probe Z: G38.2 Z-50 F100 (with configurable probe feed rate and distance)
- Return to Zero: G90 G0 X0 Y0 then G0 Z0 (Z last for safety — raise Z first if below zero)

### Claude's Discretion
- Macro panel layout (list with run buttons, grid, toolbar-style)
- Whether macros panel is standalone or section of existing CNC panel
- Keyboard shortcut binding UI
- Z-probe configuration (touch plate thickness input, default values)
- Whether to support macro variables/parameters or keep it simple (plain G-code only)

</decisions>

<specifics>
## Specific Ideas

- Keep macros simple — plain G-code text, no variable substitution for v1
- bCNC's macro system is the reference but DW should be simpler
- Built-in macros should be non-deletable but editable (user can customize)
- sendCommand() was added in Phase 3

</specifics>

<deferred>
## Deferred Ideas

- Parametric macros with variable substitution — future enhancement
- M6 tool change macro interception — v2 requirement
- Macro import/export — nice-to-have, not v1

</deferred>

---

*Phase: 08-macros*
*Context gathered: 2026-02-24*
