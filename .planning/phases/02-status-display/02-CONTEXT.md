# Phase 2: Status Display - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Real-time DRO showing work/machine XYZ position, machine state indicator, feed rate, and spindle RPM. Read-only display only — no controls, jog, or interaction (that's Phase 3). Requirements: CUI-01, CUI-02, CUI-03.

</domain>

<decisions>
## Implementation Decisions

### Panel Placement
- Dedicated dockable ImGui panel (like existing Library/Properties panels)
- Always visible — shows "Disconnected" when no machine connected
- CNC workspace mode: switching to CNC mode rearranges layout to show controller panels, hides library/materials panels
- This means the app needs a workspace mode switcher (Model mode vs CNC mode)

### Visual Reference
- CNCjs-inspired status display — modern, widget-based look
- Clean DRO with clear position readout
- Status indicators should feel polished, not raw text dumps

### DRO Layout
- Claude's discretion on exact layout — large digits for XYZ position, clear work vs machine coordinate distinction
- Feed rate and spindle RPM should be clearly visible but secondary to position

### State Indicator
- Claude's discretion on style — color-coded is expected (green=Idle, yellow=Hold, red=Alarm, blue=Run, etc.)
- Must be immediately obvious at a glance from across a shop

### Claude's Discretion
- Exact DRO digit size and font choice within ImGui constraints
- Work vs machine position toggle or dual display approach
- Feed/spindle display format (numeric, gauge, or both)
- State indicator implementation (badge, bar, background color)
- Workspace mode switching mechanism (menu, toolbar button, keyboard shortcut)

</decisions>

<specifics>
## Specific Ideas

- CNCjs is the visual reference — modern web-based look adapted to Dear ImGui
- Workspace mode concept: "Model mode" (current layout with library, properties, viewport) vs "CNC mode" (DRO, jog, console, status panels around viewport)
- Panel should be useful even when disconnected — shows "Disconnected" state, ready to connect

</specifics>

<deferred>
## Deferred Ideas

- None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-status-display*
*Context gathered: 2026-02-24*
