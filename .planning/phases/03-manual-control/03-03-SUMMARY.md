# Plan 03-03 Summary: CncWcsPanel (Work Zero / WCS)

**Status:** Complete
**Completed:** 2026-02-24

## What Was Built

CncWcsPanel — work coordinate system panel with zero-set buttons, G54-G59 coordinate system selector, and stored offset display table.

### Key Features
- **Zero Buttons:** Zero X, Zero Y, Zero Z, and Zero All (red-styled) buttons sending G10 L20 P{n} commands
- **Confirmation Modal:** Safety popup showing current position before zeroing, with red Confirm button
- **G54-G59 Selector:** 6 buttons with active highlight (blue), switches coordinate system by sending G54/G55/etc
- **Offset Table:** 4-column table (WCS, X, Y, Z) showing stored offsets from $# query with active row highlight
- **$# Response Parser:** Parses [G54:x,y,z] format lines, commits after all 6 WCS entries received
- **Auto-Load:** Requests offsets on connection and after any zero-set operation

### Key Files

| File | Purpose |
|------|---------|
| `src/ui/panels/cnc_wcs_panel.h` | Panel class with parsing state, offset storage |
| `src/ui/panels/cnc_wcs_panel.cpp` | Full implementation (~310 lines) |
| `src/core/cnc/cnc_types.h` | Added WcsOffsets struct with getByIndex() |

### Design Decisions
- WcsOffsets struct lives in cnc_types.h for reuse across panels
- $# parsing is stateful (m_parsingOffsets flag) to avoid false matches on normal output
- Confirmation popup prevents accidental zero-set (especially Zero All)
- G10 L20 used (set current position as zero) rather than G10 L2 (set absolute offset)

### Self-Check: PASSED
- [x] Zero X/Y/Z/All buttons with G10 L20 commands
- [x] Confirmation modal for safety
- [x] G54-G59 selector with active highlight
- [x] Offset table from $# response
- [x] Auto-refresh on connect and after zeroing

## Commits
- `feat: add WCS panel with zero-set, G54-G59 selector, offset display (CUI-08/09)`
