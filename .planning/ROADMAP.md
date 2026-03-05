# Roadmap: Digital Workshop

## Milestones

- [x] **v0.1.x CNC Controller Suite** - Phases 1-8 (shipped 2026-02-25)
- [x] **v0.2.0 Sender Feature Parity** - Phases 9-13 (shipped 2026-02-27)
- [x] **v0.3.0 Direct Carve** - Phases 14-19 (completed 2026-02-28)
- [x] **v0.4.0 Shared Materials & Project Costing** - Phases 20-26 (completed 2026-03-05)
- [ ] **v0.5.0 Codebase Cleanup & Simplification** - Phases 27-30 (in progress)

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

<details>
<summary>v0.2.0 Sender Feature Parity (Phases 9-13) - SHIPPED 2026-02-27</summary>

- [x] **Phase 9: Safety** - Configurable safety guards: long-press buttons, dead-man watchdog, door interlock, soft limits, pause-before-reset (2026-02-26)
- [x] **Phase 10: Core Sender** - Override controls, coolant toggles, alarm handling, status polling, precision jog, WCS quick-switch (2026-02-27)
- [x] **Phase 11: Niceties** - DRO click-to-zero, move-to dialog, diagonal jog, console tags, recent files, keyboard overrides, macro reorder, job alerts (2026-02-27)
- [x] **Phase 12: Extended** - Alarm/error references, probe workflows, M6 detection, nested macros, tool color viz, gamepad, TLS (2026-02-27)
- [x] **Phase 13: TCP/IP Transport** - Abstract byte stream interface, TCP socket, connection bar UI (2026-02-27)

</details>

<details>
<summary>v0.3.0 Direct Carve (Phases 14-19) - COMPLETED 2026-02-28</summary>

- [x] **Phase 14: Heightmap Engine** - Rasterize STL to 2D heightmap grid with configurable resolution, model fitting, background computation (2026-02-28)
- [x] **Phase 15: Model Analysis** - Minimum feature radius detection, island identification, visual annotation on heightmap preview (2026-02-28)
- [x] **Phase 16: Tool Recommendation** - Automatic tool selection from database based on model analysis (2026-02-28)
- [x] **Phase 17: Toolpath Generation** - Raster scan toolpath with configurable axis, stepover, milling direction, surgical island clearing (2026-02-28)
- [x] **Phase 18: Guided Workflow** - Step-by-step wizard with safety checks, preview, outline test (2026-02-28)
- [x] **Phase 19: Streaming Integration** - Point-by-point streaming via CncController, pause/resume/abort, G-code export (2026-02-28)

</details>

<details>
<summary>v0.4.0 Shared Materials & Project Costing (Phases 20-26) - COMPLETED 2026-03-05</summary>

- [x] **Phase 20: Data Foundation & Rename** - Stock sizes DB table, project cost JSON schema, rename estimator to costing (2026-03-04)
- [x] **Phase 21: Material Stock Sizes** - Parent-child material/stock model with dimensions, pricing, and common size suggestions (2026-03-05)
- [x] **Phase 22: Consumables & Live Pricing** - Consumable items, tool cost entries, live DB pricing for non-material costs (2026-03-05)
- [x] **Phase 23: CLO Material Integration** - Cut optimizer uses shared material/stock system with waste breakdown and scrap tracking (2026-03-05)
- [x] **Phase 24: Project Costing Engine** - Cost entry types (material, labor, overhead), estimated vs actual columns, persistence (2026-03-05)
- [x] **Phase 25: CLO-to-Costing Pipeline** - CLO results auto-push into project costing, project-level cost aggregation (2026-03-05)
- [x] **Phase 26: Estimate & Order Views** - Sale price and margin, estimate vs order toggle, formatted print output (2026-03-05)

</details>

### v0.5.0 Codebase Cleanup & Simplification (In Progress)

**Milestone Goal:** Address all findings from the comprehensive codebase audit -- fix bugs, split monolithic functions, consolidate duplicate code, and eliminate hardcoded UI values -- so the codebase is correct, readable, and maintainable with no behavior changes.

- [ ] **Phase 27: Bug Fixes & Safety** - Fix raw new/delete in ImportQueue and cross-filesystem file::move() fallback
- [ ] **Phase 28: Monolithic Function Splits** - Decompose five oversized functions and bring all files under the 800-line limit
- [ ] **Phase 29: Duplicate Code Consolidation** - Centralize UI color constants, ImGui table helpers, and edit buffer management
- [ ] **Phase 30: Code Quality Polish** - Replace hardcoded UI scale factors and glClearColor constants with style/config-derived values

## Phase Details (v0.5.0)

### Phase 27: Bug Fixes & Safety
**Goal**: Known correctness bugs are fixed -- ImportQueue manages GCodeMetadata with a smart pointer and file::move() handles cross-filesystem moves without data loss
**Depends on**: Phase 26 (v0.4.0 complete)
**Requirements**: BUG-01, BUG-02
**Success Criteria** (what must be TRUE):
  1. ImportQueue creates GCodeMetadata via std::unique_ptr or std::make_unique -- no raw new or delete calls exist in the import pipeline
  2. Moving a file between different filesystems (e.g., /tmp to /data) succeeds by falling back to copy+delete, with the original removed only after a verified copy
  3. All 931+ tests continue to pass after both fixes
**Plans**: TBD

Plans:
- [ ] 27-01: Replace raw new/delete for GCodeMetadata with smart pointer in ImportQueue
- [ ] 27-02: Add cross-filesystem copy+delete fallback to file::move()

### Phase 28: Monolithic Function Splits
**Goal**: The five largest functions in the codebase are each decomposed into focused, named sub-functions -- no single function exceeds a readable length and all split-out files stay under 800 lines
**Depends on**: Phase 27
**Requirements**: SPLIT-01, SPLIT-02, SPLIT-03, SPLIT-04, SPLIT-05, SIZE-01, QUAL-03
**Success Criteria** (what must be TRUE):
  1. Application::initWiring() delegates to domain-specific wiring functions (e.g., wireImport, wireMaterials, wireCosting) -- the top-level function is a sequenced call list, not a 900-line body
  2. ImportQueue::processTask() is decomposed into named pipeline stage functions -- each stage is independently readable and the dispatch loop is short
  3. Schema::createTables() calls per-table builder functions -- adding a new table means adding one function, not appending to a monolith
  4. Config::load() and Config::save() each delegate to per-section parser/writer functions with symmetrical structure
  5. Application::init() is decomposed into initialization phase functions and application_wiring.cpp is under 800 lines
  6. All 931+ tests continue to pass after all splits
**Plans**: TBD

Plans:
- [ ] 28-01: Decompose Application::initWiring() into domain wiring functions
- [ ] 28-02: Decompose ImportQueue::processTask() into pipeline stage functions
- [ ] 28-03: Decompose Schema::createTables() into per-table builder functions
- [ ] 28-04: Decompose Config::load() and Config::save() into per-section functions
- [ ] 28-05: Decompose Application::init() and verify application_wiring.cpp line count

### Phase 29: Duplicate Code Consolidation
**Goal**: Repeated UI patterns that appear across multiple files are extracted into single authoritative locations -- color constants, table layout helpers, and edit buffer utilities each live in one place
**Depends on**: Phase 28
**Requirements**: DUP-01, DUP-02, DUP-03
**Success Criteria** (what must be TRUE):
  1. A single header defines the canonical error-red, success-green, and warning-yellow ImVec4 constants -- no panel file defines its own copies of these colors
  2. A reusable helper function renders the ImGui 2-column label/value table pattern -- call sites pass label and value, not repeated BeginTable/TableSetColumnIndex boilerplate
  3. Inline edit buffer management (memset+snprintf pairs) is replaced by a utility function or wrapper at all call sites -- no panel contains raw memset+snprintf for buffer prep
  4. All 931+ tests continue to pass after consolidation
**Plans**: TBD

Plans:
- [ ] 29-01: Centralize UI color constants (error, success, warning) in a shared header
- [ ] 29-02: Extract ImGui 2-column label/value table into a reusable helper
- [ ] 29-03: Consolidate inline edit buffer management into a utility function

### Phase 30: Code Quality Polish
**Goal**: All hardcoded numeric values in visual and GUI code are replaced with values derived from the style system or config -- the codebase satisfies the no-hardcoded-values policy throughout
**Depends on**: Phase 29
**Requirements**: QUAL-01, QUAL-02
**Success Criteria** (what must be TRUE):
  1. Panel code contains no hardcoded UI scale factors -- spacing, padding, and size multipliers reference ImGui style members or config values, not raw numeric literals
  2. glClearColor is called with values from the active theme or config, not hardcoded float constants -- changing the theme background updates the clear color
  3. All 931+ tests continue to pass after the replacements
**Plans**: TBD

Plans:
- [ ] 30-01: Replace hardcoded UI scale factors in panel code with style-derived values
- [ ] 30-02: Replace hardcoded glClearColor constants with theme/config values

## Progress (v0.5.0)

**Execution Order:** Phase 27 -> 28 -> 29 -> 30 (sequential -- each builds on the previous)

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 27. Bug Fixes & Safety | v0.5.0 | 0/2 | Not started | - |
| 28. Monolithic Function Splits | v0.5.0 | 0/5 | Not started | - |
| 29. Duplicate Code Consolidation | v0.5.0 | 0/3 | Not started | - |
| 30. Code Quality Polish | v0.5.0 | 0/2 | Not started | - |

---
*Roadmap created: 2026-02-24*
*Last updated: 2026-03-05 -- v0.5.0 phases 27-30 added*
