# Roadmap: Digital Workshop

## Milestones

- [x] **v0.1.x CNC Controller Suite** - Phases 1-8 (shipped 2026-02-25)
- [ ] **v0.2.0 Sender Feature Parity** - Phases 9-13 (in progress)
- [ ] **v0.3.0 Direct Carve** - Phases 14-19 (planned)

## Phases

<details>
<summary>v0.1.x CNC Controller Suite (Phases 1-8) - SHIPPED 2026-02-25</summary>

- [x] **Phase 1: Fix Foundation** - Resolve thread safety, error handling, and disconnect detection bugs in existing CncController (2026-02-24)
- [x] **Phase 2: Status Display** - Real-time DRO showing position, machine state, feed rate, and spindle RPM (2026-02-24)
- [x] **Phase 3: Manual Control** - Jog, homing, work zero, coordinate systems, and MDI console (2026-02-24)
- [x] **Phase 4: Tool Integration** - Connect tool database and material system to feeds/speeds calculator (2026-02-24)
- [x] **Phase 5: Job Streaming** - Progress tracking, time estimation, and feed rate deviation warnings (2026-02-24)
- [x] **Phase 6: Job Safety** - Pause, resume, e-stop, safe job resume, sensor display, and pre-flight checks (2026-02-24)
- [x] **Phase 7: Firmware Settings** - GRBL settings panel, backup/restore, machine tuning, and profile sync (2026-02-25)
- [x] **Phase 8: Macros** - Macro storage, execution, and built-in common operations (2026-02-25)

</details>

### v0.2.0 Sender Feature Parity (In Progress)

**Milestone Goal:** Bring CNC sender to feature parity with dedicated senders (ncSender benchmark), prioritizing safety, core controls, workflow polish, and extended capabilities.

- [x] **Phase 9: Safety** - Configurable safety guards: long-press buttons, dead-man watchdog, door interlock, soft limits, pause-before-reset (2026-02-26)
- [x] **Phase 10: Core Sender** - Override controls, coolant toggles, alarm handling, status polling, precision jog, WCS quick-switch (completed 2026-02-27)
- [x] **Phase 11: Niceties** - DRO click-to-zero, move-to dialog, diagonal jog, console tags, recent files, keyboard overrides, macro reorder, job alerts (completed 2026-02-27)
- [x] **Phase 12: Extended** - Alarm/error references, probe workflows, M6 detection, nested macros, tool color viz, gamepad, TLS (completed 2026-02-27)

## Phase Details

### Phase 9: Safety
**Goal**: Operator has configurable safety guards that prevent accidental machine actions and protect against common failure modes
**Depends on**: Phase 8 (previous milestone)
**Requirements**: SAF-10, SAF-11, SAF-12, SAF-13, SAF-14, SAF-15, SAF-16, SAF-17
**Success Criteria** (what must be TRUE):
  1. Homing and job start buttons require a visible long-press hold before activating, preventing accidental triggering from a quick click
  2. Continuous jog movement stops automatically if the operator releases the key/button and no keepalive is received within the configured timeout
  3. When GRBL reports door pin active, rapid moves and spindle commands are blocked with a visible indicator explaining why
  4. Before streaming, G-code bounding box is compared against machine travel limits and the operator is warned if the job exceeds soft limits
  5. Every safety feature can be individually enabled or disabled through a dedicated safety settings UI section
**Plans:** 2 plans
Plans:
- [x] 09-01-PLAN.md -- Config keys, long-press buttons (Home/Start/Abort), dead-man watchdog
- [x] 09-02-PLAN.md -- Door interlock, soft limit pre-check, pause-before-reset, safety settings UI

### Phase 10: Core Sender
**Goal**: Operator has full real-time control over spindle speed, rapid rate, feed rate overrides, coolant, and jog precision while the machine runs
**Depends on**: Phase 9
**Requirements**: SND-01, SND-02, SND-03, SND-04, SND-05, SND-06, SND-07, SND-08
**Success Criteria** (what must be TRUE):
  1. Operator can adjust spindle speed override (0-200%) and rapid override (25/50/100%) via UI controls that send GRBL real-time commands immediately
  2. Coolant state (Flood/Mist/Off) is visible in the status panel and togglable via buttons that send M7/M8/M9 commands
  3. When GRBL enters alarm state, the alarm code number and a human-readable description are displayed with an inline unlock button
  4. Operator can jog in 0.01mm increments for precision zeroing, with separate configurable feedrates for small, medium, and large step groups
  5. WCS selector (G54-G59) is visible in the status panel header for quick switching without opening the full WCS panel
**Plans:** 2/2 plans complete
Plans:
- [ ] 10-01-PLAN.md -- Config keys, override controls (spindle/rapid/feed), coolant toggles, alarm display, status polling interval
- [ ] 10-02-PLAN.md -- Precision jog (0.01mm), per-step-group feedrates, WCS quick-switch selector

### Phase 11: Niceties
**Goal**: Common operator workflows are faster and more informative through interaction shortcuts, better feedback, and convenience features
**Depends on**: Phase 10
**Requirements**: NIC-01, NIC-02, NIC-03, NIC-04, NIC-05, NIC-06, NIC-07, NIC-08
**Success Criteria** (what must be TRUE):
  1. Double-clicking a DRO axis value zeros that axis, and a move-to dialog allows entering explicit XYZ coordinates for positioning
  2. Diagonal XY jog buttons allow simultaneous two-axis movement in all four diagonal directions
  3. Console messages are visually tagged by source ([JOB], [MDI], [MACRO], [SYS]) with distinct colors for each type
  4. Recent G-code files (last 10) appear in the GCode panel for quick re-loading, and keyboard shortcuts adjust feed/spindle overrides
  5. When a streaming job completes, the operator receives a visible toast notification and optional status bar flash
**Plans:** 3/3 plans complete
Plans:
- [ ] 11-01-PLAN.md -- DRO click-to-zero, move-to dialog, diagonal XY jog buttons
- [ ] 11-02-PLAN.md -- Console source tags, macro drag-and-drop reorder
- [ ] 11-03-PLAN.md -- Recent G-code files, keyboard override shortcuts, job completion notification

### Phase 12: Extended
**Goal**: Sender reaches full feature parity with dedicated senders through reference tables, probe workflows, tool change handling, and advanced macro capabilities
**Depends on**: Phase 10
**Requirements**: EXT-01, EXT-02, EXT-03, EXT-04, EXT-05, EXT-06, EXT-07, EXT-08, EXT-09, EXT-10, EXT-11, EXT-12, EXT-13, EXT-14, EXT-15, EXT-16
**Success Criteria** (what must be TRUE):
  1. GRBL alarm codes and error codes display human-readable descriptions inline when they occur, with full reference tables accessible from the UI
  2. Z-probe workflow guides the operator through approach speed, plate thickness, retract distance, and probe execution with a structured dialog
  3. When M6 tool change is encountered during streaming, the job pauses and waits for operator acknowledgment before continuing
  4. Macros support nested expansion via M98 with a recursion guard, and jog step sizes can be cycled via keyboard shortcut
  5. Gamepad input maps axes to jog movement and buttons to start/pause/stop/home actions via SDL_GameController
**Plans:** 5 plans
Plans:
- [x] 12-01-PLAN.md -- Complete GRBL alarm/error reference tables, firmware info display, log-to-file toggle
- [x] 12-02-PLAN.md -- Plain text GRBL settings export, G-code search/goto
- [x] 12-03-PLAN.md -- Out-of-bounds pre-check, probe LED indicator, Z-probe/TLS/3D probing workflows
- [x] 12-04-PLAN.md -- Cycle-steps keyboard shortcut, M6 tool change detection, M98 nested macros
- [x] 12-05-PLAN.md -- Per-tool color coding in toolpath, gamepad input via SDL_GameController

## Progress

**Execution Order:** Phases 9 -> 10 -> 11 -> 12

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Fix Foundation | v0.1.x | 3/3 | Complete | 2026-02-24 |
| 2. Status Display | v0.1.x | 2/2 | Complete | 2026-02-24 |
| 3. Manual Control | v0.1.x | 4/4 | Complete | 2026-02-24 |
| 4. Tool Integration | v0.1.x | 2/2 | Complete | 2026-02-24 |
| 5. Job Streaming | v0.1.x | 2/2 | Complete | 2026-02-24 |
| 6. Job Safety | v0.1.x | 4/4 | Complete | 2026-02-24 |
| 7. Firmware Settings | v0.1.x | 2/2 | Complete | 2026-02-25 |
| 8. Macros | v0.1.x | 3/3 | Complete | 2026-02-25 |
| 9. Safety | v0.2.0 | 2/2 | Complete | 2026-02-26 |
| 10. Core Sender | v0.2.0 | Complete    | 2026-02-27 | - |
| 11. Niceties | 3/3 | Complete    | 2026-02-27 | - |
| 12. Extended | v0.2.0 | 0/5 | Planned | - |

### Phase 13: TCP/IP byte stream transport for CNC connections

**Goal:** Operator can connect to CNC controllers over TCP/IP in addition to serial, using an abstract byte stream interface that keeps all existing functionality working identically
**Depends on:** Phase 12
**Requirements**: TCP-01, TCP-02, TCP-03, TCP-04, TCP-05
**Success Criteria** (what must be TRUE):
  1. SerialPort implements an abstract IByteStream interface, and CncController uses it polymorphically via unique_ptr
  2. TcpSocket class provides a raw TCP transport implementing the same IByteStream interface with non-blocking connect, TCP_NODELAY, and disconnect detection
  3. CncController has a connectTcp(host, port) method that creates a TCP connection and runs the same IO thread as serial
  4. Connection bar UI has a Serial/TCP mode selector, with host:port inputs for TCP mode
  5. All existing serial and simulator connections work identically through the new interface
**Plans:** 3/3 plans complete

Plans:
- [ ] 13-01-PLAN.md -- Extract IByteStream interface, refactor SerialPort and CncController
- [ ] 13-02-PLAN.md -- Implement TcpSocket class, add connectTcp() to CncController, tests
- [ ] 13-03-PLAN.md -- Connection bar UI with Serial/TCP mode selector

### v0.3.0 Direct Carve

**Milestone Goal:** Stream 2.5D carving toolpaths directly from STL models to the CNC machine without generating G-code files, using a guided workflow with automatic tool recommendation and surgical clearing.

- [x] **Phase 14: Heightmap Engine** - Rasterize STL to 2D heightmap grid with configurable resolution, model fitting (scale/position/depth), background computation (completed 2026-02-28)
- [x] **Phase 15: Model Analysis** - Minimum feature radius detection, island identification (enclosed depressions requiring clearing), visual annotation on heightmap preview (completed 2026-02-28)
- [x] **Phase 16: Tool Recommendation** - Automatic tool selection from database based on model analysis, V-bit/ball nose/TBN/clearing tool suggestions with feeds/speeds (completed 2026-02-28)
- [x] **Phase 17: Toolpath Generation** - Raster scan toolpath with configurable axis, stepover presets, milling direction, safe Z, surgical island clearing (completed 2026-02-28)
- [ ] **Phase 18: Guided Workflow** - Step-by-step wizard: machine check, model fitting, tool selection, material setup, preview, outline test, zero, commit, run
- [ ] **Phase 19: Streaming Integration** - Point-by-point streaming via CncController, pause/resume/abort, live progress, optional G-code file export

## Phase Details (v0.3.0)

### Phase 14: Heightmap Engine
**Goal**: STL model is converted to a 2D heightmap grid that accurately represents the top-down surface for 2.5D machining, with user control over scale, position, and depth
**Depends on**: Phase 13 (CNC infrastructure complete)
**Requirements**: DC-01, DC-02, DC-03, DC-04
**Success Criteria** (what must be TRUE):
  1. Any loaded STL model produces a heightmap grid where each cell contains the maximum Z value from ray-mesh intersection at that XY position
  2. Model can be uniformly scaled (locked aspect ratio) and Z depth independently controlled, with live preview updating as parameters change
  3. Model position on material stock is adjustable (X/Y offset) with visual bounds checking against machine travel limits
  4. Heightmap computation for a 500K triangle model at 0.1mm resolution completes in under 5 seconds on a background thread
Plans:
- [ ] 14-01-PLAN.md -- Heightmap data structure, ray-mesh intersection, resolution config
- [ ] 14-02-PLAN.md -- Model fitting (uniform scale, Z depth, X/Y position), bounds checking, background computation

### Phase 15: Model Analysis
**Goal**: Heightmap is analyzed to identify minimum feature radius and island regions that require special tooling, with results displayed visually
**Depends on**: Phase 14
**Requirements**: DC-05, DC-06, DC-07, DC-08
**Success Criteria** (what must be TRUE):
  1. Minimum concave feature radius is computed from heightmap curvature and displayed as a measurement annotation
  2. Island regions (enclosed depressions) are detected with flood-fill from the heightmap surface, each classified by depth and area
  3. Island boundaries are highlighted on the heightmap preview with distinct coloring to show regions requiring clearing passes
  4. Analysis runs automatically after heightmap generation and updates when model fitting parameters change
Plans:
- [ ] 15-01-PLAN.md -- Minimum feature radius computation from heightmap curvature analysis
- [ ] 15-02-PLAN.md -- Island detection via flood-fill, classification by depth/area, visual annotation

### Phase 16: Tool Recommendation
**Goal**: System automatically recommends appropriate tools from the database based on model geometry analysis, with clear reasoning for each suggestion
**Depends on**: Phase 15
**Requirements**: DC-09, DC-10, DC-11, DC-12, DC-13
**Success Criteria** (what must be TRUE):
  1. Tool recommendation queries the .vtdb tool database and returns ranked suggestions with type, diameter, and reasoning
  2. V-bit is recommended for finishing when model allows taper-only contact (no buried sections outside islands)
  3. Ball nose or TBN is recommended when minimum feature radius exceeds the tool tip radius
  4. Clearing tool is recommended only when islands are detected, with diameter sized to island geometry
  5. Each recommendation includes feed rate, spindle speed, and stepover from the tool database cutting data
Plans:
- [ ] 16-01-PLAN.md -- Tool database query interface, V-bit/ball nose/TBN selection logic
- [ ] 16-02-PLAN.md -- Clearing tool recommendation for islands, feeds/speeds display, UI presentation

### Phase 17: Toolpath Generation
**Goal**: Heightmap and tool parameters are converted into an efficient machining toolpath with configurable scan strategy, stepover, and surgical clearing for island regions only
**Depends on**: Phase 16
**Requirements**: DC-14, DC-15, DC-16, DC-17, DC-18, DC-19
**Success Criteria** (what must be TRUE):
  1. Raster scan generates parallel toolpath lines along user-selected axis (X, Y, X-then-Y, Y-then-X) with correct tool offset compensation
  2. Stepover presets (Ultra Fine 1%, Fine 8%, Basic 12%, Rough 25%, Roughing 40%) produce correct line spacing relative to tool tip diameter
  3. Milling direction (climb/conventional/alternating) controls scan line direction correctly
  4. Clearing passes are generated only for identified island regions with appropriate tool, not the entire surface
  5. Toolpath respects safe Z height, machine travel limits, and produces valid motion commands
Plans:
- [ ] 17-01-PLAN.md -- Raster scan generation, axis selection, stepover calculation, milling direction
- [ ] 17-02-PLAN.md -- Tool offset compensation, safe Z retract, travel limit enforcement
- [ ] 17-03-PLAN.md -- Surgical island clearing pass generation, multi-pass strategy

### Phase 18: Guided Workflow
**Goal**: Operator is guided through every step from model loading to job execution via a wizard interface that enforces safety checks and prevents common mistakes
**Depends on**: Phase 17
**Requirements**: DC-20, DC-21, DC-22, DC-23, DC-24, DC-25
**Success Criteria** (what must be TRUE):
  1. Wizard progresses through sequential steps with back/next navigation, each step validating before allowing progression
  2. Machine readiness check verifies connection, homing state, safe Z, and machine profile before proceeding
  3. Model fitting step shows live 3D preview with stock outline, scale/position/depth controls, and machine bounds warning
  4. Preview step shows toolpath overlaid on heightmap with estimated carving time and total line count
  5. Outline test traces the job perimeter at safe Z so operator can verify work area before committing
Plans:
- [ ] 18-01-PLAN.md -- Wizard framework, step navigation, machine readiness checks
- [ ] 18-02-PLAN.md -- Model fitting UI, preview rendering, outline test execution
- [ ] 18-03-PLAN.md -- Confirmation summary, commit flow, run integration

### Phase 19: Streaming Integration
**Goal**: Generated toolpath streams point-by-point to the CNC controller with full job control (pause/resume/abort), live progress, and optional file export
**Depends on**: Phase 18
**Requirements**: DC-26, DC-27, DC-28, DC-29, DC-30
**Success Criteria** (what must be TRUE):
  1. Toolpath points are converted to G0/G1 commands and streamed via CncController character-counting protocol
  2. Pause, resume, and abort work identically to G-code file streaming with same safety guarantees
  3. G-code panel shows live progress (current line, percentage, elapsed time, estimated remaining)
  4. Commands are generated on-the-fly from the toolpath buffer without pre-building a complete G-code file in memory
  5. Operator can optionally save the generated toolpath as a .nc file for future replay
Plans:
- [ ] 19-01-PLAN.md -- On-the-fly G-code generation, streaming adapter for CncController
- [ ] 19-02-PLAN.md -- Job control (pause/resume/abort), progress reporting, G-code file export

## Progress (v0.3.0)

**Execution Order:** Phases 14 -> 15 -> 16 -> 17 -> 18 -> 19

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 14. Heightmap Engine | 2/2 | Complete    | 2026-02-28 | - |
| 15. Model Analysis | 2/2 | Complete    | 2026-02-28 | - |
| 16. Tool Recommendation | 2/2 | Complete    | 2026-02-28 | - |
| 17. Toolpath Generation | 3/3 | Complete    | 2026-02-28 | - |
| 18. Guided Workflow | 1/3 | In Progress|  | - |
| 19. Streaming Integration | v0.3.0 | 0/2 | Planned | - |

---
*Roadmap created: 2026-02-24*
*Last updated: 2026-02-28 -- v0.3.0 Direct Carve milestone added (6 phases, 30 requirements, 14 plans)*
