# Feature Landscape: CNC Control Suite

**Domain:** Desktop CNC Controller (GRBL-focused, C++17/ImGui)
**Researched:** 2026-02-24
**Competitors analyzed:** UGS, CNCjs, bCNC, Candle, gSender, Mach3/4, LinuxCNC

## Existing Digital Workshop Capabilities

Before categorizing new features, here is what already exists in the codebase:

| Existing Feature | Status | Location |
|-----------------|--------|----------|
| GRBL serial connection | Working | `cnc_controller.h`, `serial_port.h` |
| Character-counting streaming | Working | `CncController::sendNextLines()` |
| Status polling (5 Hz) | Working | `MachineStatus` struct |
| Feed/rapid/spindle override | Working | Real-time GRBL commands |
| Feed hold / cycle start | Working | `feedHold()`, `cycleStart()` |
| Soft reset | Working | `softReset()` |
| Jog cancel | Working | `jogCancel()` |
| Alarm handling | Working | `onAlarm` callback |
| G-code parser (G0/G1/G2/G3, arcs) | Working | `gcode_parser.h` |
| G-code 3D visualizer | Working | `GCodePanel` with OpenGL |
| Toolpath simulation with playback | Working | `SimState`, trapezoidal planner |
| Machine profiles (Shapeoko, LongMill) | Working | `MachineProfile` |
| Serial console | Working | `ConsoleLine`, `renderConsole()` |
| Feed override slider | Working | `renderFeedOverride()` |
| Stream progress tracking | Working | `StreamProgress` |
| Tool database (Vectric .vtdb) | Working | `ToolDatabase` |
| Materials system | Working | `VtdbMaterial` |
| Feeds and speeds calculator | Working | Recent commit v1.7.1 |
| Z-clip layer slider | Working | `renderZClipSlider()` |
| Path filtering (rapid/cut/plunge/retract) | Working | View toggles |

---

## Table Stakes

Features every CNC controller GUI has. Missing any of these makes the product feel incomplete or untrustworthy. Users coming from UGS, CNCjs, Candle, or gSender expect all of these.

### Connection and Communication

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Serial port auto-detection | Every controller scans /dev/ttyUSB*, ttyACM* | Low | DONE | `listSerialPorts()` |
| Baud rate selection | Standard GRBL setup | Low | DONE | 6 rates supported |
| Connection status indicator | Users need to know if machine is connected | Low | DONE | `isConnected()` |
| Auto-reconnect on disconnect | USB disconnects happen frequently mid-job | Medium | MISSING | Should retry with backoff |
| Firmware version display | Shown on every GRBL connect handshake | Low | DONE | `onConnectionChanged` |

### Digital Readout (DRO)

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Machine position (MPos) display | Fundamental to CNC operation | Low | PARTIAL | Status struct has it, needs dedicated DRO widget |
| Work position (WPos) display | What operators actually look at | Low | PARTIAL | Same -- need prominent UI |
| Feed rate display (live) | Every controller shows this | Low | PARTIAL | In MachineStatus, needs UI |
| Spindle speed display (live) | Every controller shows this | Low | PARTIAL | Same |
| Machine state indicator (Idle/Run/Hold/Alarm) | Critical safety awareness | Low | PARTIAL | Enum exists, needs colored badge |
| Override percentages display | Feed/rapid/spindle override readback | Low | PARTIAL | In status, needs UI |

### Jogging

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| XYZ jog buttons (+ / -) | Primary manual positioning method | Medium | MISSING | Need jog command generation |
| Step size presets (0.01, 0.1, 1, 10 mm) | All controllers have these | Low | MISSING | |
| Continuous jog (hold-to-move) | Expected for rapid positioning | Medium | MISSING | GRBL $J= jog commands |
| Keyboard jog (arrow keys, PgUp/PgDn) | Power users expect this | Medium | MISSING | Key bindings exist in framework |
| Jog speed slider | Control how fast manual moves are | Low | MISSING | |

### Homing and Zero Setting

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Home all axes ($H) | First thing done on power-up | Low | MISSING | Single GRBL command |
| Home individual axis | Some setups need per-axis homing | Low | MISSING | |
| Zero X / Zero Y / Zero Z buttons | Set work coordinate origin | Low | MISSING | G10 L20 P1 commands |
| Zero All (XYZ) | Convenience button | Low | MISSING | |
| Go to Zero (rapid to WCS origin) | Verify zero is correct before cutting | Low | MISSING | G0 X0 Y0 then G0 Z0 |
| Go to machine zero (G53 G0 X0 Y0 Z0) | Park / tool change position | Low | MISSING | |

### Job Control

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Start / Pause / Resume / Stop | Basic streaming controls | Low | PARTIAL | Start/stop exist, need pause/resume UI |
| Progress bar with percentage | Visual job progress | Low | DONE | `renderProgressBar()` |
| Elapsed time display | How long has this been running | Low | PARTIAL | `elapsedSeconds` tracked |
| Estimated time remaining | When will this finish | Medium | PARTIAL | Total time from analyzer, need countdown |
| Line count progress (N / Total) | Which line are we on | Low | DONE | `StreamProgress` |

### G-code Management

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Load G-code file | Basic file open | Low | DONE | `loadFile()` |
| G-code line listing with scroll | See the program | Low | DONE | In panel |
| Current line highlight during streaming | Know where the machine is in the program | Low | DONE | `m_lastAckedLine` |
| G-code 3D visualization | Visual verification before cutting | Low | DONE | Full OpenGL visualizer |
| MDI / Console command entry | Send manual G-code commands | Medium | PARTIAL | Console exists, need input field |

### Safety

| Feature | Why Expected | Complexity | DW Status | Notes |
|---------|-------------|------------|-----------|-------|
| Software E-stop (soft reset 0x18) | Emergency machine stop | Low | DONE | `softReset()` |
| Feed hold (pause) | Immediate safe pause | Low | DONE | `feedHold()` |
| Alarm display with description | Understand why machine stopped | Low | DONE | `alarmDescription()` |
| Error display with description | Understand G-code errors | Low | DONE | `errorDescription()` |
| Unlock ($X) after alarm | Recovery from alarm state | Low | DONE | `unlock()` |
| Confirmation dialog before streaming | Prevent accidental job start | Low | MISSING | Simple modal |

---

## Differentiators

Features that set Digital Workshop apart. Not every controller has these. Each one is a reason someone would choose DW over UGS or CNCjs.

### Integrated Tool Database (UNIQUE to DW)

| Feature | Value Proposition | Complexity | DW Status | Notes |
|---------|------------------|------------|-----------|-------|
| Tool library with Vectric .vtdb format | Bidirectional tool sharing with Vectric ecosystem | N/A | DONE | Major differentiator |
| Material-aware feeds/speeds calculator | Auto-suggest based on tool + material | N/A | DONE | v1.7.1 |
| Tool change with database lookup | When M6 Tn is hit, show tool details from DB | Medium | MISSING | No other GRBL sender does this |
| Feed rate validation against tool DB | Warn if program feed exceeds tool's rated max | Medium | MISSING | Unique safety feature |
| Recommended vs actual feed comparison | Show "recommended: 1200, actual: 2000" overlay | Medium | MISSING | |

No other GRBL sender integrates a full tool database with material-aware feeds/speeds. UGS, CNCjs, Candle, and gSender all treat the machine as a dumb G-code executor with no knowledge of tooling. This is DW's biggest competitive advantage.

### Intelligent Simulation

| Feature | Value Proposition | Complexity | DW Status | Notes |
|---------|------------------|------------|-----------|-------|
| Trapezoidal motion simulation | Accurate time estimates accounting for accel/decel | N/A | DONE | Machine profile based |
| Real-time position overlay on 3D view | Show tool position on visualizer during streaming | Medium | MISSING | Map MPos to path |
| Toolpath comparison (expected vs actual) | Detect drift or missed steps | High | MISSING | Research needed |

### Probing Routines

| Feature | Value Proposition | Complexity | Notes |
|---------|------------------|------------|-------|
| Z-probe (tool length) | Required for tool changes, most common probe | Medium | G38.2 command + offset calc |
| XYZ corner probe | Find workpiece corner to set WCS | Medium | Standard on bCNC, gSender |
| Edge finder (X or Y) | Single-axis edge detection | Medium | Subset of corner probe |
| Center finder (bore/boss) | Find center of hole or cylinder | Medium | bCNC and LinuxCNC have this |
| Auto height map (surface probing) | PCB milling, warped stock compensation | High | bCNC, OpenCNCPilot, Candle have this |

Probing is a strong differentiator because most lightweight GRBL senders have minimal or no probing. bCNC has the best probing but terrible UX. gSender has basic probing. Building clean, guided probing routines in a native C++ app would be compelling.

### Safe Job Recovery

| Feature | Value Proposition | Complexity | Notes |
|---------|------------------|------------|-------|
| Start-from-line (skip to line N) | Resume after error without re-running whole file | Medium | gSender has this, UGS does not |
| State reconstruction on resume | Replay modal state (G90, G21, spindle, etc.) to line N | High | Critical safety -- must restore machine state |
| Job checkpoint bookmarks | Save progress periodically for crash recovery | High | No GRBL sender does this well |
| Pre-flight checks | Verify tool, position, WCS before starting | Medium | Unique safety layer |

### Macro System

| Feature | Value Proposition | Complexity | Notes |
|---------|------------------|------------|-------|
| Named macro buttons | Store and recall G-code sequences | Medium | CNCjs, gSender, UGS all have this |
| Program event hooks (start/end/pause/resume) | Auto-run macros at job boundaries | Medium | gSender has this as "Program Events" |
| Parametric macros with variables | Probe routines, tool change cycles | High | Mach4 Lua, bCNC Python |
| Macro editor with syntax highlighting | Edit macros in-app | Medium | |

### Workspace Coordinate System Management

| Feature | Value Proposition | Complexity | Notes |
|---------|------------------|------------|-------|
| G54-G59 workspace selector | Switch between work offsets | Medium | GRBL supports 6 WCS (G54-G59) |
| Named workspaces (e.g., "Left Vise", "Fixture Plate") | Human-readable WCS labels | Low | No GRBL sender does this |
| WCS table showing all offsets | View all 6 workspaces at once | Medium | LinuxCNC and Mach4 have this |

### Gamepad / Pendant Support

| Feature | Value Proposition | Complexity | Notes |
|---------|------------------|------------|-------|
| Gamepad jogging (Xbox/PS controller) | Jog from beside the machine | High | gSender has this, very popular |
| USB shuttle/pendant support | Industry standard for CNC | High | Platform-specific HID handling |

---

## Anti-Features

Features to explicitly NOT build. These add complexity without proportional value, or actively harm the product.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Built-in CAM / toolpath generation | Massive scope explosion, inferior to Fusion/VCarve | Stay a sender + controller. Import G-code from CAM. |
| Built-in G-code editor with full IDE features | Text editors exist. G-code should come from CAM. | Basic line viewer with syntax highlight is enough. MDI for quick edits. |
| Web-based / browser interface | CNCjs already does this. Native app is DW's strength. | Stay native C++/ImGui. Performance and responsiveness are the advantage. |
| Multi-machine simultaneous control | Enormous complexity, niche use case | One machine per instance. Advanced users run multiple instances. |
| Custom firmware / direct stepper control | LinuxCNC territory. Requires RT kernel, hardware coupling. | Support GRBL/grblHAL protocol only. Let firmware handle real-time. |
| 3D printing / FDM features | Different domain, different protocols (Marlin/Klipper) | Stay CNC-focused: mills, routers, lasers. |
| Cloud connectivity / remote monitoring over internet | Security nightmare, unnecessary for hobby/small shop | Local network only if remote at all. File-based sharing. |
| Built-in SVG/DXF to G-code converter | CAM responsibility, not controller responsibility | Import G-code only. DW already has model import for visualization. |
| Automatic feed optimization during cut | Modifying G-code in real-time is dangerous | Show warnings/recommendations, but send what the user loaded |

---

## Feature Dependencies

```
Serial Connection
  +-- Status Polling
  |     +-- DRO Display
  |     +-- Machine State Badge
  |     +-- Override Readback
  +-- Streaming
  |     +-- Job Control (start/pause/resume/stop)
  |     +-- Progress Bar
  |     +-- Line Highlight
  |     +-- Start-from-line
  +-- Jog Commands
  |     +-- Keyboard Jog
  |     +-- Gamepad Jog
  +-- Homing ($H)
  |     +-- WCS Management
  |     +-- Go-to-Zero
  +-- Probing (G38.2)
        +-- Z-Probe
        +-- Corner Probe
        +-- Edge/Center Finder
        +-- Height Map

Tool Database (existing)
  +-- M6 Tool Change Handler
  |     +-- Tool info display on change
  |     +-- Z-probe after change
  +-- Feed Rate Validation
  +-- Recommended vs Actual overlay

Macro System
  +-- Program Event Hooks
  +-- Probe Routines (built as macros internally)
  +-- Tool Change Cycle (macro-driven)
```

---

## MVP Recommendation

### Phase 1: Core Controls (Table Stakes)

The existing codebase has the streaming engine and protocol layer but lacks the UI controls that make it a usable CNC controller. Build these first:

1. **DRO widget** -- Machine/work position, feed rate, spindle speed, state badge
2. **Jog controls** -- Buttons, step presets, continuous jog, keyboard binding
3. **Homing and zero-setting** -- $H, Zero X/Y/Z, Go-to-Zero
4. **MDI input field** -- Type and send G-code commands from console
5. **Job confirmation dialog** -- "Start streaming 4,521 lines to machine?"
6. **Auto-reconnect** -- Handle USB disconnects gracefully

**Rationale:** Without these, the controller is unusable despite having the protocol layer. Every competitor has them. These are all Low-Medium complexity.

### Phase 2: Tool-Aware Controller (Primary Differentiator)

Leverage the existing tool database and feeds/speeds calculator:

1. **M6 tool change handler** -- Pause on M6, display tool info from database
2. **Feed rate validation** -- Pre-stream check: warn if feed exceeds tool limits
3. **Recommended feed overlay** -- Show DB-recommended vs programmed during streaming
4. **Z-probe tool length** -- Basic G38.2 probe routine for tool changes

**Rationale:** This is what no other GRBL sender has. The tool DB and feeds calculator are already built -- connecting them to the streaming engine is DW's unique value.

### Phase 3: Probing and WCS

1. **XYZ corner probe wizard** -- Guided workflow with visualization
2. **Edge and center finder** -- Single-axis and bore/boss probing
3. **G54-G59 workspace management** -- Selector, named workspaces, offset table

**Rationale:** Probing is a gap in most lightweight GRBL senders. Building it with good UX (guided wizards, 3D preview of where probe will touch) differentiates against bCNC's powerful-but-ugly probing.

### Phase 4: Macros and Automation

1. **Named macro buttons** -- Store G-code sequences, bind to keys
2. **Program event hooks** -- Auto-run on start/end/pause
3. **Start-from-line with state reconstruction** -- Safe job resume

### Defer

- **Height map auto-leveling:** High complexity, niche use case (PCB milling). Build after core is solid.
- **Gamepad jogging:** Popular but platform-specific HID work. Build after keyboard jog is proven.
- **Job checkpoint/crash recovery:** Hard to do safely. Requires deep state tracking. Research phase-specific.

---

## Sources

- [UGS Website](https://winder.github.io/ugs_website/) -- Feature overview, plugin architecture
- [UGS GitHub](https://github.com/winder/Universal-G-Code-Sender) -- Source, releases
- [CNCjs GitHub](https://github.com/cncjs/cncjs) -- Feature set, firmware support
- [CNCjs User Guide](https://cnc.js.org/docs/user-guide/) -- Macro widget, workspace management
- [bCNC Wiki - Tool Change](https://github.com/vlachoudis/bCNC/wiki/Tool-Change) -- M6 handling, probe-based tool change
- [bCNC Wiki - AutoLevel](https://github.com/vlachoudis/bCNC/wiki/AutoLevel) -- Height map probing
- [Candle GitHub](https://github.com/Denvi/Candle) -- Qt-based GRBL controller, height mapping
- [gSender by Sienci Labs](https://sienci.com/gsender/) -- Gamepad, macros, program events, calibration
- [gSender Additional Features](https://resources.sienci.com/view/gs-additional-features/) -- Start-from-line, shortcuts, rotary
- [Mach4 Features](https://www.machsupport.com/software/mach4/) -- Lua scripting, ladder logic, tool life
- [Mach3 vs Mach4 Comparison](https://mellowpine.com/mach3-vs-mach4/) -- Feature differences
- [LinuxCNC Homing Config](https://linuxcnc.org/docs/html/config/ini-homing.html) -- Homing modes
- [LinuxCNC GMOCCAPY GUI](https://linuxcnc.org/docs/html/gui/gmoccapy.html) -- Full-featured GUI reference
- [CNC Probing - Centroid](https://www.centroidcnc.com/cnc_probing.html) -- Edge, center, corner finding
- [CNC Restart Safety Protocol](https://cnccode.com/2026/02/11/cnc-restart-safety-protocol-2026-how-to-restart-any-program-without-crashing-after-feed-hold-alarm-or-power-loss/) -- Job recovery dangers
- [CNC Work Coordinate Systems](https://www.cnccookbook.com/g54-g92-g52-work-offsets-cnc-g-code/) -- G54-G59 offset management
