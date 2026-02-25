# Plan 03-04 Summary: Integration & Wiring

**Status:** Complete
**Completed:** 2026-02-24

## What Was Built

Full UIManager integration of all Phase 3 CNC panels: creation, rendering, View menu entries, workspace mode visibility, keyboard jog dispatch, and CncController callback wiring.

### Key Changes
- **UIManager Header:** Added forward declarations, unique_ptr members, accessors, and visibility flags for CncJogPanel, CncConsolePanel, CncWcsPanel
- **Panel Lifecycle:** Created in init(), destroyed in shutdown(), rendered in renderPanels() with X-button close sync
- **View Menu:** Added "Jog Control", "MDI Console", "Work Zero / WCS" menu items
- **Workspace Mode:** CNC mode (Ctrl+2) shows all 4 CNC panels + G-code; Model mode (Ctrl+1) hides them
- **Keyboard Jog:** handleKeyboardShortcuts() calls cncJogPanel->handleKeyboardJog() when panel visible and not typing
- **Callback Wiring:** Extended CncCallbacks lambdas in application_wiring.cpp to forward events to all new panels (connection, status, raw lines, alarms, errors)

### Key Files

| File | Purpose |
|------|---------|
| `src/managers/ui_manager.h` | Forward decls, members, accessors, visibility flags |
| `src/managers/ui_manager.cpp` | Panel creation, rendering, menu, workspace mode, keyboard jog |
| `src/app/application_wiring.cpp` | CncController callback wiring to new panels |
| `src/CMakeLists.txt` | Added cnc_console_panel.cpp, cnc_wcs_panel.cpp to build |

### Design Decisions
- Keyboard jog runs before Ctrl+ shortcut check (no modifier needed for arrow keys)
- Guard: keyboard jog only when panel visible AND WantTextInput is false
- All new panels follow same X-button close pattern as existing panels

### Self-Check: PASSED
- [x] All 3 new panels created and rendered by UIManager
- [x] View menu entries for all new panels
- [x] CNC workspace mode shows all CNC panels
- [x] Keyboard jog dispatched from handleKeyboardShortcuts()
- [x] CncController callbacks wired to all panels
- [x] CMakeLists.txt includes all new source files
- [x] Compiles cleanly (only pre-existing tool_database errors)

## Commits
- `feat: integrate CNC panels into UIManager with keyboard jog and callback wiring`
