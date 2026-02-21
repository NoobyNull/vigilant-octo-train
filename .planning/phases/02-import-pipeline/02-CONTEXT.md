# Phase 2: Import Pipeline - Context

**Gathered:** 2026-02-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Overhaul the model import pipeline: add parallel worker processing, G-code import with toolpath visualization and hierarchical organization, non-blocking import UX, file handling options (copy/move/leave-in-place), and polish existing STL/OBJ/3MF import robustness. This phase covers the complete import lifecycle from file selection through library storage and organization.

</domain>

<decisions>
## Implementation Decisions

### Pipeline Architecture
- Switch from sequential per-file processing to **parallel workers** using a thread pool
- Three parallelism tiers (user-configurable in Settings):
  - **Auto** (default): 60% of CPU cores
  - **Fixed/Performance**: capped at 90% of cores
  - **Expert**: 100% of cores (user accepts UI responsiveness risk)
- Each worker runs the full pipeline independently (read → hash → dedup → parse → insert)

### File Handling After Import
- Three modes: **copy into library**, **move to library**, or **leave in place**
- Configured as a **global setting** in the Settings app — applies to all imports silently
- Default: leave in place (current behavior)
- When copy/move is selected, files go to a managed library directory

### G-code Support
- Add G-code as a new importable format alongside STL/OBJ/3MF
- Target dialects: **Grbl, grblHAL, FluidNC** (all Grbl-compatible core)
- G-code imports produce **toolpath visualization** rendered as 3D lines in the viewport
- G-code can exist **standalone** in the library (association with a model is optional)
- Basic metadata extracted on import: bounding box, total travel distance, estimated run time, feed rates used, tool numbers referenced

### G-code Hierarchy & Organization
- Models can have associated G-code files organized in a hierarchy: **Model → Operation Groups → ordered G-code files**
- **Templates** are opt-in — user can apply a template (e.g., CNC Router Basic) or skip and create groups manually
- **CNC Router Basic template**: Roughing, Finishing, Profiling, Drilling groups
- **Custom/Minimal**: start with no groups, user creates their own
- User picks one template or no template — no automatic mapping
- G-code imported standalone goes to a general G-code area; user can drag-and-drop to associate with models and organize into groups later
- Auto-detect attempts to match G-code to models by filename

### Duplicate Handling
- Duplicates are **skipped automatically** (current behavior preserved)
- **Summary shown at end** of batch listing all skipped duplicates with names

### Error Handling
- Errors do not interrupt the batch — other files continue importing
- **Summary at end** lists all errors with file names and failure reasons
- **Toast notifications** for individual errors, configurable on/off in settings

### Import UX
- Import runs in **background** with a **status bar indicator** — non-blocking, user can keep working
- Replace current modal overlay with background processing
- After batch completes, **viewport focus does not change** — stay on whatever user was viewing
- Tags assigned **after import only**, not during

### Format Polish
- Improve robustness of existing STL/OBJ/3MF loaders (error handling, edge cases)
- No new mesh formats this phase (STEP/DXF/SVG deferred)

### Metadata
- Mesh files: capture what's naturally available (current fields sufficient)
- G-code files: basic stats (bounding box, travel distance, estimated time, feed rates, tool numbers)
- Richer metadata (material, stock info, source tracking) deferred to export system phase

### Claude's Discretion
- Managed library directory structure and naming
- G-code parser implementation approach
- Toolpath rendering style (colors, line widths, rapid vs cutting move differentiation)
- Status bar indicator design
- Toast notification styling and duration
- Thread pool implementation details
- Database schema extensions for G-code and hierarchy

</decisions>

<specifics>
## Specific Ideas

- Parallelism tiers named Auto/Fixed/Expert with specific core percentage caps (60/90/100)
- CNC Router Basic template groups: Roughing, Finishing, Profiling, Drilling
- G-code dialects are all Grbl-family — shared core parsing with dialect-specific extensions
- File handling is a "set and forget" global setting, not per-import

</specifics>

<deferred>
## Deferred Ideas

- STEP/IGES import — CAD solid model support, requires tessellation (future format phase)
- DXF/SVG import — 2D vector formats for cutting paths (future format phase)
- Export system — when built, will need material/stock info, source tracking, attribution metadata
- Full G-code analysis — rapid vs cutting breakdown, depth of cuts, spindle speeds, axis limits (future enhancement)
- Tag UI during import — tags exist in schema, UI deferred to library management phase

</deferred>

---

*Phase: 02-import-pipeline*
*Context gathered: 2026-02-09*
