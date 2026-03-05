---
phase: 24-project-costing-engine
plan: 02
subsystem: ui
tags: [costing, imgui, panel, categories, estimated-actual]

requires:
  - phase: 24-project-costing-engine
    provides: CostingEngine class with entry CRUD, material snapshots, category totals
provides:
  - CostingPanel with 5 category-grouped sections
  - Estimated vs actual side-by-side columns
  - Labor and overhead add forms
  - Variance display with color coding
affects: [24-03, cost-panel]

tech-stack:
  added: []
  patterns: [category-grouped-rendering, engine-record-sync]

key-files:
  created: []
  modified:
    - src/ui/panels/cost_panel.h
    - src/ui/panels/cost_panel.cpp

key-decisions:
  - "Renamed CostCategory namespace to CostCat to avoid conflict with CostCategory enum in cost_repository.h"
  - "syncEngineFromRecord/syncRecordFromEngine bridge CostItem (DB) and CostingEntry (engine)"

patterns-established:
  - "Category string mapping: Material<->material, Labor<->labor, Tool<->tooling, Other<->overhead"

requirements-completed: [COST-05, COST-06, COST-07, COST-10]

duration: 10min
completed: 2026-03-04
---

# Plan 24-02: CostingPanel Category-Grouped Display Summary

**CostingPanel rewritten with 5 category sections, estimated vs actual columns, and inline add forms for labor/overhead/material**

## Performance

- **Duration:** 10 min
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- CostingPanel renders 5 collapsible category sections (Material, Tooling, Consumable, Labor, Overhead)
- Each entry row shows Name, Qty, Rate, Estimated, Actual with inline editing
- Add forms for labor (name + $/hr + hours), overhead (name + amount + notes), material (name + price + qty + unit)
- Category subtotals and grand total with estimated/actual/variance
- Variance color-coded green (under budget) / red (over budget)
- CostingEngine integrated with sync to/from CostingRecord for DB compatibility

## Task Commits

1. **Task 1: Integrate CostingEngine with category-grouped display** - `ead28fb` (feat)
2. **Task 2: Verify UIManager wiring** - verified within Task 1 build (no separate commit needed)

## Files Created/Modified
- `src/ui/panels/cost_panel.h` - Added CostingEngine member, category methods, form buffers
- `src/ui/panels/cost_panel.cpp` - Complete rewrite of editor section with category-grouped rendering
- `src/core/project/project_costing_io.h` - Renamed CostCategory namespace to CostCat
- `src/core/project/costing_engine.cpp` - Updated CostCat references
- `tests/test_costing_engine.cpp` - Updated CostCat references

## Decisions Made
- Renamed CostCategory namespace to CostCat to avoid enum conflict with cost_repository.h
- Material snapshot entries show read-only price with tooltip

## Deviations from Plan
None - plan executed as written, with CostCategory->CostCat rename as necessary fix

## Issues Encountered
None

## User Setup Required
None

## Next Phase Readiness
- Panel ready for persistence wiring (plan 24-03)
- syncEngineFromRecord/syncRecordFromEngine enable save/load integration

---
*Plan: 24-02 of 24-project-costing-engine*
*Completed: 2026-03-04*
