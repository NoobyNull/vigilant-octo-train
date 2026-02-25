# Phase 4: Tool Integration - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Connect tool database and material system to the feeds/speeds calculator within the CNC controller context. Operator selects a tool geometry and wood species, sees calculated cutting parameters. This is about CALCULATING reference parameters — not physical tool changing or M6 handling. Requirements: TAC-01, TAC-02, TAC-03.

</domain>

<decisions>
## Implementation Decisions

### Core Workflow
- User selects tool geometry from existing ToolDatabase (Vectric .vtdb compatible)
- User selects wood species from existing MaterialManager (Janka-based hardness)
- Existing ToolCalculator produces RPM, feed rate, plunge rate, stepdown, stepover
- Results displayed in a reference panel — operator reads these while setting up or running a job

### What This Is NOT
- NOT a tool change system — no M6 interception, no tool swap prompts
- NOT automatic — operator manually reads the calculated values and applies them
- The calculated parameters are advisory/reference only

### Panel Placement
- New panel or section within the CNC workspace mode (established in Phase 2)
- Should be visible alongside the DRO and jog controls
- Tool and material selection should persist across sessions (SQLite)

### Claude's Discretion
- Whether tool/material selection is its own panel or integrated into CncStatusPanel
- UI layout for the calculated parameters display
- How to present the tool database browser in the CNC context (reuse ToolBrowserPanel or simplified selector)
- Whether to show a compact material dropdown or full material browser

</decisions>

<specifics>
## Specific Ideas

- Existing ToolCalculator (src/core/cnc/tool_calculator.h) already does all the math — this phase wires it to UI
- Existing ToolDatabase has full CRUD and tree-view browsing
- Existing MaterialManager has 31+ built-in species with Janka hardness
- The calculator classifies materials into hardness bands and recommends SFM, chip load, RPM

</specifics>

<deferred>
## Deferred Ideas

- M6 tool change interception — v2 requirement (TCH-01, TCH-02)
- Feed rate deviation warning — Phase 5 (TAC-04)

</deferred>

---

*Phase: 04-tool-integration*
*Context gathered: 2026-02-24*
