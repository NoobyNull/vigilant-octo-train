---
phase: 26-estimate-order-views
plan: 01
subsystem: ui
tags: [imgui, costing, margin, orders]

requires:
  - phase: 24-project-costing-engine
    provides: CostingEngine, CostingEntry, category totals
  - phase: 25-clo-to-costing-pipeline
    provides: CLO auto-push integration
provides:
  - Sale price input with margin calculation in CostingPanel
  - Estimate vs Order view toggle
  - Order conversion and persistence
  - Color-coded margin display
affects: [26-estimate-order-views]

tech-stack:
  added: []
  patterns:
    - "CostViewMode enum for view state switching"
    - "Theme-derived color coding for profit/loss indicators"

key-files:
  created: []
  modified:
    - src/ui/panels/cost_panel.h
    - src/ui/panels/cost_panel.cpp

key-decisions:
  - "Used CostViewMode enum (Estimate/Order) for clean view state management"
  - "Order view renders entries as read-only Text instead of InputFloat for locked state"
  - "Margin colors derived from ImGuiCol_PlotHistogram with channel overrides, matching variance pattern"

patterns-established:
  - "View mode toggle pattern: two buttons with active highlight via ImGuiCol_ButtonActive"

requirements-completed: [COST-08, COST-11]

duration: 5min
completed: 2026-03-05
---

# Phase 26 Plan 01: Sale Price, Margin & Estimate/Order Toggle Summary

**Sale price input with real-time margin calculation, color-coded profit/loss display, and Estimate/Order view toggle with frozen order snapshots persisted to orders.json**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-05T05:00:00Z
- **Completed:** 2026-03-05T05:05:00Z
- **Tasks:** 1 (+ 1 checkpoint auto-approved)
- **Files modified:** 2

## Accomplishments
- Sale price input with margin display (color-coded green for profit, red for loss)
- Estimate/Order view toggle with visual mode indicator
- Convert-to-order flow freezing all values as read-only snapshot
- Order persistence via ProjectCostingIO (orders.json)
- Orders load on application restart via setCostingDir()

## Task Commits

1. **Task 1: Add sale price input, margin display, and estimate/order view toggle** - `3cefc58` (feat)
2. **Task 2: Verify sale price, margin, and estimate/order toggle** - auto-approved checkpoint

## Files Created/Modified
- `src/ui/panels/cost_panel.h` - Added CostViewMode enum, view mode/order state members, new render methods
- `src/ui/panels/cost_panel.cpp` - Implemented sale price input, margin display, view toggle, order view, conversion flow

## Decisions Made
- Used CostViewMode enum for clean view state management
- Order view uses ImGui::Text for all values (read-only) rather than BeginDisabled on InputFloat
- Margin colors derived from theme style (ImGuiCol_PlotHistogram with channel overrides), consistent with existing variance display

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Ready for plan 26-02 (text export for estimates and orders)
- CostViewMode and order state now available for export button integration

---
*Phase: 26-estimate-order-views*
*Completed: 2026-03-05*
