# Roadmap: Digital Workshop

## Milestones

- [x] **v0.1.x CNC Controller Suite** - Phases 1-8 (shipped 2026-02-25)
- [ ] **v0.2.0 Sender Feature Parity** - Phases 9-12 (in progress)

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
- [ ] **Phase 10: Core Sender** - Override controls, coolant toggles, alarm handling, status polling, precision jog, WCS quick-switch
- [ ] **Phase 11: Niceties** - DRO click-to-zero, move-to dialog, diagonal jog, console tags, recent files, keyboard overrides, macro reorder, job alerts
- [ ] **Phase 12: Extended** - Alarm/error references, probe workflows, M6 detection, nested macros, tool color viz, gamepad, TLS

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
**Plans:** 2 plans
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
**Plans**: TBD

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
**Plans**: TBD

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
| 10. Core Sender | v0.2.0 | 0/2 | Planned | - |
| 11. Niceties | v0.2.0 | 0/0 | Not started | - |
| 12. Extended | v0.2.0 | 0/0 | Not started | - |

---
*Roadmap created: 2026-02-24*
*Last updated: 2026-02-26 -- Phase 10 Core Sender planned (2 plans)*
