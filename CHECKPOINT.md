# Checkpoint: AutoOrient Performance Optimization (2026-02-20)

## Summary

Completed **Phase 01-01 performance optimization**: Precompute autoOrient at import time with lazy-write fallback on load. This resolves the ~2-3s per-model-load performance bottleneck caused by redundant autoOrient computation.

**Status**: ✅ Complete and verified

**Commit**: `9dfafd6` — feat(01-01): implement autoOrient precompute at import + lazy-write on load

## What Was Done

### 1. Database Schema Extension (v4 → v5)
- Added `orient_yaw REAL DEFAULT NULL` column to models table
- Added `orient_matrix TEXT DEFAULT NULL` column (JSON serialization of Mat4)
- Implemented `Schema::migrate(Database&, int fromVersion)` for safe schema upgrade
- Includes backwards-compatibility transaction handling

### 2. Database Layer (ModelRepository)
- Implemented `ModelRepository::updateOrient(i64 id, f32 yaw, const Mat4& matrix)`
- Extended `rowToModel(Statement&)` to deserialize orient data from DB
- Extended `insert(const ModelRecord&)` to store orient values

### 3. Mesh API
- Implemented `Mesh::applyStoredOrient(const Mat4& matrix)` — fast path that skips axis detection and normal counting
- Added `getOrientMatrix()` accessor for retrieving computed permutation matrix

### 4. Import Pipeline
- Wired autoOrient computation in ImportQueue Stage 5 (after mesh parsing)
- Stores computed yaw and matrix in ModelRecord
- Respects `Config::getAutoOrient()` for enablement

### 5. Load Pipeline (Lazy-Write Fallback)
- In application.cpp `onModelSelected()` worker thread:
  - If orient data exists in DB → apply via `applyStoredOrient()` (fast)
  - If orient data missing → compute `autoOrient()` on worker thread
  - Write computed values back to DB via `updateOrient()` for future loads

## Performance Impact

**Before**: Every model load triggers full `autoOrient()` computation (~2-3s for large meshes)
- Mesh axis detection with normal counting
- Permutation matrix computation

**After**:
- New imports: Orient computed once at import time, zero overhead on load
- Legacy imports: Orient computed on first load, stored for future loads
- Subsequent loads: Direct matrix application, no computation needed

## Files Changed

```
22 files changed, 348 insertions(+), 86 deletions(-)

Core changes:
  src/core/database/schema.h/cpp — v4→v5 migration
  src/core/database/model_repository.h/cpp — updateOrient(), rowToModel() extension
  src/core/mesh/mesh.h/cpp — applyStoredOrient() implementation
  src/core/import/import_queue.cpp — compute autoOrient at Stage 5
  src/app/application.cpp — lazy-write fallback on load

Supporting:
  src/core/config/config.h/cpp — Config::getAutoOrient() accessor
  settings/settings_app.h/cpp — UI control for autoOrient enablement
  src/managers/config_manager.h/cpp — config wiring
  src/ui/panels/{materials,properties,viewport}_panel.* — UI updates
  src/managers/ui_manager.cpp — manager updates
```

## Build & Test Status

✅ **Clean build** — No warnings or errors
- digital_workshop target: ✅
- dw_settings target: ✅
- dw_tests target: ✅

✅ **All tests passing** — 422 tests in full suite

## Next Steps

Phase 01 Materials Mapping continues with:
1. **01-05-PLAN**: MaterialsPanel UI with category tabs, thumbnail grid, edit form
2. **01-06-PLAN**: Application wiring, PropertiesPanel integration, human verification

This optimization provides the foundation for smooth performance when material-assigned models are loaded repeatedly.

## Technical Notes

### Backwards Compatibility
- Database migration v4→v5 handles existing records gracefully
- Legacy models get orient data computed on first load (transparent to user)
- No data loss or schema conflicts

### Thread Safety
- Orient computation happens on ImportQueue worker thread (safe, no GL context needed)
- DB writes protected by transaction handles
- Load computation happens on application worker thread, safe for main thread updates

### Design Rationale
Two-phase approach (import + lazy-write) chosen over single approach because:
- **Import-time only** would orphan existing model library
- **Lazy-write only** would cause expensive first-load stalls
- **Hybrid** achieves zero-cost new imports + transparent migration for legacy models
