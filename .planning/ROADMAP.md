# Roadmap: Digital Workshop

## Milestones

- [x] **v0.1.x CNC Controller Suite** - Phases 1-8 (shipped 2026-02-25)
- [x] **v0.2.0 Sender Feature Parity** - Phases 9-13 (shipped 2026-02-27)
- [x] **v0.3.0 Direct Carve** - Phases 14-19 (completed 2026-02-28)
- [ ] **v0.4.0 Shared Materials & Project Costing** - Phases 20-26 (in progress)

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

### v0.4.0 Shared Materials & Project Costing (In Progress)

**Milestone Goal:** Unify the material system across the entire application and build project-level costing -- so every material, stock sheet, cut, and labor hour feeds into a single cost picture with estimated vs actual tracking.

- [x] **Phase 20: Data Foundation & Rename** - Stock sizes DB table, project cost JSON schema, rename estimator to costing (2026-03-04)
- [x] **Phase 21: Material Stock Sizes** - Parent-child material/stock model with dimensions, pricing, and common size suggestions (completed 2026-03-05)
- [x] **Phase 22: Consumables & Live Pricing** - Consumable items, tool cost entries, live DB pricing for non-material costs (completed 2026-03-05)
- [ ] **Phase 23: CLO Material Integration** - Cut optimizer uses shared material/stock system with waste breakdown and scrap tracking
- [ ] **Phase 24: Project Costing Engine** - Cost entry types (material, labor, overhead), estimated vs actual columns, persistence
- [ ] **Phase 25: CLO-to-Costing Pipeline** - CLO results auto-push into project costing, project-level cost aggregation
- [ ] **Phase 26: Estimate & Order Views** - Sale price and margin, estimate vs order toggle, formatted print output

## Phase Details (v0.4.0)

### Phase 20: Data Foundation & Rename
**Goal**: Data layer is ready to support the entire costing system -- stock sizes have a DB table, project costs have a JSON schema, and the old "estimator" naming is gone
**Depends on**: Phase 19 (previous milestone)
**Requirements**: COST-01, DATA-01, DATA-02
**Success Criteria** (what must be TRUE):
  1. A stock_sizes table exists in the database with columns for material FK, dimensions (width, height, thickness), and per-unit price
  2. Project folders contain a structured JSON file for project-specific costs (captured prices, actuals, sale price) that loads and saves correctly
  3. Every UI label, menu item, panel title, and code reference that said "Estimator" or "estimate" now says "Costing" or "cost"
**Plans**: 3 plans (2 waves)

Plans:
- [x] 20-01-PLAN.md -- Stock sizes DB table, StockSizeRepository CRUD, remove costPerBoardFoot (TDD, wave 1)
- [x] 20-02-PLAN.md -- Project costing JSON schema and I/O with estimates/orders/CLO results (TDD, wave 1)
- [x] 20-03-PLAN.md -- Rename estimator to costing across UI, code, DB, config (wave 2, depends on 20-01 + 20-02)

### Phase 21: Material Stock Sizes
**Goal**: Users can manage stock size entries on any material, seeing dimensions, pricing, and auto-suggested common sizes
**Depends on**: Phase 20
**Requirements**: MATL-01, MATL-02, MATL-03, MATL-04
**Success Criteria** (what must be TRUE):
  1. User can add a stock size entry to any material specifying width, height, thickness, and per-unit price
  2. User can edit and delete stock size entries from the Materials panel with changes persisting to the database
  3. When adding a new stock size, common dimension presets (4x8, 5x5 sheets; 2x4, 1x6 lumber) are suggested for quick selection
  4. Each stock size entry displays a calculated board-foot value and a derived cost-per-board-foot based on its dimensions and price
**Plans**: 2 plans (2 waves)

Plans:
- [ ] 21-01-PLAN.md -- Unit conversion + board-foot calculation utilities (TDD, wave 1)
- [ ] 21-02-PLAN.md -- Stock size CRUD UI in Materials panel detail view with presets (wave 2, depends on 21-01)

### Phase 22: Consumables & Live Pricing
**Goal**: Users can create consumable items and tool cost entries with live pricing that updates automatically from the database
**Depends on**: Phase 20
**Requirements**: MATL-05, DATA-03, COST-03, COST-04
**Success Criteria** (what must be TRUE):
  1. User can create consumable items (sandpaper, finish, glue) with a name, per-unit price, and unit-of-measure (e.g., sheet, ml, oz)
  2. Tool cost entries support flexible units (per hour, per minute, per object in batch) with pricing pulled live from the tool database
  3. Changes to consumable or tool prices in the database propagate automatically to any open project costing view without manual refresh
**Plans**: 2 plans (2 waves)

Plans:
- [ ] 22-01-PLAN.md -- Rate category data model, repository CRUD, schema v16, global + per-project overrides (TDD, wave 1)
- [ ] 22-02-PLAN.md -- Rate category UI in CostingPanel, live cost computation via recalc-on-render (wave 2, depends on 22-01)

### Phase 23: CLO Material Integration
**Goal**: Cut optimizer uses the shared material/stock system instead of hardcoded presets, with detailed waste breakdown and scrap tracking
**Depends on**: Phase 21
**Requirements**: CLO-01, CLO-02, CLO-03, CLO-04, DATA-04
**Success Criteria** (what must be TRUE):
  1. User selects material and stock size from the shared material system when setting up a CLO run -- no hardcoded presets remain
  2. After optimization, user sees an itemized waste breakdown: usable scrap pieces (with dimensions), kerf loss (blade width x cut length), and unusable waste
  3. User can optimize parts across multiple stock sizes in the same optimization run
  4. Usable scrap pieces can be flagged for reuse and appear as available stock in future optimization runs
  5. CLO results are stored in the project folder with references to stock_size IDs from the database
**Plans**: TBD

Plans:
- [ ] 23-01: Replace hardcoded presets with shared material/stock selection
- [ ] 23-02: Waste breakdown (scrap, kerf, unusable) and multi-stock optimization
- [ ] 23-03: Scrap tracking and reuse, CLO result persistence

### Phase 24: Project Costing Engine
**Goal**: Users can build a complete cost picture for a project with material, labor, and overhead entries, each showing estimated and actual costs, all persisting to the project folder
**Depends on**: Phase 20, Phase 22
**Requirements**: COST-02, COST-05, COST-06, COST-07, COST-10, COST-13
**Success Criteria** (what must be TRUE):
  1. Material costs are captured (snapshot) at point of addition to the project, preserving the historical price even if DB prices change later
  2. User can add labor entries with a configurable hourly rate, and overhead entries as user-defined line items
  3. Every cost line displays side-by-side estimated and actual columns, with actual editable via manual entry
  4. Cost categories (Material, Tooling, Consumable, Labor, Overhead) are visually grouped and labeled in the costing panel
  5. All costing data persists to the user's TheDigitalWorkshop data directory and survives application restart
**Plans**: TBD

Plans:
- [ ] 24-01: Cost entry model (categories, estimated/actual), material snapshot
- [ ] 24-02: Labor and overhead entries, cost panel UI
- [ ] 24-03: Costing data persistence to project folder

### Phase 25: CLO-to-Costing Pipeline
**Goal**: CLO results automatically feed into project costing and the project panel shows aggregated cost totals
**Depends on**: Phase 23, Phase 24
**Requirements**: CLO-05, COST-09
**Success Criteria** (what must be TRUE):
  1. When a CLO run completes, material costs (stock sheets used), waste amounts, and sheet count are automatically pushed as cost entries into the project costing view
  2. Project panel displays total estimated cost, total actual cost, and the difference -- aggregated from all cost entries across all categories
**Plans**: TBD

Plans:
- [ ] 25-01: Auto-push CLO results to costing, project-level cost aggregation

### Phase 26: Estimate & Order Views
**Goal**: Users can set a sale price to see margin, toggle between a working estimate and a finalized order view, and print either as a formatted document
**Depends on**: Phase 25
**Requirements**: COST-08, COST-11, COST-12
**Success Criteria** (what must be TRUE):
  1. User can set a sale price on the project; margin (sale price minus total cost) is calculated and displayed prominently
  2. User can toggle between Estimate view (editable, live prices update) and Order view (finalized receipt, locked values)
  3. User can print the estimate or order as a formatted document suitable for handing to a customer
**Plans**: TBD

Plans:
- [ ] 26-01: Sale price, margin calculation
- [ ] 26-02: Estimate vs Order toggle, print/export

## Progress (v0.4.0)

**Execution Order:** Phases 20 -> 21 -> 22 -> 23 -> 24 -> 25 -> 26
(Phases 21 and 22 can execute in parallel after Phase 20)

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 20. Data Foundation & Rename | v0.4.0 | 3/3 | Complete | 2026-03-04 |
| 21. Material Stock Sizes | 2/2 | Complete    | 2026-03-05 | - |
| 22. Consumables & Live Pricing | 2/2 | Complete    | 2026-03-05 | - |
| 23. CLO Material Integration | 2/3 | In Progress|  | - |
| 24. Project Costing Engine | v0.4.0 | 0/3 | Not started | - |
| 25. CLO-to-Costing Pipeline | v0.4.0 | 0/1 | Not started | - |
| 26. Estimate & Order Views | v0.4.0 | 0/2 | Not started | - |

---
*Roadmap created: 2026-02-24*
*Last updated: 2026-03-04 -- Phase 20 complete (3/3 plans, 5 commits, 846 tests passing)*
