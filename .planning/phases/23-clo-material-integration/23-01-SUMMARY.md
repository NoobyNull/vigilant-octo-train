---
phase: 23-clo-material-integration
plan: 01
subsystem: optimizer
tags: [waste-breakdown, multi-stock, material-grouping, tdd]

requires:
  - phase: 20-materials-data-layer
    provides: MaterialManager, StockSize, MaterialRecord types
provides:
  - WasteBreakdown struct with scrap pieces, kerf totals, dollar values
  - computeWasteBreakdown() function for CutPlan analysis
  - MultiStockOptimizer that groups parts by materialId and selects optimal stock sizes
  - Part.materialId field for material assignment
affects: [23-clo-material-integration, 25-project-costing]

tech-stack:
  added: []
  patterns: [bounding-box scrap estimation, cost-minimizing stock selection]

key-files:
  created:
    - src/core/optimizer/waste_breakdown.h
    - src/core/optimizer/waste_breakdown.cpp
    - src/core/optimizer/multi_stock_optimizer.h
    - src/core/optimizer/multi_stock_optimizer.cpp
    - tests/test_waste_breakdown.cpp
    - tests/test_multi_stock_optimizer.cpp
  modified:
    - src/core/optimizer/sheet.h
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Scrap estimation uses bounding-box remainders (right strip + bottom strip) rather than full free-rect tracking"
  - "Multi-stock optimizer prefers complete plans over cheaper incomplete ones"

patterns-established:
  - "Material grouping: parts grouped by materialId, each group optimized independently"
  - "Cost comparison: sheetsUsed * sheetCost across stock sizes per group"

requirements-completed: [CLO-02, CLO-03]

duration: 12min
completed: 2026-03-05
---

# Plan 23-01: Multi-Stock Optimizer and Waste Breakdown Summary

**WasteBreakdown computes scrap rectangles, kerf loss, and dollar values; MultiStockOptimizer groups parts by material and selects cheapest stock sizes**

## Performance

- **Duration:** 12 min
- **Tasks:** 2 (RED + GREEN TDD cycle)
- **Files modified:** 9

## Accomplishments
- Part struct extended with materialId field (backward compatible, default 0)
- computeWasteBreakdown produces scrap pieces with dimensions, kerf area totals, and dollar values proportional to sheet cost
- optimizeMultiStock groups parts by materialId, tries all stock sizes per material group, picks minimum cost with preference for complete plans
- 10 new tests, 911 total tests passing

## Task Commits

1. **Task 1: RED** - `900fb65` (test) - Failing tests for waste breakdown and multi-stock optimizer
2. **Task 2: GREEN** - `fa8be6c` (feat) - Implement waste breakdown and multi-stock optimizer

## Files Created/Modified
- `src/core/optimizer/sheet.h` - Added materialId field to Part struct
- `src/core/optimizer/waste_breakdown.h` - ScrapPiece and WasteBreakdown structs, computeWasteBreakdown declaration
- `src/core/optimizer/waste_breakdown.cpp` - Bounding-box scrap estimation, kerf calculation, dollar value computation
- `src/core/optimizer/multi_stock_optimizer.h` - MaterialGroup, MultiStockResult, optimizeMultiStock declaration
- `src/core/optimizer/multi_stock_optimizer.cpp` - Material grouping, per-group stock size selection, cost minimization
- `tests/test_waste_breakdown.cpp` - 5 tests: single part, kerf, dollar values, multi-sheet, empty plan
- `tests/test_multi_stock_optimizer.cpp` - 5 tests: grouping, cheaper sheet, fallback, unassigned, cost aggregation

## Decisions Made
- Used bounding-box remainder approach for scrap estimation (right strip + bottom strip from placement bounds) rather than tracking exact guillotine free rectangles -- simpler and sufficient for cost estimation
- Multi-stock optimizer prefers complete plans (all parts placed) over cheaper but incomplete plans

## Deviations from Plan
None - plan executed as written

## Issues Encountered
- MultipleSheets test required constructing a CutPlan manually since the optimizer packs efficiently enough to fit test parts on single sheets

## Next Phase Readiness
- WasteBreakdown and MultiStockResult types ready for UI integration (Plan 23-02)
- Part.materialId ready for material assignment in CutOptimizerPanel

---
*Phase: 23-clo-material-integration*
*Completed: 2026-03-05*
