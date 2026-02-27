# Requirements: Digital Workshop -- Sender Feature Parity

**Defined:** 2026-02-26
**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

## v1 Requirements

Requirements for the Sender Feature Parity milestone. Each maps to roadmap phases.

### Safety

- [ ] **SAF-10**: Home button requires configurable long-press (default 1s) to prevent accidental homing
- [ ] **SAF-11**: Job start requires configurable long-press (default 1s) to prevent accidental job launch
- [ ] **SAF-12**: Abort button supports configurable long-press requirement as alternative to confirmation dialog
- [ ] **SAF-13**: Dead-man watchdog stops continuous jog if no keepalive received within configurable timeout (default 1000ms)
- [ ] **SAF-14**: Door interlock blocks rapid moves and spindle commands when GRBL Pn:D (door) pin is active, configurable on/off
- [ ] **SAF-15**: Soft limit pre-check compares G-code bounds against machine travel ($130-$132) before streaming, configurable on/off
- [ ] **SAF-16**: Stop command sends feed hold before soft reset to reduce tool marks (pause-before-reset), configurable on/off
- [ ] **SAF-17**: Safety settings UI exposes per-feature enable/disable toggles with defaults in Config

### Core Sender

- [ ] **SND-01**: Spindle override slider (0-200%) sends GRBL real-time spindle override commands
- [ ] **SND-02**: Rapid override control (25%, 50%, 100%) sends GRBL real-time rapid override commands
- [ ] **SND-03**: Coolant toggle buttons (Flood M8, Mist M7, Off M9) with visual state indicators in status panel
- [ ] **SND-04**: Alarm state displays GRBL alarm code with human-readable description and inline $X unlock button
- [ ] **SND-05**: Status polling interval configurable (50, 100, 150, 200ms) in settings, applied at runtime
- [ ] **SND-06**: Jog step sizes include 0.01mm for precision zeroing (added to existing 0.1, 1, 10, 100mm)
- [ ] **SND-07**: Per-step-group feedrate: separate configurable feedrates for small (0.01-0.1mm), medium (1mm), and large (10-100mm) step groups
- [ ] **SND-08**: WCS quick-switch selector (G54-G59) visible in CNC status panel header without opening WCS panel

### Niceties

- [ ] **NIC-01**: Double-click DRO axis value to zero that axis (sends G10 L20 for clicked axis)
- [ ] **NIC-02**: Move-to dialog for explicit XYZ target entry with Go button (sends G0/G1 to target)
- [ ] **NIC-03**: Diagonal XY jog buttons (+X+Y, +X-Y, -X+Y, -X-Y) in jog panel
- [ ] **NIC-04**: Console messages tagged with source type ([JOB], [MDI], [MACRO], [SYS]) via color and prefix
- [ ] **NIC-05**: Recent G-code files list (last 10) in GCode panel for quick re-load
- [ ] **NIC-06**: Keyboard shortcuts for feed override (±10%) and spindle override (±10%) bindable in config
- [ ] **NIC-07**: Macro list supports drag-and-drop reordering with sort_order persisted to database
- [ ] **NIC-08**: Job completion notification via toast and optional status bar flash when streaming finishes

### Extended

- [ ] **EXT-01**: GRBL alarm code reference table (codes 1-10+) accessible from alarm display as tooltip or expandable section
- [ ] **EXT-02**: GRBL error code reference (errors 1-37+) shown in console when error responses received
- [ ] **EXT-03**: Firmware info query ($I) with version display in settings panel
- [ ] **EXT-04**: Export GRBL settings to plain text file (in addition to existing JSON backup)
- [ ] **EXT-05**: Log-to-file toggle in settings exposing existing setLogFile capability
- [ ] **EXT-06**: G-code out-of-bounds pre-check compares parsed bounds vs machine travel limits before streaming
- [ ] **EXT-07**: Probe active indicator (LED) in status panel parsing Pn:P from GRBL status
- [ ] **EXT-08**: Cycle-steps keyboard shortcut cycles through jog step size groups (small→medium→large→small)
- [ ] **EXT-09**: Per-tool color coding in toolpath visualization segments based on T-code parsing
- [ ] **EXT-10**: M6 tool change detection pauses streaming, notifies operator, waits for acknowledgment before continuing
- [ ] **EXT-11**: Z-probe workflow dialog with guided steps: approach speed, plate thickness, retract distance, probe execution
- [ ] **EXT-12**: G-code search/goto: find text in loaded G-code file and scroll to matching line
- [ ] **EXT-13**: Nested macros via M98 Pxxxx: sender-side expansion with recursion guard (max depth 16) and error reporting
- [ ] **EXT-14**: Gamepad input via SDL_GameController: axis mapping for jog, button mapping for start/pause/stop/home
- [ ] **EXT-15**: Tool length setter (TLS) workflow: measure tool offset, store per-tool, apply G43 compensation
- [ ] **EXT-16**: 3D probing workflows: edge finding, corner probing, center finding with approach/retract sequences

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Connectivity

- **CON-01**: Ethernet/Telnet connection for network-attached CNC controllers
- **CON-02**: WebSocket protocol for FluidNC WiFi connectivity
- **CON-03**: Windows serial port implementation (Win32 CreateFile/ReadFile/WriteFile)

### Remote Control

- **RMT-01**: Headless server mode for browser-based remote access
- **RMT-02**: Multi-client support for simultaneous monitoring

### Plugin System

- **PLG-01**: Event-based plugin architecture with command transformation pipeline
- **PLG-02**: Plugin marketplace with GitHub-based install/update

## Out of Scope

| Feature | Reason |
|---------|--------|
| Toolpath generation / CAM | Future milestone, requires fundamentally different architecture |
| Remote/headless server mode | Desktop CNC machines are physically attended |
| Plugin/extensibility system | Premature for current userbase size |
| Pendant hardware support | Requires specific hardware testing unavailable |
| Firmware flashing (DFU) | Too risky without extensive hardware testing matrix |
| Full G-code editor (Monaco-style) | MDI console + search/goto covers immediate needs |
| Height map auto-leveling | Complex feature, deferred to probing milestone |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| SAF-10 | Phase 9: Safety | Pending |
| SAF-11 | Phase 9: Safety | Pending |
| SAF-12 | Phase 9: Safety | Pending |
| SAF-13 | Phase 9: Safety | Pending |
| SAF-14 | Phase 9: Safety | Pending |
| SAF-15 | Phase 9: Safety | Pending |
| SAF-16 | Phase 9: Safety | Pending |
| SAF-17 | Phase 9: Safety | Pending |
| SND-01 | Phase 10: Core Sender | Pending |
| SND-02 | Phase 10: Core Sender | Pending |
| SND-03 | Phase 10: Core Sender | Pending |
| SND-04 | Phase 10: Core Sender | Pending |
| SND-05 | Phase 10: Core Sender | Pending |
| SND-06 | Phase 10: Core Sender | Pending |
| SND-07 | Phase 10: Core Sender | Pending |
| SND-08 | Phase 10: Core Sender | Pending |
| NIC-01 | Phase 11: Niceties | Pending |
| NIC-02 | Phase 11: Niceties | Pending |
| NIC-03 | Phase 11: Niceties | Pending |
| NIC-04 | Phase 11: Niceties | Pending |
| NIC-05 | Phase 11: Niceties | Pending |
| NIC-06 | Phase 11: Niceties | Pending |
| NIC-07 | Phase 11: Niceties | Pending |
| NIC-08 | Phase 11: Niceties | Pending |
| EXT-01 | Phase 12: Extended | Pending |
| EXT-02 | Phase 12: Extended | Pending |
| EXT-03 | Phase 12: Extended | Pending |
| EXT-04 | Phase 12: Extended | Pending |
| EXT-05 | Phase 12: Extended | Pending |
| EXT-06 | Phase 12: Extended | Pending |
| EXT-07 | Phase 12: Extended | Pending |
| EXT-08 | Phase 12: Extended | Pending |
| EXT-09 | Phase 12: Extended | Pending |
| EXT-10 | Phase 12: Extended | Pending |
| EXT-11 | Phase 12: Extended | Pending |
| EXT-12 | Phase 12: Extended | Pending |
| EXT-13 | Phase 12: Extended | Pending |
| EXT-14 | Phase 12: Extended | Pending |
| EXT-15 | Phase 12: Extended | Pending |
| EXT-16 | Phase 12: Extended | Pending |

**Coverage:**
- v1 requirements: 42 total
- Mapped to phases: 42
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-26*
*Last updated: 2026-02-26 after initial definition*
