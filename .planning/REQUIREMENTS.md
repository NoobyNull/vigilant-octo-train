# Requirements: Digital Workshop

**Defined:** 2026-03-09
**Core Value:** A woodworker can go from selecting a piece of wood and a cutting tool to safely running a CNC job with optimized feeds and speeds -- all without leaving the application.

## v0.6.0 Requirements

Requirements for Unified 3D Viewport milestone. Each maps to roadmap phases.

### Viewport Rendering (VPR)

- [ ] **VPR-01**: User can see 3D model mesh and G-code toolpath lines in the same viewport with a single shared camera
- [ ] **VPR-02**: User can toggle model visibility on/off via a toolbar button
- [ ] **VPR-03**: User can toggle toolpath visibility on/off via a toolbar button
- [ ] **VPR-04**: User can toggle individual move types (rapids, cuts, plunges, retracts) on/off
- [ ] **VPR-05**: User can toggle color-by-tool mode for multi-tool G-code
- [ ] **VPR-06**: User can filter visible toolpath depth with a Z-clip slider
- [ ] **VPR-07**: G-code cutting paths render with height-based color gradient showing depth

### Alignment (ALN)

- [ ] **ALN-01**: Model transforms to machine space using FitParams so it overlays G-code correctly
- [ ] **ALN-02**: System validates alignment by sampling ~1% of cutting points against mesh surface
- [ ] **ALN-03**: External .nc files render in viewport standalone without requiring a model

### Simulation (SIM)

- [ ] **SIM-01**: User can play/pause/scrub simulation playback in the viewport
- [ ] **SIM-02**: Simulation shows completed path (green), current segment (yellow), and cutter position (red dot)

### Code Elimination (ELM)

- [ ] **ELM-01**: GCodePanel no longer owns Renderer, Camera, or Framebuffer
- [ ] **ELM-02**: GCodePanel retains text listing, statistics, CNC sender controls, and file management
- [ ] **ELM-03**: Mouse interaction is identical regardless of visible layers

## Previous Milestone Requirements

<details>
<summary>v0.5.0 Codebase Cleanup (14 requirements)</summary>

### Bug Fixes (BUG)
- [ ] **BUG-01**: GCodeMetadata in ImportQueue uses smart pointer instead of raw new/delete
- [ ] **BUG-02**: file::move() handles cross-filesystem moves via copy+delete fallback

### Monolithic Function Splits (SPLIT)
- [ ] **SPLIT-01**: Application::initWiring() decomposed into domain-specific wiring functions
- [ ] **SPLIT-02**: ImportQueue::processTask() decomposed into pipeline stage functions
- [ ] **SPLIT-03**: Schema::createTables() decomposed into per-table builder functions
- [ ] **SPLIT-04**: Config::load() decomposed into per-section parser functions
- [ ] **SPLIT-05**: Application::init() decomposed into initialization phase functions

### Duplicate Code Consolidation (DUP)
- [x] **DUP-01**: UI color constants centralized in single header
- [x] **DUP-02**: ImGui 2-column label/value table helper extracted
- [ ] **DUP-03**: Edit buffer management consolidated into utility function

### Code Quality (QUAL)
- [ ] **QUAL-01**: Hardcoded UI scale factors replaced with style-derived values
- [ ] **QUAL-02**: glClearColor uses theme/config values
- [ ] **QUAL-03**: application_wiring.cpp under 800 lines

### Oversized Files (SIZE)
- [ ] **SIZE-01**: Config::save() decomposed into per-section writer functions

</details>

## Future Requirements

### Deferred

- **ALN-04**: Auto-alignment for external G-code files without FitParams
- **VPR-08**: Orthographic projection toggle
- **VPR-09**: Multi-model overlay support
- **REF-01**: Config class split into concern-specific classes
- **REF-02**: Repository CRUD template base class
- **REF-03**: gcode_panel.cpp decomposition (partially addressed by ELM-01)
- **REF-06**: Windows serial port enumeration
- **REF-07**: Undo/redo system
- **REF-08**: 3MF deflate compression full integration

## Out of Scope

| Feature | Reason |
|---------|--------|
| Auto-alignment for external G-code | No FitParams available; would require surface matching solver |
| Ortho projection mode | Current perspective works; can add later |
| Multi-model overlay | One model + one toolpath is the use case |
| Toolpath editing in viewport | View-only; editing is in Direct Carve workflow |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| VPR-01 | TBD | Pending |
| VPR-02 | TBD | Pending |
| VPR-03 | TBD | Pending |
| VPR-04 | TBD | Pending |
| VPR-05 | TBD | Pending |
| VPR-06 | TBD | Pending |
| VPR-07 | TBD | Pending |
| ALN-01 | TBD | Pending |
| ALN-02 | TBD | Pending |
| ALN-03 | TBD | Pending |
| SIM-01 | TBD | Pending |
| SIM-02 | TBD | Pending |
| ELM-01 | TBD | Pending |
| ELM-02 | TBD | Pending |
| ELM-03 | TBD | Pending |

**Coverage:**
- v0.6.0 requirements: 15 total
- Mapped to phases: 0
- Unmapped: 15

---
*Requirements defined: 2026-03-09*
*Last updated: 2026-03-09 after initial definition*
