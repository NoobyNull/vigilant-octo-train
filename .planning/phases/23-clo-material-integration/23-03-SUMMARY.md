---
phase: 23-clo-material-integration
plan: 03
subsystem: optimizer
tags: [clo-result-file, json-persistence, stock-size-id, tdd]

requires:
  - phase: 23-clo-material-integration
    provides: MultiStockResult, WasteBreakdown types from plan 23-01
provides:
  - CloResultFile class for saving/loading CLO results as .clo.json
  - stock_size_id references in serialized results
  - Waste breakdown persistence through save/load cycle
affects: [25-project-costing]

tech-stack:
  added: []
  patterns: [.clo.json extension, JSON serialization mirroring cut_list_file pattern]

key-files:
  created:
    - src/core/optimizer/clo_result_file.h
    - src/core/optimizer/clo_result_file.cpp
    - tests/test_clo_result_file.cpp
  modified:
    - src/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "Used .clo.json extension to distinguish from cut_list .json files"
  - "Followed exact same pattern as CutListFile for sanitization and file I/O"

patterns-established:
  - "CLO result files use .clo.json extension in project folder"

requirements-completed: [DATA-04]

duration: 8min
completed: 2026-03-05
---

# Plan 23-03: CLO Result File Persistence Summary

**CloResultFile saves MultiStockResult to project folder JSON with stock_size_id references and full waste breakdown**

## Performance

- **Duration:** 8 min
- **Tasks:** 2 (RED + GREEN TDD cycle)
- **Files modified:** 5

## Accomplishments
- CloResultFile class with save/load/list/remove API
- stock_size_id from database stored per group in JSON
- WasteBreakdown (scrap pieces, kerf, dollar values) persists through save/load cycle
- Parts with materialId preserved through round-trip
- 6 new tests, 917 total tests passing

## Task Commits

1. **Task 1: RED** - `6a33454` (test) - Failing tests for CLO result file persistence
2. **Task 2: GREEN** - `5c2825d` (feat) - Implement CLO result file persistence

## Files Created/Modified
- `src/core/optimizer/clo_result_file.h` - CloResultFile class, CloResultMeta struct
- `src/core/optimizer/clo_result_file.cpp` - Full JSON serialization/deserialization following cut_list_file pattern
- `tests/test_clo_result_file.cpp` - 6 tests: save/load, stock IDs, waste breakdown, materialId, list, empty

## Decisions Made
- Used .clo.json extension to avoid collision with cut_list .json files
- Copied sanitizeFilename logic from CutListFile rather than extracting to shared utility (minimal duplication)

## Deviations from Plan
None - plan executed as written

## Issues Encountered
None

## Next Phase Readiness
- CLO results can be saved to and loaded from project folder
- Ready for costing pipeline integration (Phase 25)

---
*Phase: 23-clo-material-integration*
*Completed: 2026-03-05*
