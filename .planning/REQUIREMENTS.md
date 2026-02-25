# Requirements: Digital Workshop -- CNC Controller Suite

**Defined:** 2026-02-24
**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

## v1 Requirements

Requirements for the CNC Controller Suite milestone. Each maps to roadmap phases.

### Foundation

- [x] **FND-01**: Real-time commands (feed hold, cycle start, overrides) are dispatched thread-safely via the IO thread, not directly from UI thread
- [x] **FND-02**: Streaming engine issues soft reset on unrecoverable error to prevent GRBL from executing remaining buffer
- [x] **FND-03**: Serial port detects USB disconnect (POLLHUP/POLLERR) and reports connection loss to UI
- [x] **FND-04**: Serial port suppresses DTR on open to prevent Arduino auto-reset and position loss

### Control UI

- [ ] **CUI-01**: DRO displays work and machine XYZ position, updated at polling rate (5Hz)
- [ ] **CUI-02**: DRO displays machine state (Idle, Run, Hold, Alarm, Home, etc.) with visual indicator
- [ ] **CUI-03**: DRO displays real-time feed rate and spindle RPM
- [ ] **CUI-04**: Jog controls with XYZ +/- buttons, configurable step sizes (0.1, 1, 10, 100mm)
- [ ] **CUI-05**: Keyboard jog shortcuts (arrow keys, page up/down for Z) with focus-aware input routing
- [ ] **CUI-06**: Continuous jog support using GRBL $J= jog commands with cancel on release
- [ ] **CUI-07**: Home cycle button that sends $H and tracks homing state
- [ ] **CUI-08**: Set work zero buttons (X, Y, Z, All) using G10 L20
- [ ] **CUI-09**: Work coordinate system selector (G54-G59) with display of stored offsets
- [ ] **CUI-10**: MDI console with single-line G-code input, command history (up/down arrow), and response display

### Tool-Aware Controller

- [x] **TAC-01**: User can select a tool geometry from the tool database to calculate cutting parameters
- [x] **TAC-02**: Selected tool geometry + selected wood species auto-calculates optimal feeds/speeds via existing calculator (no physical tool change)
- [x] **TAC-03**: Calculated cutting parameters (RPM, feed rate, plunge rate, stepdown, stepover) displayed in a reference panel for the operator to use
- [x] **TAC-04**: During streaming, running feed rate is compared to calculator recommendation with visual warning if >20% deviation
- [x] **TAC-05**: Job elapsed time displayed and updated during streaming
- [x] **TAC-06**: Estimated remaining time calculated from line progress and current feed rate
- [x] **TAC-07**: Current line number and total line count displayed during streaming
- [x] **TAC-08**: Job progress percentage displayed (current line / total lines)

### Safety & Recovery

- [ ] **SAF-01**: Pause button sends feed hold (!) with visual state change
- [ ] **SAF-02**: Resume button sends cycle start (~) only when in Hold state
- [ ] **SAF-03**: E-stop button sends soft reset (0x18) with confirmation dialog for running jobs
- [ ] **SAF-04**: Safe job resume: user can specify line number to resume from
- [ ] **SAF-05**: Resume builds modal state preamble by scanning G-code from line 0 to resume point (G90/G91, coordinate system, spindle, coolant, feed rate, units)
- [ ] **SAF-06**: Sensor status display shows endstop, probe, and door states from GRBL Pn: field
- [ ] **SAF-07**: Pre-flight check before streaming: verify connection active, no alarm state, tool selected (optional), material selected (optional)

### Firmware & Configuration

- [ ] **FWC-01**: GRBL settings panel reads $$ and displays all settings with human-readable descriptions
- [ ] **FWC-02**: User can edit individual GRBL settings with validation (min/max/type constraints)
- [ ] **FWC-03**: Settings backup: export current $$ to JSON file
- [ ] **FWC-04**: Settings restore: import $$ from JSON backup file with confirmation
- [ ] **FWC-05**: Machine tuning UI for steps/mm, max feed rate, acceleration per axis
- [ ] **FWC-06**: Machine profile sync: read GRBL $$ settings into DW machine profile on connect
- [ ] **FWC-07**: Machine profile push: write DW machine profile values to GRBL $$ settings
- [ ] **FWC-08**: Macro storage in SQLite with name, G-code content, optional keyboard shortcut
- [ ] **FWC-09**: Macro execution sends G-code lines sequentially via sendCommand()
- [ ] **FWC-10**: Built-in macros for common operations: homing, probe Z, return to zero

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Tool Change

- **TCH-01**: M6 tool change interception pauses job and prompts operator
- **TCH-02**: Tool change applies new tool parameters from database automatically

### Probing

- **PRB-01**: Z-probe wizard with touch plate thickness input
- **PRB-02**: Edge/corner/center probe wizards with guided workflow
- **PRB-03**: Probe results stored and displayed in WCS panel

### Platform

- **PLT-01**: Windows serial port implementation (Win32 CreateFile/ReadFile/WriteFile)
- **PLT-02**: WebSocket transport for FluidNC WiFi connectivity
- **PLT-03**: Height map auto-leveling with bilinear interpolation

## Out of Scope

| Feature | Reason |
|---------|--------|
| Toolpath generation / CAM | Future milestone, requires fundamentally different architecture |
| Rest machining | Depends on CAM foundation that doesn't exist yet |
| LinuxCNC support | Different protocol (NML/HAL), not GRBL-compatible |
| Real-time motion control | DW is a sender, not a motion controller; GRBL handles real-time |
| Gamepad/joystick jog | Nice-to-have but not core; keyboard jog covers the need |
| G-code editor | MDI console covers single commands; full editing is a separate tool |
| Remote/network access | Desktop-first, CNC machines are physically attended |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| FND-01 | Phase 1: Fix Foundation | Done |
| FND-02 | Phase 1: Fix Foundation | Done |
| FND-03 | Phase 1: Fix Foundation | Done |
| FND-04 | Phase 1: Fix Foundation | Done |
| CUI-01 | Phase 2: Status Display | Done |
| CUI-02 | Phase 2: Status Display | Done |
| CUI-03 | Phase 2: Status Display | Done |
| CUI-04 | Phase 3: Manual Control | Pending |
| CUI-05 | Phase 3: Manual Control | Pending |
| CUI-06 | Phase 3: Manual Control | Pending |
| CUI-07 | Phase 3: Manual Control | Pending |
| CUI-08 | Phase 3: Manual Control | Pending |
| CUI-09 | Phase 3: Manual Control | Pending |
| CUI-10 | Phase 3: Manual Control | Pending |
| TAC-01 | Phase 4: Tool Integration | Pending |
| TAC-02 | Phase 4: Tool Integration | Pending |
| TAC-03 | Phase 4: Tool Integration | Pending |
| TAC-04 | Phase 5: Job Streaming | Complete |
| TAC-05 | Phase 5: Job Streaming | Complete |
| TAC-06 | Phase 5: Job Streaming | Complete |
| TAC-07 | Phase 5: Job Streaming | Complete |
| TAC-08 | Phase 5: Job Streaming | Complete |
| SAF-01 | Phase 6: Job Safety | Pending |
| SAF-02 | Phase 6: Job Safety | Pending |
| SAF-03 | Phase 6: Job Safety | Pending |
| SAF-04 | Phase 6: Job Safety | Pending |
| SAF-05 | Phase 6: Job Safety | Pending |
| SAF-06 | Phase 6: Job Safety | Pending |
| SAF-07 | Phase 6: Job Safety | Pending |
| FWC-01 | Phase 7: Firmware Settings | Pending |
| FWC-02 | Phase 7: Firmware Settings | Pending |
| FWC-03 | Phase 7: Firmware Settings | Pending |
| FWC-04 | Phase 7: Firmware Settings | Pending |
| FWC-05 | Phase 7: Firmware Settings | Pending |
| FWC-06 | Phase 7: Firmware Settings | Pending |
| FWC-07 | Phase 7: Firmware Settings | Pending |
| FWC-08 | Phase 8: Macros | Pending |
| FWC-09 | Phase 8: Macros | Pending |
| FWC-10 | Phase 8: Macros | Pending |

**Coverage:**
- v1 requirements: 39 total
- Mapped to phases: 39
- Unmapped: 0

---
*Requirements defined: 2026-02-24*
*Last updated: 2026-02-24 after roadmap creation (8-phase structure)*
