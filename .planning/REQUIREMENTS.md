# Requirements: Digital Workshop

**Defined:** 2026-03-05
**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

## v0.5.0 Requirements

Requirements for Codebase Cleanup & Simplification milestone. Pure refactoring -- no user-facing behavior changes.

### Bug Fixes (BUG)

- [ ] **BUG-01**: GCodeMetadata in ImportQueue uses smart pointer instead of raw new/delete
- [ ] **BUG-02**: file::move() handles cross-filesystem moves via copy+delete fallback

### Monolithic Function Splits (SPLIT)

- [ ] **SPLIT-01**: Application::initWiring() (~905 lines) is decomposed into domain-specific wiring functions
- [ ] **SPLIT-02**: ImportQueue::processTask() (~540 lines) is decomposed into pipeline stage functions
- [ ] **SPLIT-03**: Schema::createTables() (~425 lines) is decomposed into per-table builder functions
- [ ] **SPLIT-04**: Config::load() (~373 lines) is decomposed into per-section parser functions
- [ ] **SPLIT-05**: Application::init() (~292 lines) is decomposed into initialization phase functions

### Duplicate Code Consolidation (DUP)

- [x] **DUP-01**: UI color constants (error red, success green, warning yellow) are centralized in a single header
- [x] **DUP-02**: ImGui 2-column label/value table pattern is extracted into a reusable helper function
- [ ] **DUP-03**: Inline edit buffer management (memset+snprintf pairs) is consolidated into a utility function

### Code Quality (QUAL)

- [ ] **QUAL-01**: Hardcoded UI scale factors in panel code are replaced with style-system-derived values
- [ ] **QUAL-02**: glClearColor uses theme/config values instead of hardcoded constants
- [ ] **QUAL-03**: application_wiring.cpp is under 800 lines (consequence of SPLIT-01)

### Oversized Files (SIZE)

- [ ] **SIZE-01**: Config::save() (~370 lines) is decomposed into per-section writer functions (symmetry with SPLIT-04)

## v0.6.0+ Requirements

Deferred to future release. Tracked but not in current roadmap.

### Deferred Refactoring

- **REF-01**: Config class split into concern-specific classes (AppConfig, RenderConfig, CncConfig, etc.)
- **REF-02**: Repository CRUD template base class (reduces 10 repos by ~1,100 lines)
- **REF-03**: gcode_panel.cpp decomposition (1956 lines)
- **REF-04**: cnc_controller.cpp decomposition (1351 lines -- justified complexity but monitor)
- **REF-05**: tool_database.cpp decomposition (1006 lines)
- **REF-06**: Windows serial port enumeration (TODO stub)
- **REF-07**: Undo/redo system
- **REF-08**: 3MF deflate compression full integration

## Out of Scope

| Feature | Reason |
|---------|--------|
| Config class split into concern classes | Too invasive for cleanup milestone, defer to v0.6.0 |
| Repository template base class | High-value but needs careful design, defer |
| CNC controller refactoring | Justified complexity, single responsibility |
| New features or behavior changes | This is a pure refactoring milestone |
| Thread safety for Config singleton | Known single-threaded by design |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUG-01 | Phase 27 | Pending |
| BUG-02 | Phase 27 | Pending |
| SPLIT-01 | Phase 28 | Pending |
| SPLIT-02 | Phase 28 | Pending |
| SPLIT-03 | Phase 28 | Pending |
| SPLIT-04 | Phase 28 | Pending |
| SPLIT-05 | Phase 28 | Pending |
| SIZE-01 | Phase 28 | Pending |
| QUAL-03 | Phase 28 | Pending |
| DUP-01 | Phase 29 | Complete |
| DUP-02 | Phase 29 | Complete |
| DUP-03 | Phase 29 | Pending |
| QUAL-01 | Phase 30 | Pending |
| QUAL-02 | Phase 30 | Pending |

**Coverage:**
- v0.5.0 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0

---
*Requirements defined: 2026-03-05*
*Last updated: 2026-03-05 -- traceability updated after roadmap creation*
