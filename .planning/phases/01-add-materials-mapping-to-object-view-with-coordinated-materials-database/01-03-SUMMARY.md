---
phase: 01-add-materials-mapping-to-object-view-with-coordinated-materials-database
plan: 03
subsystem: materials
tags: [materials, database, archive, miniz, nlohmann-json, cnc, wood-species]

requires:
  - phase: 01-01
    provides: MaterialRecord struct, MaterialRepository CRUD, materials table in SQLite schema v4

provides:
  - getDefaultMaterials() returning 32 built-in species (8 hardwood, 7 softwood, 7 domestic, 10 composite)
  - getMaterialsDir() in app_paths for managed .dwmat storage
  - MaterialManager with seedDefaults/importMaterial/exportMaterial/assignMaterialToModel
  - MaterialArchive create/load updated to support metadata-only archives (no texture)
affects: [01-04, 01-05, 01-06]

tech-stack:
  added: []
  patterns: [manager-pattern, seed-on-first-run, copy-to-managed-dir]

key-files:
  created:
    - src/core/materials/default_materials.h
    - src/core/materials/default_materials.cpp
    - src/core/materials/material_manager.h
    - src/core/materials/material_manager.cpp
    - tests/test_material_manager.cpp
  modified:
    - src/core/paths/app_paths.h
    - src/core/paths/app_paths.cpp
    - src/core/materials/material_archive.cpp
    - src/CMakeLists.txt
    - tests/CMakeLists.txt
    - cmake/Dependencies.cmake

key-decisions:
  - "32 built-in materials covering hardwoods, softwoods, domestic woods, composites/metals/plastics with real Janka ratings"
  - "seedDefaults() guards with count()==0 check — fully idempotent, safe to call on every app launch"
  - "importMaterial() copies .dwmat to getMaterialsDir() before inserting to DB — prevents path invalidation"
  - "MaterialArchive::create() handles empty texturePath for metadata-only export of default species"
  - "clearMaterialAssignment() added (not in original plan) for completeness of assignment lifecycle"

patterns-established:
  - "Seed-on-first-run: count()==0 guard ensures idempotent seeding without duplicate entries"
  - "Copy-to-managed-dir: all imported archives live under getMaterialsDir() preventing stale external paths"
  - "Manager wraps repository: MaterialManager owns MaterialRepository internally, exposes domain-level operations"

requirements-completed: []

duration: 7min
completed: 2026-02-20
---

# Phase 01 Plan 03: Default Materials and MaterialManager Summary

**32 built-in wood species with Janka ratings, MaterialManager seeding/import/export pipeline, and getMaterialsDir() for app-managed .dwmat archive storage**

## Performance

- **Duration:** ~7 minutes
- **Started:** 2026-02-20T05:48:26Z
- **Completed:** 2026-02-20T05:55:43Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments

- Added `getMaterialsDir()` to app_paths (returns `getDataDir()/materials`), auto-created in `ensureDirectoriesExist()`
- Implemented `getDefaultMaterials()` with 32 species: 8 hardwoods (Red Oak, White Oak, Hard Maple, Cherry, Black Walnut, White Ash, Yellow Birch, Hickory), 7 softwoods, 7 domestic, 10 composites (MDF, HDF, plywood, aluminum, brass, HDPE, acrylic, rigid foam)
- Implemented `MaterialManager` with seed/import/export/CRUD/model-assignment following the LibraryManager pattern
- Fixed pre-existing miniz build (split into 4 source files in v3.0.2) enabling ZIP operations for .dwmat archives
- All 482 tests pass including 24 new MaterialManager tests

## Task Commits

1. **Task 1: Default materials list + getMaterialsDir()** - `d0c7531` (feat: add default materials list and getMaterialsDir())
2. **Task 2: MaterialManager implementation** - `801340a` (feat: implement MaterialManager for import/export/seeding)

## Files Created/Modified

- `src/core/materials/default_materials.h` - getDefaultMaterials() declaration
- `src/core/materials/default_materials.cpp` - 32 built-in species with Janka ratings and CNC defaults
- `src/core/materials/material_manager.h` - MaterialManager interface
- `src/core/materials/material_manager.cpp` - Seed/import/export/assign implementation
- `src/core/materials/material_archive.cpp` - Updated create/load to handle metadata-only archives
- `src/core/paths/app_paths.h` - Added getMaterialsDir() declaration
- `src/core/paths/app_paths.cpp` - getMaterialsDir() impl + ensureDirectoriesExist() enrollment
- `src/CMakeLists.txt` - Added default_materials.cpp, material_manager.cpp
- `tests/CMakeLists.txt` - Added test_material_manager.cpp, material_manager.cpp to deps
- `cmake/Dependencies.cmake` - Fixed miniz_static to include all 4 source files
- `tests/test_material_manager.cpp` - 24 tests covering seed/import/export/assign/remove

## Decisions Made

- Composites (MDF, metals, plastics, foam) use jankaHardness=0 since Janka rating is N/A for non-wood materials
- `clearMaterialAssignment()` added beyond plan spec for completeness of model material lifecycle
- MaterialArchive::create() changed to treat empty texturePath as "skip texture" rather than fail — enables metadata-only export for default species without .dwmat files

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed miniz_static to include all 4 source files**
- **Found during:** Task 1 build verification
- **Issue:** miniz 3.0.2 split library into miniz.c, miniz_tdef.c, miniz_tinfl.c, miniz_zip.c; cmake only included miniz.c causing undefined references to mz_zip_* functions
- **Fix:** Updated cmake/Dependencies.cmake to add all 4 source files to miniz_static target
- **Files modified:** cmake/Dependencies.cmake
- **Verification:** Build succeeds, all ZIP operations link correctly
- **Committed in:** d0c7531 (Task 1)

**2. [Rule 2 - Missing Critical] Updated MaterialArchive::create() for empty texture path**
- **Found during:** Task 2 (exportMaterial for default species)
- **Issue:** MaterialArchive::create() required a texture file path; default species have no .dwmat so export needed metadata-only archive support
- **Fix:** Wrapped texture add in `if (!texturePath.empty())` block; load() updated to handle archives missing texture.png
- **Files modified:** src/core/materials/material_archive.cpp
- **Verification:** Export_DefaultMaterial tests pass, isValidArchive() still works for metadata-only archives
- **Committed in:** 801340a (Task 2)

**3. [Rule 1 - Bug] Fixed pre-existing clang-format violation in texture.cpp**
- **Found during:** Full test run after Task 2
- **Issue:** src/render/texture.cpp had formatting violations failing LintCompliance test
- **Fix:** Ran clang-format -i on texture.cpp
- **Files modified:** src/render/texture.cpp
- **Committed in:** 801340a (Task 2)

---

**Total deviations:** 3 auto-fixed (1 blocking, 1 missing critical, 1 bug)
**Impact on plan:** All fixes necessary for correctness and build. No scope creep.

## Issues Encountered

None significant — miniz split was the main surprise, fixed cleanly with CMake update.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- MaterialManager ready for UI integration (01-04: MaterialsPanel) and renderer integration (01-05: texture mapping)
- seedDefaults() should be called at app startup (in Application.cpp or a startup manager)
- getMaterialsDir() path is established and auto-created; import pipeline copies to this location

## Self-Check: PASSED

All created files exist on disk. Both task commits (d0c7531, 801340a) present in git history.

---
*Phase: 01-add-materials-mapping-to-object-view-with-coordinated-materials-database*
*Completed: 2026-02-20*
