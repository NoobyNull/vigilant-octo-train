---
phase: 11-niceties
plan: 02
subsystem: ui
tags: [imgui, cnc, console, macros, drag-and-drop]

requires:
  - phase: 08-macros
    provides: CncMacroPanel with macro list and MacroManager
provides:
  - Color-coded console message source tags ([JOB], [MDI], [MACRO], [SYS])
  - Public addLine() API for explicit source tagging from wiring code
  - Drag-and-drop macro reordering replacing up/down arrow buttons
affects: []

tech-stack:
  added: []
  patterns: [ImGui DragDropSource/Target for list reordering]

key-files:
  created: []
  modified:
    - src/ui/panels/cnc_console_panel.h
    - src/ui/panels/cnc_console_panel.cpp
    - src/ui/panels/cnc_macro_panel.h
    - src/ui/panels/cnc_macro_panel.cpp

key-decisions:
  - "MessageSource enum made public for external callers to tag console lines"
  - "Used ImGui Button as drag handle with = label for drag-and-drop source"

patterns-established:
  - "Console tagging: MessageSource enum + sourceTag/sourceColor helpers"
  - "List reordering: ImGui DragDrop with PAYLOAD type string for identity"

requirements-completed: [NIC-04, NIC-07]

duration: 8min
completed: 2026-02-27
---

# Plan 11-02: Console Source Tags and Macro Drag-and-Drop Summary

**Color-coded console source tags and drag-and-drop macro list reordering**

## Performance

- **Duration:** 8 min
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Console messages display source tags with distinct colors (MDI=cyan, JOB=blue, MACRO=purple, SYS=yellow)
- Public addLine() method enables explicit source tagging from application wiring
- Macro list items are draggable for intuitive reordering, persisted via MacroManager::reorder()

## Task Commits

1. **Task 1: Console source tags** - `8e954f4` (feat)
2. **Task 2: Drag-and-drop macro reordering** - `8e954f4` (feat, same commit)

## Files Created/Modified
- `src/ui/panels/cnc_console_panel.h` - Added MessageSource enum, addLine() public method, source field on ConsoleLine
- `src/ui/panels/cnc_console_panel.cpp` - Source tag rendering with color helpers, source assignment in callbacks
- `src/ui/panels/cnc_macro_panel.h` - Added drag source index state
- `src/ui/panels/cnc_macro_panel.cpp` - ImGui DragDrop reorder replacing up/down buttons

## Decisions Made
- Made MessageSource and ConsoleLine public so external code can use addLine()
- Used a small "=" button as drag handle since no grip icon was available

## Deviations from Plan
None - plan executed as written.

## Issues Encountered
- MessageSource initially placed in private section causing compilation error; moved to public.

## Next Phase Readiness
- Console and macro improvements complete
- Ready for verification

---
*Phase: 11-niceties*
*Completed: 2026-02-27*
