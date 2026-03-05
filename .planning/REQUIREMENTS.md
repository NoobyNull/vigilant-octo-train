# Requirements: Digital Workshop

**Defined:** 2026-03-04
**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

## v0.4.0 Requirements

Requirements for Shared Materials & Project Costing milestone. Each maps to roadmap phases.

### Materials & Stock (MATL)

- [ ] **MATL-01**: User can add stock size entries to any material with dimensions (WxH*thickness) and per-unit price
- [ ] **MATL-02**: User can add, edit, and delete stock sizes from the Materials panel
- [ ] **MATL-03**: User sees auto-suggested common sizes when adding stock (4x8, 5x5 for sheets; 2x4, 1x6 for lumber)
- [x] **MATL-04**: User can view board foot calculation and cost-per-board-foot derived from stock dimensions and price
- [x] **MATL-05**: User can create consumable items (sandpaper, finish, glue) with per-unit pricing and unit-of-measure

### Cut List Optimizer (CLO)

- [ ] **CLO-01**: User selects material and stock size from the shared material system instead of hardcoded presets
- [ ] **CLO-02**: User sees itemized waste breakdown: usable scrap pieces, kerf loss (blade width * cut length), and unusable waste
- [ ] **CLO-03**: User can optimize parts across multiple stock sizes in the same run
- [ ] **CLO-04**: User can track usable scrap pieces and flag them for reuse in future optimizations
- [ ] **CLO-05**: CLO results automatically push material costs, waste, and sheet count into project costing

### Project Costing (COST)

- [ ] **COST-01**: Application uses "Costing" instead of "Estimator" throughout UI and code
- [ ] **COST-02**: Material costs are captured (snapshot) at point of addition to project
- [x] **COST-03**: Tool costs are per-unit with flexible units (hours, minutes, per-object-in-batch), live from DB
- [x] **COST-04**: Consumable costs track quantity used, live pricing from DB
- [ ] **COST-05**: Labor entries with configurable hourly rate
- [ ] **COST-06**: Overhead entries (user-defined line items)
- [ ] **COST-07**: Each cost line shows estimated and actual columns; actual is manual entry
- [ ] **COST-08**: User can set a sale price; margin (sale price minus total cost) is calculated and displayed
- [ ] **COST-09**: Project panel shows total estimated cost, total actual cost, and margin aggregated from all entries
- [ ] **COST-10**: Cost categories: Material, Tooling, Consumable, Labor, Overhead
- [ ] **COST-11**: User can toggle between Estimate view (working/editable) and Order view (finalized receipt)
- [ ] **COST-12**: User can print the estimate or order as a formatted document
- [ ] **COST-13**: Costing data persists to user's TheDigitalWorkshop data directory

### Data Architecture (DATA)

- [ ] **DATA-01**: Stock sizes stored in DB table with FK to materials (single source of truth for current prices)
- [ ] **DATA-02**: Project-specific costs (captured material prices, actuals, sale price) stored in project folder as JSON
- [x] **DATA-03**: Live pricing (tools, consumables) reads directly from DB -- changes propagate automatically
- [ ] **DATA-04**: CLO results stored in project folder with references to stock_size IDs

## Design Decisions

**Three cost categories with different pricing behaviors:**

| Category | Examples | Price Source | Behavior |
|----------|----------|-------------|----------|
| Material | Pine 4x8, walnut board | Captured at point of addition | Snapshot -- historical accuracy |
| Mine (tooling) | V-bit, sanding disc | Live from DB, per-unit (hrs/min/per-object) | Real-time -- reflects current replacement cost |
| Consumable | Sandpaper, finish, glue | Live from DB, per-unit | Real-time -- reflects current market price |

**Estimate vs Order:**
- Estimate = working view, editable, live prices update automatically
- Order = finalized receipt, locked, represents what was actually charged

**Data split:**
- Database = global truth (materials, stock prices, tool costs, consumable prices)
- Project folder = project-specific (captured material prices, actual costs paid, sale price, CLO results)

**Git versioning:** Skip v0.3.0 tag -- go directly to v0.4.0 to align with GSD milestone numbering.

## Previous Milestone Requirements

### v0.2.0: Sender Feature Parity (Phases 9-13)

8 SAF requirements (complete), 8 SND requirements (pending), 8 NIC requirements (pending), 16 EXT requirements (complete), 5 TCP requirements (complete).

### v0.3.0: Direct Carve (Phases 14-19)

30 DC requirements (complete). See REQUIREMENTS-DIRECT-CARVE.md.

## Future Requirements

### Deferred

- **INV-01**: Material inventory tracking (how many sheets on hand)
- **INV-02**: Supplier/vendor info on stock entries
- **INV-03**: Tax/markup calculation presets
- **TOOL-01**: Tool wear tracking with automatic replacement alerts
- **TOOL-02**: Per-tool cost history over time

## Out of Scope

| Feature | Reason |
|---------|--------|
| Accounting integration (QuickBooks, etc.) | Desktop app, not an accounting tool |
| Multi-currency support | USD-only for now, user is domestic |
| Cloud-based price updates | No cloud sync -- user manages prices manually |
| Purchase order generation | Receipt/order view covers the output need |
| Git tag v0.3.0 | Skipped to align with GSD versioning -- Direct Carve was on feature branch |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| MATL-01 | Phase 21 | Pending |
| MATL-02 | Phase 21 | Pending |
| MATL-03 | Phase 21 | Pending |
| MATL-04 | Phase 21 | Complete |
| MATL-05 | Phase 22 | Complete |
| CLO-01 | Phase 23 | Pending |
| CLO-02 | Phase 23 | Pending |
| CLO-03 | Phase 23 | Pending |
| CLO-04 | Phase 23 | Pending |
| CLO-05 | Phase 25 | Pending |
| COST-01 | Phase 20 | Pending |
| COST-02 | Phase 24 | Pending |
| COST-03 | Phase 22 | Complete |
| COST-04 | Phase 22 | Complete |
| COST-05 | Phase 24 | Pending |
| COST-06 | Phase 24 | Pending |
| COST-07 | Phase 24 | Pending |
| COST-08 | Phase 26 | Pending |
| COST-09 | Phase 25 | Pending |
| COST-10 | Phase 24 | Pending |
| COST-11 | Phase 26 | Pending |
| COST-12 | Phase 26 | Pending |
| COST-13 | Phase 24 | Pending |
| DATA-01 | Phase 20 | Pending |
| DATA-02 | Phase 20 | Pending |
| DATA-03 | Phase 22 | Complete |
| DATA-04 | Phase 23 | Pending |

**Coverage:**
- v0.4.0 requirements: 27 total
- Mapped to phases: 27/27
- Unmapped: 0

---
*Requirements defined: 2026-03-04*
*Last updated: 2026-03-04 -- traceability updated with phase mappings*
