# Phase 1: Add Materials Mapping to Object View with Coordinated Materials Database - Context

**Gathered:** 2026-02-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Add a materials system that lets a CNC woodworker assign wood species to objects in their 3D model. This includes: a global materials database backed by archive files (.dwmat), a materials panel for browsing/editing species, texture mapping onto 3D objects, and import/export of material archives. The material replaces the existing color selector as the primary surface appearance mechanism.

</domain>

<decisions>
## Implementation Decisions

### Materials database
- Support wood species and composite woods (MDF, plywood, etc.)
- Categories: hardwood, softwood, domestic, composite — with specific species under each
- Composite covers: MDF, HDF, plywood, non-ferrous metals, plastics, foams
- No "exotic" category
- User-editable with built-in defaults shipped with the app
- Storage: hybrid — SQLite entry in existing database pointing to a .dwmat material archive file
- Global library shared across all projects (not per-project)

### Material archive format (.dwmat)
- ZIP-based archive with `.dwmat` extension
- Contains: texture PNG (tileable and/or photo), technical specifications (JSON or XML), metadata
- User provides the texture images — app reads them from the archive
- Import and export of .dwmat archives required (share materials between users/machines)

### Object view integration
- Material replaces the existing color selector as an overlay image on the object
- Wood species database gets its own dedicated panel (not inline in another panel)
- All fields in the material record must be editable in the panel
- Objects with no material assigned: blank/hidden (no material info shown)
- One model loaded at a time — material assignment is per-object, one at a time

### 3D visualization
- Texture mapped: apply the wood species PNG texture to the 3D object surface
- Grain direction: auto-detected from texture orientation, but user can modify/override
- Grain direction affects texture mapping on the object

### Material properties
- Janka hardness rating
- Feed and speed recommendations (CNC-specific: feed rate, spindle speed, depth of cut)
- Cost per board-foot (for project cost estimation)
- Grain direction (auto + user-modifiable)
- Texture image(s) from the .dwmat archive

### Claude's Discretion
- Database schema design for the SQLite materials table
- Default built-in species list (common North American hardwoods/softwoods)
- Panel layout and widget arrangement for the materials browser
- Texture mapping UV approach (how grain direction maps to object geometry)
- Archive internal structure (file naming within the .dwmat ZIP)

</decisions>

<specifics>
## Specific Ideas

- Material replaces the color selector — it's not an addition alongside color, it IS the surface appearance
- The .dwmat archive is the portable unit — everything about a material is self-contained in one file
- User will provide their own tileable textures in the archives
- Categories (hardwood/softwood/domestic/composite) are organizational groupings, not just metadata tags

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-add-materials-mapping-to-object-view-with-coordinated-materials-database*
*Context gathered: 2026-02-09*
