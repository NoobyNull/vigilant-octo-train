# Plan 18-01 Summary: Wizard Framework, Step Navigation, Machine Readiness

**Status**: Complete
**Requirements**: DC-20, DC-21

## What Was Built

- `DirectCarvePanel` class (`direct_carve_panel.h/cpp`) implementing a 9-step wizard:
  MachineCheck -> ModelFit -> ToolSelect -> MaterialSetup -> Preview -> OutlineTest -> ZeroConfirm -> Commit -> Running
- Step indicator with custom ImDrawList circle+line rendering, theme-colored per status
- Navigation: Back/Next/Cancel with `canAdvance()` gate per step
- Machine readiness check: CNC connected, homed (idle), profile configured, safe Z verified
- Integration into UIManager: panel member, accessor, visibility toggle, View menu, panel registry
- Wiring into application_wiring.cpp: CncController, ToolDatabase, CNC callback fan-out

## Key Decisions

- Panel hidden by default (`m_open = false`), shown via View > Direct Carve menu
- Safe Z verified by either "Test Safe Z" (sends G0 command) or manual confirmation
- All step colors use Theme::Colors constants (no hardcoded values)
- Step indicator uses font-relative sizing for DPI awareness

## Files Changed

| File | Lines | Action |
|------|-------|--------|
| `src/ui/panels/direct_carve_panel.h` | ~155 | Created |
| `src/ui/panels/direct_carve_panel.cpp` | ~490 | Created |
| `src/managers/ui_manager.h` | +5 | Modified (member, accessor, visibility) |
| `src/managers/ui_manager.cpp` | +15 | Modified (init, render, menu, registry) |
| `src/app/application_wiring.cpp` | +14 | Modified (dependencies, callbacks) |
| `src/CMakeLists.txt` | +1 | Modified (source list) |

## Test Results

804/804 tests passing. No new unit tests (UI panel — verified manually).
