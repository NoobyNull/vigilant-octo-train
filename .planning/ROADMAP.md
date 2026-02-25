# Roadmap: CNC Controller Suite

**Milestone:** CNC Controller Suite
**Depth:** Comprehensive
**Phases:** 8
**Total v1 Requirements:** 39

## Phases

- [x] **Phase 1: Fix Foundation** - Resolve thread safety, error handling, and disconnect detection bugs in existing CncController (2026-02-24)
- [x] **Phase 2: Status Display** - Real-time DRO showing position, machine state, feed rate, and spindle RPM (2026-02-24)
- [x] **Phase 3: Manual Control** - Jog, homing, work zero, coordinate systems, and MDI console (2026-02-24)
- [ ] **Phase 4: Tool Integration** - Connect tool database and material system to feeds/speeds calculator
- [ ] **Phase 5: Job Streaming** - Progress tracking, time estimation, and feed rate deviation warnings
- [ ] **Phase 6: Job Safety** - Pause, resume, e-stop, safe job resume, sensor display, and pre-flight checks
- [ ] **Phase 7: Firmware Settings** - GRBL settings panel, backup/restore, machine tuning, and profile sync
- [ ] **Phase 8: Macros** - Macro storage, execution, and built-in common operations

## Phase Details

### Phase 1: Fix Foundation
**Goal**: Existing CncController code is reliable enough to build features on top of
**Depends on**: Nothing (first phase)
**Requirements**: FND-01, FND-02, FND-03, FND-04
**Success Criteria** (what must be TRUE):
  1. Real-time commands (feed hold, cycle start, overrides) execute without data races or crashes under concurrent use
  2. When GRBL enters an error state during streaming, the remaining buffer is flushed via soft reset and the UI shows the error
  3. Unplugging the USB cable during operation results in a clear "connection lost" message in the UI within 2 seconds
  4. Connecting to an Arduino-based GRBL board does not trigger a board reset or lose machine position
**Plans:** 3 plans
  - [x] 01-01-PLAN.md -- SerialPort hardening (DTR suppression + disconnect detection + partial write fix)
  - [x] 01-02-PLAN.md -- Thread-safe command dispatch + controller-level disconnect detection
  - [x] 01-03-PLAN.md -- Error-triggered soft reset during streaming

### Phase 2: Status Display
**Goal**: Operator can see machine position, state, and spindle/feed data at a glance while the machine runs
**Depends on**: Phase 1
**Requirements**: CUI-01, CUI-02, CUI-03
**Success Criteria** (what must be TRUE):
  1. Work position and machine position (X, Y, Z) update visually at polling rate (5Hz) during motion
  2. Machine state (Idle, Run, Hold, Alarm, Home) is displayed with a color-coded indicator that changes in real time
  3. Current feed rate (mm/min) and spindle RPM are displayed and update during streaming
**Plans:** 2 plans
  - [x] 02-01-PLAN.md -- CncStatusPanel with DRO, state indicator, feed/spindle display
  - [x] 02-02-PLAN.md -- UIManager integration, CNC callback wiring, workspace mode switching

### Phase 3: Manual Control
**Goal**: Operator can manually move the machine, set work zero, switch coordinate systems, and send ad-hoc G-code commands
**Depends on**: Phase 2
**Requirements**: CUI-04, CUI-05, CUI-06, CUI-07, CUI-08, CUI-09, CUI-10
**Success Criteria** (what must be TRUE):
  1. User can jog any axis in both directions using on-screen buttons or keyboard shortcuts, with selectable step sizes (0.1, 1, 10, 100mm) and continuous jog that stops on key release
  2. User can initiate a homing cycle and see the machine transition through homing states to Idle
  3. User can set work zero on any individual axis or all axes, and switch between G54-G59 coordinate systems with stored offsets displayed
  4. User can type a G-code command in the MDI console, see the machine response, and navigate command history with up/down arrows
**Plans:** 4 plans
  - [x] 03-01-PLAN.md -- CncController sendCommand() + CncJogPanel with jog buttons and homing
  - [x] 03-02-PLAN.md -- CncConsolePanel MDI console with input, history, and response display
  - [x] 03-03-PLAN.md -- CncWcsPanel with zero-set buttons and G54-G59 WCS selector
  - [x] 03-04-PLAN.md -- UIManager integration, keyboard jog, continuous jog, callback wiring

### Phase 4: Tool Integration
**Goal**: Operator selects a tool and wood species, and the application shows optimal cutting parameters calculated from the existing feeds/speeds engine
**Depends on**: Phase 2
**Requirements**: TAC-01, TAC-02, TAC-03
**Success Criteria** (what must be TRUE):
  1. User can select a tool geometry from the tool database within the controller UI context
  2. When both a tool and a wood species are selected, the feeds/speeds calculator automatically produces recommended RPM, feed rate, plunge rate, stepdown, and stepover
  3. Calculated cutting parameters are displayed in a reference panel that the operator can consult while setting up or running a job
**Plans:** 2 plans
  - [x] 04-01-PLAN.md -- CncToolPanel with tool selector, material selector, auto-calculator, result display
  - [x] 04-02-PLAN.md -- UIManager integration, workspace mode wiring, application dependency injection

### Phase 5: Job Streaming
**Goal**: Operator has full visibility into job progress, timing, and can see when actual cutting conditions deviate from recommendations
**Depends on**: Phase 4
**Requirements**: TAC-04, TAC-05, TAC-06, TAC-07, TAC-08
**Success Criteria** (what must be TRUE):
  1. During streaming, the running feed rate is compared to the calculator recommendation and a visual warning appears when deviation exceeds 20%
  2. Job elapsed time, estimated remaining time, current line number, total line count, and progress percentage are all displayed and update during streaming
  3. Time estimation adjusts based on current feed rate rather than showing a static estimate
**Plans**: TBD

### Phase 6: Job Safety
**Goal**: Operator can safely pause, stop, and resume jobs with confidence that the machine will not behave unexpectedly
**Depends on**: Phase 5
**Requirements**: SAF-01, SAF-02, SAF-03, SAF-04, SAF-05, SAF-06, SAF-07
**Success Criteria** (what must be TRUE):
  1. Pause button sends feed hold and visually changes state; resume button sends cycle start only when machine is in Hold state
  2. E-stop button sends soft reset with a confirmation dialog when a job is running, preventing accidental activation
  3. User can specify a line number to resume from, and the controller automatically rebuilds modal state (units, coordinate system, spindle, coolant, feed rate) by scanning all prior G-code
  4. Endstop, probe, and door sensor states are displayed from GRBL Pn: field data
  5. Before streaming begins, a pre-flight check verifies connection is active and no alarm state exists, with optional warnings for missing tool/material selection
**Plans**: TBD

### Phase 7: Firmware Settings
**Goal**: Operator can view, edit, back up, and restore GRBL firmware settings, and keep the DW machine profile in sync with the controller
**Depends on**: Phase 1
**Requirements**: FWC-01, FWC-02, FWC-03, FWC-04, FWC-05, FWC-06, FWC-07
**Success Criteria** (what must be TRUE):
  1. User can view all GRBL $$ settings with human-readable descriptions and edit individual settings with min/max/type validation
  2. User can export current GRBL settings to a JSON backup file and restore settings from a backup file with confirmation
  3. Machine tuning UI allows per-axis editing of steps/mm, max feed rate, and acceleration
  4. On connect, GRBL $$ settings are read into the DW machine profile; user can push DW machine profile values back to the GRBL controller
**Plans**: TBD

### Phase 8: Macros
**Goal**: Operator can store, organize, and execute custom G-code sequences for repetitive operations
**Depends on**: Phase 7
**Requirements**: FWC-08, FWC-09, FWC-10
**Success Criteria** (what must be TRUE):
  1. User can create, edit, and delete macros stored in SQLite with name, G-code content, and optional keyboard shortcut
  2. Executing a macro sends its G-code lines sequentially via the existing sendCommand path, with execution visible in the console
  3. Built-in macros for homing, Z-probe, and return-to-zero are available out of the box
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Fix Foundation | 3/3 | Complete | 2026-02-24 |
| 2. Status Display | 2/2 | Complete | 2026-02-24 |
| 3. Manual Control | 4/4 | Complete | 2026-02-24 |
| 4. Tool Integration | 0/0 | Not started | - |
| 5. Job Streaming | 0/0 | Not started | - |
| 6. Job Safety | 0/0 | Not started | - |
| 7. Firmware Settings | 0/0 | Not started | - |
| 8. Macros | 0/0 | Not started | - |

---
*Roadmap created: 2026-02-24*
*Last updated: 2026-02-24*
