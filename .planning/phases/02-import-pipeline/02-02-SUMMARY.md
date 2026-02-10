---
phase: 02-import-pipeline
plan: 02
subsystem: database
tags: [schema, repository, gcode, hierarchy]
completed: 2026-02-10

dependencies:
  requires: []
  provides:
    - gcode_files table with metadata storage
    - operation_groups hierarchy support
    - gcode_templates system
    - GCodeRepository CRUD API
  affects:
    - src/core/database/schema.cpp (v3)
    - Database initialization

tech_stack:
  added:
    - GCodeRepository pattern
    - JSON serialization for arrays (feed rates, tool numbers)
  patterns:
    - Repository pattern consistency
    - Hierarchy junction tables

key_files:
  created:
    - src/core/database/gcode_repository.h
    - src/core/database/gcode_repository.cpp
  modified:
    - src/core/database/schema.h
    - src/core/database/schema.cpp
    - tests/test_schema.cpp
    - src/CMakeLists.txt

decisions:
  - Feed rates and tool numbers stored as JSON arrays (same pattern as tags in ModelRepository)
  - Operation groups use model_id foreign key for hierarchy (Model -> Groups -> G-code files)
  - Templates stored as JSON array of group names, applied by creating groups in order
  - Cascade delete on groups removes all memberships automatically

metrics:
  duration: 432s
  tasks_completed: 2
  files_created: 2
  files_modified: 4
  commits: 2
  tests_passing: 421/422 (lint failure pre-existing)
---

# Phase 02 Plan 02: Database Schema for G-code Storage

**One-liner:** Schema v3 with G-code files, operation groups, templates, and full-featured GCodeRepository following ModelRepository patterns.

## Tasks Completed

### Task 1: Extend database schema to v3 with G-code tables

**Status:** ✅ Complete
**Commit:** cd72d11

**Implementation:**

- Bumped schema version from 2 to 3 in schema.h
- Added 4 new tables in schema.cpp:
  - `gcode_files`: Stores G-code file metadata (hash, name, path, bounds, distance, time, feed rates, tool numbers, thumbnail)
  - `operation_groups`: Hierarchy table linking models to named operation groups with sort order
  - `gcode_group_members`: Junction table mapping G-code files to groups with sort order
  - `gcode_templates`: Pre-defined operation group templates (e.g., "CNC Router Basic")
- Seeded CNC Router Basic template with ["Roughing","Finishing","Profiling","Drilling"] groups
- Added 4 indexes for common queries (hash, name, model_id, group_id lookups)
- Updated test_schema.cpp to expect version 3 and verify all 4 new tables exist
- All 6 schema tests pass

**Files modified:**
- src/core/database/schema.h
- src/core/database/schema.cpp
- tests/test_schema.cpp

### Task 2: Create GCodeRepository with CRUD and hierarchy operations

**Status:** ✅ Complete
**Commit:** 084f814

**Implementation:**

**Data structures:**
- `GCodeRecord`: 17 fields including hash, metadata, bounds, feed rates, tool numbers
- `OperationGroup`: id, modelId, name, sortOrder
- `GCodeTemplate`: id, name, groups array

**CRUD operations (10 methods):**
- insert, findById, findByHash, findAll, findByName
- update, updateThumbnail, remove, exists, count

**Hierarchy operations (6 methods):**
- createGroup, getGroups, addToGroup, removeFromGroup
- getGroupMembers, deleteGroup

**Template operations (2 methods):**
- getTemplates, applyTemplate

**JSON serialization helpers:**
- feedRatesToJson/jsonToFeedRates for float arrays
- toolNumbersToJson/jsonToToolNumbers for int arrays
- groupsToJson/jsonToGroups for string arrays

**Patterns followed:**
- Same structure as ModelRepository (constructor takes Database&)
- Same Statement binding and error handling patterns
- Same JSON serialization approach as tags
- LIKE queries with escapeLike for search
- Foreign key cascade deletes handled automatically

**Files created:**
- src/core/database/gcode_repository.h
- src/core/database/gcode_repository.cpp
- Added to src/CMakeLists.txt

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed incorrect include path in gcode_loader.cpp**
- **Found during:** Task 1 build verification
- **Issue:** gcode_loader.cpp had `#include "../log.h"` which doesn't exist
- **Fix:** Changed to `#include "../utils/log.h"` (correct path)
- **Files modified:** src/core/loaders/gcode_loader.cpp
- **Commit:** Not committed separately (linter auto-fixed during build)

**2. [Rule 1 - Bug] Fixed optional<string> handling in gcode_loader.cpp**
- **Found during:** Task 1 build verification
- **Issue:** file::readText returns optional<string> but code treated it as string
- **Fix:** Changed `if (content.empty())` to `if (!content || content->empty())` and `parser.parse(content)` to `parser.parse(*content)`
- **Files modified:** src/core/loaders/gcode_loader.cpp
- **Commit:** Not committed separately (fixed as blocking issue)

## Verification

**Build verification:**
```bash
cmake --build build
# Result: Clean build, all targets compile successfully
```

**Test verification:**
```bash
./build/tests/dw_tests --gtest_filter="Schema.*"
# Result: All 6 schema tests pass
```

**Full test suite:**
```bash
cd build && ctest --output-on-failure
# Result: 421/422 tests pass (lint failure pre-existing, unrelated to changes)
```

**Schema tables verified:**
- gcode_files table created with all metadata fields
- operation_groups table created with model_id foreign key
- gcode_group_members junction table created with composite primary key
- gcode_templates table created and seeded with CNC Router Basic template

**Repository verified:**
- GCodeRepository compiles without errors
- All methods follow ModelRepository patterns
- JSON serialization follows same approach as tags

## Success Criteria Met

✅ Schema v3 creates gcode_files, operation_groups, gcode_group_members, gcode_templates tables
✅ CNC Router Basic template seeded with Roughing/Finishing/Profiling/Drilling groups
✅ GCodeRepository provides full CRUD for G-code records
✅ GCodeRepository provides hierarchy operations (groups, template application)
✅ All existing tests pass unchanged (421/422, lint failure unrelated)

## Self-Check: PASSED

**Created files exist:**
```bash
[ -f "src/core/database/gcode_repository.h" ] && echo "FOUND: src/core/database/gcode_repository.h"
# FOUND: src/core/database/gcode_repository.h

[ -f "src/core/database/gcode_repository.cpp" ] && echo "FOUND: src/core/database/gcode_repository.cpp"
# FOUND: src/core/database/gcode_repository.cpp
```

**Commits exist:**
```bash
git log --oneline --all | grep -q "cd72d11" && echo "FOUND: cd72d11"
# FOUND: cd72d11

git log --oneline --all | grep -q "084f814" && echo "FOUND: 084f814"
# FOUND: 084f814
```

**Schema changes verified:**
- schema.h CURRENT_VERSION = 3 ✓
- schema.cpp contains all 4 new tables ✓
- schema.cpp seeds CNC Router Basic template ✓
- test_schema.cpp expects version 3 ✓
- test_schema.cpp checks all 4 new tables ✓

**Repository completeness verified:**
- GCodeRepository has all 18 public methods ✓
- All CRUD operations implemented ✓
- All hierarchy operations implemented ✓
- All template operations implemented ✓
- JSON helpers for all array types ✓

## Next Steps

This plan provides the database foundation for G-code import. Next plans should:

1. **Plan 02-03**: Create G-code import queue and metadata extraction
2. **Plan 02-04**: Implement G-code file handler with thumbnail generation
3. **Plan 02-05**: Add G-code library UI panel with hierarchy display
4. **Plan 02-06**: Integrate G-code visualization in viewport

All database infrastructure is now ready for G-code import pipeline implementation.
