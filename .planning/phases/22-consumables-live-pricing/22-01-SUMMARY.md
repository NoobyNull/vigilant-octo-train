---
phase: 22-consumables-live-pricing
plan: 01
subsystem: database
tags: [sqlite, repository, tdd, rate-categories, cost-estimation]

requires:
  - phase: 20-data-foundation-rename
    provides: "Database wrapper, schema system, repository patterns"
provides:
  - "RateCategory struct with computeCost() method"
  - "RateCategoryRepository with full CRUD + getEffectiveRates()"
  - "Schema v16 migration adding rate_categories table"
affects: [22-02, cost-panel, project-costing]

tech-stack:
  added: []
  patterns: ["Global/project-override merge via name-keyed map in getEffectiveRates"]

key-files:
  created:
    - src/core/database/rate_category_repository.h
    - src/core/database/rate_category_repository.cpp
    - tests/test_rate_category_repository.cpp
  modified:
    - src/core/database/schema.h
    - src/core/database/schema.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt
    - tests/test_schema.cpp
    - tests/test_database.cpp

key-decisions:
  - "No FK constraint on project_id — sentinel value 0 means global, avoids FK violation on non-existent row"
  - "UNIQUE index on (name, project_id) prevents duplicate rate names within same scope"

patterns-established:
  - "Sentinel project_id=0 for global defaults, >0 for project-specific overrides"
  - "getEffectiveRates merges global + project rates via name-keyed unordered_map"

requirements-completed: [MATL-05, DATA-03, COST-03, COST-04]

duration: 4min
completed: 2026-03-05
---

# Phase 22 Plan 01: Rate Category Repository (TDD) Summary

**RateCategory struct and repository with global/per-project override merge logic, schema v16 migration, 11 TDD tests**

## Performance

- **Duration:** 4 min
- **Started:** 2026-03-05T03:10:41Z
- **Completed:** 2026-03-05T03:14:29Z
- **Tasks:** 2 (RED + GREEN)
- **Files modified:** 9

## Accomplishments
- RateCategory struct with name, ratePerCuUnit, projectId, notes, and computeCost() method
- RateCategoryRepository with full CRUD: insert, findById, findGlobal, findByProject, getEffectiveRates, update, remove, removeByProject, count
- getEffectiveRates() merges global defaults with project overrides by name — project rate replaces global when names match
- Schema v16 migration with rate_categories table and unique (name, project_id) index
- 11 tests covering all CRUD operations, override logic, and cost computation

## Task Commits

Each task was committed atomically:

1. **Task 1: RED - Failing tests** - `e66fc25` (test)
2. **Task 2: GREEN - Implementation + schema v16** - `35e2e95` (feat)

## Files Created/Modified
- `src/core/database/rate_category_repository.h` - RateCategory struct + RateCategoryRepository class
- `src/core/database/rate_category_repository.cpp` - Full CRUD implementation with name-keyed merge
- `tests/test_rate_category_repository.cpp` - 11 test cases covering all operations
- `src/core/database/schema.h` - Bumped CURRENT_VERSION to 16
- `src/core/database/schema.cpp` - v16 migration + rate_categories table in createTables
- `src/CMakeLists.txt` - Added rate_category_repository.cpp to DW_SOURCES
- `tests/CMakeLists.txt` - Added test + dep to DW_TEST_SOURCES/DW_TEST_DEPS
- `tests/test_schema.cpp` - Updated version expectations to 16
- `tests/test_database.cpp` - Updated version expectations to 16

## Decisions Made
- Removed FOREIGN KEY constraint on project_id — projectId=0 is a sentinel for "global" and no row with id=0 exists in projects table. Application logic handles referential integrity for project-specific rates.
- UNIQUE index on (name, project_id) prevents duplicate rate category names within the same scope.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Removed FK constraint on project_id**
- **Found during:** Task 2 (GREEN - Implementation)
- **Issue:** FK constraint on project_id referencing projects(id) caused insert failures for global rates (project_id=0) since no project with id=0 exists
- **Fix:** Removed FOREIGN KEY clause from both createTables and v16 migration
- **Files modified:** src/core/database/schema.cpp
- **Verification:** All 11 tests pass, 857 total tests pass
- **Committed in:** 35e2e95 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Essential fix for correctness. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Rate category data model complete and tested
- Ready for Plan 22-02: CostingPanel UI integration with rate category management

---
*Phase: 22-consumables-live-pricing*
*Completed: 2026-03-05*
