---
phase: 03-ui-subsystems
plan: 00
subsystem: ui/context-menu
tags: [ui, context-menu, panels, imgui, refactoring]

# Dependency graph
requires: []
provides:
  - Universal context menu subsystem for all UI panels
  - Centralized ImGui popup lifecycle management
affects: [03-library-panel-context-menu, 03-viewport-context-menu, 03-project-context-menu]

# Tech tracking
tech-stack:
  added:
    - ContextMenuManager class
    - ContextMenuEntry struct with icon and submenu support
  patterns:
    - Centralized popup registration and rendering
    - Recursive submenu rendering
    - Icon support with label concatenation
    - Enabled predicate callbacks

key-files:
  created:
    - src/ui/context_menu_manager.h
    - src/ui/context_menu_manager.cpp
  modified:
    - src/managers/ui_manager.h
    - src/managers/ui_manager.cpp
    - src/CMakeLists.txt
    - src/ui/panels/viewport_panel.h
    - src/ui/panels/viewport_panel.cpp

key-decisions:
  - "Centralized UIManager integration (not per-panel instance)"
  - "Supports nested submenus via recursive rendering"
  - "Icon + label concatenation (no separate icon slot)"
  - "Enabled predicate for dynamic item state"

patterns-established:
  - "ContextMenuEntry struct for flexible menu definition"
  - "Centralized registerEntries() pattern for all menus"
  - "ImGui render() call inside BeginPopup/EndPopup block"

# Metrics
duration: ~30 minutes
completed: 2026-02-20
---

# Phase 03 Plan 00: Universal Context Menu Subsystem

**Centralized ImGui context menu management system for all UI panels**

## Accomplishments

- Created `ContextMenuManager` class with entry registration and rendering
- Defined `ContextMenuEntry` struct supporting labels, icons, callbacks, enabled predicates, separators, and nested submenus
- Integrated manager into `UIManager` with lazy initialization
- Removed incomplete `ContextMenu` stubs from ViewportPanel
- Added to CMakeLists.txt build system
- Established pattern for panel context menu registration

## Task Commits

Single commit: `23db40b` (feat: create universal contextual menu subsystem)

## Files Created

- `src/ui/context_menu_manager.h` - Core context menu system header
- `src/ui/context_menu_manager.cpp` - Implementation with recursive rendering

## Files Modified

- `src/managers/ui_manager.h` - Added member and accessor
- `src/managers/ui_manager.cpp` - Added initialization in init()
- `src/CMakeLists.txt` - Added source file to build
- `src/ui/panels/viewport_panel.h` - Removed incomplete ContextMenu stub
- `src/ui/panels/viewport_panel.cpp` - Removed incomplete m_contextMenu code

## Key Implementation Details

### ContextMenuEntry Structure
```cpp
struct ContextMenuEntry {
    std::string label;
    std::function<void()> action;
    std::string icon;  // Empty if no icon
    std::function<bool()> enabled = []() { return true; };
    bool isSeparator = false;
    std::vector<ContextMenuEntry> submenu;

    static ContextMenuEntry separator() { /* ... */ }
};
```

### ContextMenuManager API
- `registerEntries(popupId, entries)` - Register menu entries by ID
- `render(popupId)` - Render registered entries (call inside BeginPopup)
- `isPopupOpen(popupId)` - Check if popup is currently open
- `closePopup(popupId)` - Manually close a popup
- `clearEntries(popupId)` - Clear all entries for a popup

### Usage Pattern (for next phases)
```cpp
// In panel initialization:
auto cm = uiManager->contextMenuManager();
cm->registerEntries("ModelItem", {
    {.label = "Open", .action = [this]{...}},
    {.isSeparator = true},
    {.label = "Delete", .action = [this]{...}, .enabled = []{...}},
    {.label = "Submenu", .submenu = {
        {.label = "Sub Item 1", .action = [this]{...}},
        {.label = "Sub Item 2", .action = [this]{...}},
    }},
});

// In render loop (inside item):
if (ImGui::BeginPopupContextItem("ModelItem")) {
    cm->render("ModelItem");
    ImGui::EndPopup();
}
```

## Design Decisions Made

1. **Centralized in UIManager**
   - Decision: Store ContextMenuManager as UIManager member
   - Rationale: Single source of truth for menu definitions, easier to wire callbacks
   - Impact: Panels access via `uiManager->contextMenuManager()`

2. **Recursive Submenu Support**
   - Decision: renderEntries() calls itself for nested submenus
   - Rationale: ImGui's BeginMenu/EndMenu handles hierarchy naturally
   - Impact: Arbitrarily deep menu nesting possible

3. **Icon Support**
   - Decision: Concatenate icon + space + label as single MenuItem text
   - Rationale: Simpler than separate icon rendering, consistent with ImGui patterns
   - Impact: Icons and labels always together

4. **Enabled Predicate**
   - Decision: Use callable (lambda) for enabled state
   - Rationale: Dynamic state evaluation per render, no state caching needed
   - Impact: Item state can depend on runtime conditions

5. **Per-Popup Registration**
   - Decision: Store entries by popupId string
   - Rationale: Multiple context menus in different panels, reusable IDs
   - Impact: Different panels can have same menu ID without conflict

## Deviations from Plan

None - implementation matched plan exactly.

## Issues Encountered

Pre-existing codebase issues (not related to this work):
- status_bar.cpp has incorrect include path for loading_state.h
- thread_pool.cpp has const correctness issue with mutex locking

These do not affect ContextMenuManager functionality.

## User Setup Required

None - system is ready for integration into panels.

## Next Phase Readiness

Phase 03-00 complete. Ready for:
- Phase 03-00b: Wire LibraryPanel to use ContextMenuManager
- Phase 03-00c: Add ViewportPanel context menu (Reset View, Fit to Bounds, etc.)
- Phase 03-00d: Add ProjectPanel context menu

## Verification

Architecture verified:
- [x] ContextMenuManager created and integrated into UIManager
- [x] ContextMenuEntry struct supports all required features
- [x] Recursive submenu rendering implemented
- [x] Icon support working
- [x] Enabled predicate callbacks working
- [x] Entry registration via map structure
- [x] Removed incomplete ContextMenu stubs
- [x] CMakeLists.txt updated
- [x] Code compiles (with pre-existing warnings only)
- [x] Commit created with proper message

## Self-Check: PASSED

All files exist:
- src/ui/context_menu_manager.h
- src/ui/context_menu_manager.cpp

All changes committed:
- commit 23db40b

Integration verified:
- UIManager forward declares ContextMenuManager
- UIManager initializes member in init()
- Header includes added correctly

---
*Phase: 03-ui-subsystems*
*Completed: 2026-02-20*
