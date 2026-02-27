# Phase 7 Plan 2: Cross-Panel Navigation Wiring Summary

Wire ProjectPanel navigation callbacks so clicking any asset opens the correct panel. Add "Add to Project" context menus in Library panel for models and G-code files.

## Commits

| Hash | Description |
|------|-------------|
| 5da7ade | feat(07-02): wire cross-panel navigation and "Add to Project" menus |

## Changes

### Panel Selection Methods (Task 1)

- **CostPanel::selectEstimate(i64)** -- finds estimate by ID, populates edit buffers
- **CutOptimizerPanel::loadCutPlan(CutPlanRecord)** -- deserializes sheet config, parts, result JSON; restores algorithm/kerf/margin settings
- **MaterialsPanel::selectMaterial(i64)** -- refreshes list, sets selection, fires callback

### Cross-Panel Navigation Wiring (Task 2)

In `application_wiring.cpp`, wired 4 ProjectPanel navigation callbacks:
- `onGCodeSelected` -- opens GCodePanel, loads file by path from GCodeRepository
- `onMaterialSelected` -- opens MaterialsPanel, selects material
- `onCostSelected` -- opens CostPanel, selects estimate
- `onCutPlanSelected` -- opens CutOptimizerPanel, loads plan from CutPlanRepository

### "Add to Project" Context Menus (Tasks 3 + 5)

- **Library panel models**: "Add to Project" entry in model context menu (enabled when project is open). Uses `ProjectManager::addModelToProject()` for each selected model.
- **Library panel G-code**: "Add to Project" entry in G-code context menu. Uses `GCodeRepository::addToProject()` callback wired in application_wiring.cpp.
- **ProjectManager injected** into LibraryPanel via `setProjectManager()` setter.

### Repository Access (Task 4)

Application already owned `m_cutPlanRepo` and `m_gcodeRepo` (from Phase 6/07-01). No changes needed.

## Files Modified

| File | Change |
|------|--------|
| src/ui/panels/cost_panel.h | Add `selectEstimate(i64)` |
| src/ui/panels/cost_panel.cpp | Implement selectEstimate() |
| src/ui/panels/cut_optimizer_panel.h | Add `loadCutPlan(CutPlanRecord)`, include cut_plan_repository.h |
| src/ui/panels/cut_optimizer_panel.cpp | Implement loadCutPlan() |
| src/ui/panels/materials_panel.h | Add `selectMaterial(i64)` |
| src/ui/panels/materials_panel.cpp | Implement selectMaterial() |
| src/ui/panels/library_panel.h | Add ProjectManager*, GCodeAddToProjectCallback |
| src/ui/panels/library_panel_items.cpp | Add "Add to Project" entries for models and G-code |
| src/app/application_wiring.cpp | Wire 4 navigation callbacks + inject ProjectManager + G-code add-to-project |

## Verification

- Build: `cmake --build build --target digital_workshop` -- clean
- Tests: 538/538 passing
- Pre-existing tool linker errors (matgen/texgen) unrelated to changes

## Deviations from Plan

None -- plan executed exactly as written.

## Metrics

- Duration: 4m 30s
- Tasks: 5/5 complete
- Files modified: 9

## Self-Check: PASSED
