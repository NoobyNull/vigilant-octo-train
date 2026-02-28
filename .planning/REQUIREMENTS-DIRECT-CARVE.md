# Requirements: Digital Workshop -- Direct Carve

**Defined:** 2026-02-28
**Core Value:** A woodworker can load an STL model, select a tool and material, and stream a 2.5D carving toolpath directly to the CNC machine — no G-code files, no external CAM software.

## Direct Carve Requirements

Requirements for the v0.3.0 Direct Carve milestone. Each maps to roadmap phases 14-19.

### Heightmap Engine

- [x] **DC-01**: STL model is rasterized to a 2D heightmap grid via ray-mesh intersection on the XY plane, with configurable resolution (points per mm)
- [x] **DC-02**: Heightmap supports uniform scaling (locked aspect ratio) with independent Z depth control from top surface
- [x] **DC-03**: Model can be positioned on the material stock (X/Y offset), with bounds checked against machine travel limits
- [x] **DC-04**: Heightmap computation runs on a background thread with progress reporting to the UI

### Model Analysis

- [ ] **DC-05**: Minimum feature radius analysis identifies the smallest concave radius in the heightmap for tool selection guidance
- [ ] **DC-06**: Island detection finds enclosed depressions where tapered/ball nose bits would be buried, requiring a clearing pass
- [ ] **DC-07**: Island regions are classified by depth and area to determine minimum clearing tool diameter
- [ ] **DC-08**: Analysis results are displayed visually on the heightmap preview (islands highlighted, minimum radius annotated)

### Tool Recommendation

- [ ] **DC-09**: Automatic tool recommendation queries the tool database for compatible tools based on model analysis results
- [x] **DC-10**: V-bit recommended when model geometry allows taper-only finishing (no clearing needed outside islands)
- [x] **DC-11**: Ball nose / tapered ball nose recommended when minimum feature radius exceeds the tool tip radius
- [x] **DC-12**: Clearing tool recommended only for identified island regions, with diameter sized to island geometry
- [x] **DC-13**: Tool recommendation displays feed rate, spindle speed, and stepover from the tool database cutting data

### Toolpath Generation

- [ ] **DC-14**: Raster scan toolpath generates parallel lines along user-selected axis (X-only, Y-only, X-then-Y, Y-then-X)
- [ ] **DC-15**: Stepover is configurable as percentage of tool tip diameter with presets: Ultra Fine 1%, Fine 8%, Basic 12%, Rough 25%, Roughing 40%
- [ ] **DC-16**: Milling direction selectable: climb, conventional, or alternating (bidirectional)
- [ ] **DC-17**: Safe Z height configurable with automatic retract between scan lines and around island boundaries
- [ ] **DC-18**: Clearing passes generate only for identified island regions (surgical clearing), not the entire surface
- [x] **DC-19**: Toolpath respects machine profile travel limits (acceleration managed by GRBL firmware)

### Guided Workflow

- [x] **DC-20**: Step-by-step wizard UI guides operator through: machine check, model fitting, tool selection, material setup, preview, outline test, zero, commit, run
- [x] **DC-21**: Machine readiness check verifies: connection active, homed, safe Z tested, machine profile configured
- [ ] **DC-22**: Model fitting step shows 3D preview with stock dimensions, allows scale/position/depth adjustment
- [ ] **DC-23**: Preview step shows toolpath overlay on heightmap with estimated time and line count
- [ ] **DC-24**: Outline test step runs a perimeter trace at safe Z to verify work area before committing to cut
- [ ] **DC-25**: Confirmation step shows full summary (tool, material, feeds/speeds, stepover, estimated time) with commit button

### Streaming Integration

- [ ] **DC-26**: Toolpath streams point-by-point to CncController using existing character-counting protocol
- [ ] **DC-27**: Streaming supports pause/resume/abort with same safety guarantees as G-code job streaming
- [ ] **DC-28**: Live progress shown in G-code panel (current line, percentage, elapsed/remaining time)
- [ ] **DC-29**: Streaming generates G-code commands on the fly (G0/G1 with F rates) rather than pre-building a file
- [ ] **DC-30**: Job can optionally be saved as a .nc G-code file after generation for future replay without re-analysis

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| DC-01 | Phase 14: Heightmap Engine | Complete |
| DC-02 | Phase 14: Heightmap Engine | Complete |
| DC-03 | Phase 14: Heightmap Engine | Complete |
| DC-04 | Phase 14: Heightmap Engine | Complete |
| DC-05 | Phase 15: Model Analysis | Planned |
| DC-06 | Phase 15: Model Analysis | Planned |
| DC-07 | Phase 15: Model Analysis | Planned |
| DC-08 | Phase 18: Guided Workflow | Planned |
| DC-09 | Phase 18: Guided Workflow | Planned |
| DC-10 | Phase 16: Tool Recommendation | Complete |
| DC-11 | Phase 16: Tool Recommendation | Complete |
| DC-12 | Phase 16: Tool Recommendation | Complete |
| DC-13 | Phase 16: Tool Recommendation | Complete |
| DC-14 | Phase 17: Toolpath Generation | Planned |
| DC-15 | Phase 17: Toolpath Generation | Planned |
| DC-16 | Phase 17: Toolpath Generation | Planned |
| DC-17 | Phase 17: Toolpath Generation | Planned |
| DC-18 | Phase 17: Toolpath Generation | Planned |
| DC-19 | Phase 17: Toolpath Generation | Planned |
| DC-20 | Phase 18: Guided Workflow | Complete |
| DC-21 | Phase 18: Guided Workflow | Complete |
| DC-22 | Phase 18: Guided Workflow | Planned |
| DC-23 | Phase 18: Guided Workflow | Planned |
| DC-24 | Phase 18: Guided Workflow | Planned |
| DC-25 | Phase 18: Guided Workflow | Planned |
| DC-26 | Phase 19: Streaming Integration | Planned |
| DC-27 | Phase 19: Streaming Integration | Planned |
| DC-28 | Phase 19: Streaming Integration | Planned |
| DC-29 | Phase 19: Streaming Integration | Planned |
| DC-30 | Phase 19: Streaming Integration | Planned |

**Coverage:**
- Direct Carve requirements: 30 total
- Mapped to phases: 30
- Unmapped: 0

---
*Requirements defined: 2026-02-28*
