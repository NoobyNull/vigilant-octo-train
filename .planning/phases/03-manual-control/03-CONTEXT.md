# Phase 3: Manual Control - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Jog controls, homing, work zero setting, G54-G59 coordinate system switching, and MDI console. Interactive machine control — the operator moves the machine through this UI. Requirements: CUI-04 through CUI-10.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
User deferred all implementation decisions to Claude. The following areas are fully flexible:

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

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches. Follow CNCjs-inspired design language from Phase 2.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-manual-control*
*Context gathered: 2026-02-24*
