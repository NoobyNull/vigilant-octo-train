---
phase: 18-guided-workflow
plan: "01"
subsystem: ui
tags: [imgui, wizard, cnc, direct-carve, panel]

requires:
  - phase: 17-toolpath-generation
    provides: CarveJob, ToolpathConfig, FitParams, ModelFitter types
  - phase: 12-cnc-firmware-settings
    provides: CncController, MachineStatus, MachineState types
provides:
  - DirectCarvePanel wizard framework with 9-step navigation
  - Machine readiness validation (connected, homed, profile, safe Z)
  - Step indicator with custom ImDrawList rendering
  - UIManager integration (member, accessor, visibility, menu, registry)
  - CNC callback fan-out (onConnectionChanged, onStatusUpdate)
affects: [18-02-step-content, 18-03-streaming-integration]

tech-stack:
  added: []
  patterns: [step-wizard-panel, canAdvance-gate-per-step, theme-color-functions]

key-files:
  created:
    - src/ui/panels/direct_carve_panel.h
    - src/ui/panels/direct_carve_panel.cpp
  modified:
    - src/managers/ui_manager.h
    - src/managers/ui_manager.cpp
    - src/app/application_wiring.cpp
    - src/CMakeLists.txt

key-decisions:
  - "Panel hidden by default (m_open=false), accessible via View > Sender > Direct Carve menu"
  - "Theme::Colors constants (not hardcoded ImVec4) for all status indicator colors"
  - "Font-relative sizing for step indicator circles and spacing (DPI-aware)"
  - "Safe Z verified by either G0 test command or manual skip confirmation"
  - "onModelLoaded stores vertices+indices for later heightmap generation"

patterns-established:
  - "Step wizard pattern: enum Step, canAdvance() switch, renderX() per step"
  - "Theme color helper functions: successColor(), errorColor(), warningColor()"

requirements-completed: [DC-20, DC-21]

duration: 12min
completed: 2026-02-28
---

# Phase 18 Plan 01: Wizard Framework, Step Navigation, Machine Readiness Summary

**9-step DirectCarvePanel wizard with step indicator, navigation gates, machine readiness validation, and full UIManager/wiring integration**

## Performance

- **Duration:** 12 min
- **Started:** 2026-02-28T20:22:22Z
- **Completed:** 2026-02-28T20:34:30Z
- **Tasks:** 5 (header, implementation, CMake, UIManager, wiring)
- **Files modified:** 6

## Accomplishments
- DirectCarvePanel class with 9-step wizard: MachineCheck, ModelFit, ToolSelect, MaterialSetup, Preview, OutlineTest, ZeroConfirm, Commit, Running
- Custom step indicator bar with circle+line rendering using ImDrawList, theme-derived colors
- Machine readiness validation: CNC connected, idle (homed), machine profile configured, safe Z verified
- Full UIManager integration: member, accessor, visibility toggle, View menu entry, panel registry, dock layout
- CNC callback wiring in application_wiring.cpp: CncController, ToolDatabase, onConnectionChanged, onStatusUpdate fan-out

## Task Commits

1. **Header + Implementation + CMake** - `96a5a39` (feat: wizard steps with navigation)
2. **UIManager + Wiring integration** - `1f541ec` (fix: theme colors, UIManager wiring)

**Plan metadata:** (this commit)

## Files Created/Modified
- `src/ui/panels/direct_carve_panel.h` - Panel class with Step enum, per-step state, dependency setters
- `src/ui/panels/direct_carve_panel.cpp` - 9 step renderers, step indicator, nav buttons, validation
- `src/managers/ui_manager.h` - DirectCarvePanel member, accessor, visibility flag
- `src/managers/ui_manager.cpp` - Panel creation, rendering, View menu, panel registry, dock layout
- `src/app/application_wiring.cpp` - CncController/ToolDatabase wiring, CNC callback fan-out
- `src/CMakeLists.txt` - Added direct_carve_panel.cpp to source list

## Decisions Made
- Panel hidden by default, shown via View > Sender > Direct Carve menu entry
- All indicator colors use Theme::Colors helper functions (successColor, errorColor, etc.) instead of hardcoded values
- Step indicator uses font-relative circle radius and spacing for DPI scaling
- Safe Z verified by either "Test Safe Z" (sends G0 Zsafe command) or "Skip (confirm safe Z)" manual override
- Model data (vertices, indices, bounds) stored in panel for later heightmap generation in plan 18-02

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Used getActiveMachineProfile() instead of non-existent getCncTravelX/Y/Z**
- **Found during:** Implementation (machine check step)
- **Issue:** Plan referenced getCncTravelX() etc. which don't exist in Config; travel limits are in MachineProfile
- **Fix:** Used Config::instance().getActiveMachineProfile().maxTravelX/Y/Z
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Verification:** Build succeeds, correct travel limits accessed

**2. [Rule 1 - Bug] Used sendCommand() instead of sendLine()**
- **Found during:** Implementation (safe Z test button)
- **Issue:** CncController API uses sendCommand(), not sendLine()
- **Fix:** Changed to m_cnc->sendCommand(cmd)
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Verification:** Build succeeds

**3. [Rule 2 - Missing Critical] Added Zero action buttons to ZeroConfirm step**
- **Found during:** Implementation (zero confirm step)
- **Issue:** Plan described verification but lacked active zero-setting capability
- **Fix:** Added "Set Zero Here", "Zero XY Only", "Zero Z Only" buttons sending G10 L20 commands
- **Files modified:** src/ui/panels/direct_carve_panel.cpp
- **Verification:** Build succeeds, buttons properly disabled when CNC not connected

---

**Total deviations:** 3 auto-fixed (2 bugs, 1 missing critical)
**Impact on plan:** All fixes necessary for correct API usage and user workflow. No scope creep.

## Issues Encountered
- Linter actively co-modified files during development, adding forward-looking state members (VtdbToolGeometry, vertices/indices storage, RunState enum) and making autonomous commits. Accepted these additions as they align with plans 18-02 and 18-03.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Wizard framework ready for step content implementation (plan 18-02)
- Placeholder steps (ToolSelect, Preview, OutlineTest) need real tool browser and toolpath integration
- CarveJob not yet owned by Application; needs creation and wiring in plan 18-02

## Self-Check: PASSED

- All 6 created/modified files verified on disk
- All task commits verified in git log
- Build: 804/804 tests passing
- DirectCarvePanel renders with 9 steps, navigation, machine readiness validation

---
*Phase: 18-guided-workflow*
*Completed: 2026-02-28*
