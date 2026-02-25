# Phase 4: Tool Integration - Research

**Researched:** 2026-02-24
**Domain:** CNC tool/material selection and feeds/speeds calculation UI integration
**Confidence:** HIGH

## Summary

Phase 4 connects three existing subsystems -- ToolDatabase (Vectric .vtdb), MaterialManager (Janka-based hardness), and ToolCalculator (feeds/speeds engine) -- into a new CNC workspace panel. All three subsystems are fully implemented and tested. The work is primarily UI integration: creating a new `CncToolPanel` that provides tool selection, material selection, and auto-calculated parameter display within the CNC workspace mode.

The existing `ToolBrowserPanel` already demonstrates the full pattern (tree view, material combo, calculator invocation, result display) but lives in the Model workspace as a full CRUD browser. Phase 4 needs a streamlined CNC-context panel that focuses on selection + calculation rather than editing.

**Primary recommendation:** Create a new `CncToolPanel` that reuses ToolDatabase and MaterialManager for selection, auto-invokes ToolCalculator when both tool and material are selected, and displays results in a compact reference format. Wire it into CNC workspace mode alongside existing panels.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- User selects tool geometry from existing ToolDatabase (Vectric .vtdb compatible)
- User selects wood species from existing MaterialManager (Janka-based hardness)
- Existing ToolCalculator produces RPM, feed rate, plunge rate, stepdown, stepover
- Results displayed in a reference panel -- operator reads these while setting up or running a job
- NOT a tool change system -- no M6 interception, no tool swap prompts
- NOT automatic -- operator manually reads the calculated values and applies them
- The calculated parameters are advisory/reference only
- New panel or section within the CNC workspace mode (established in Phase 2)
- Should be visible alongside the DRO and jog controls
- Tool and material selection should persist across sessions (SQLite)

### Claude's Discretion
- Whether tool/material selection is its own panel or integrated into CncStatusPanel
- UI layout for the calculated parameters display
- How to present the tool database browser in the CNC context (reuse ToolBrowserPanel or simplified selector)
- Whether to show a compact material dropdown or full material browser

### Deferred Ideas (OUT OF SCOPE)
- M6 tool change interception -- v2 requirement (TCH-01, TCH-02)
- Feed rate deviation warning -- Phase 5 (TAC-04)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| TAC-01 | User can select a tool geometry from the tool database to calculate cutting parameters | ToolDatabase.findAllGeometries(), getAllTreeEntries() provide full tool listing; VtdbToolGeometry has all fields needed for CalcInput |
| TAC-02 | Selected tool geometry + selected wood species auto-calculates optimal feeds/speeds via existing calculator | ToolCalculator::calculate() takes CalcInput (diameter, flutes, type, units, janka, material_name, machine params) and returns CalcResult with all parameters |
| TAC-03 | Calculated cutting parameters (RPM, feed rate, plunge rate, stepdown, stepover) displayed in a reference panel | CalcResult struct contains rpm, feed_rate, plunge_rate, stepdown, stepover, chip_load, power_required, power_limited -- all ready for display |
</phase_requirements>

## Standard Stack

### Core (All Existing)
| Component | Location | Purpose | Status |
|-----------|----------|---------|--------|
| ToolDatabase | src/core/database/tool_database.h/.cpp | Vectric .vtdb compatible tool storage | Complete, tested |
| MaterialManager | src/core/materials/material_manager.h/.cpp | Material CRUD with 31+ built-in species | Complete, tested |
| ToolCalculator | src/core/cnc/tool_calculator.h/.cpp | Pure calculation engine, no DB deps | Complete, tested |
| Panel base class | src/ui/panels/panel.h | ImGui panel with open/close state | Established pattern |
| UIManager | src/managers/ui_manager.h/.cpp | Panel ownership, visibility, workspace mode | Complete |
| Application wiring | src/app/application_wiring.cpp | Dependency injection for panels | Established pattern |

### Key Data Types
| Type | Header | Key Fields |
|------|--------|------------|
| VtdbToolGeometry | cnc_tool.h | id, tool_type, units, diameter, num_flutes, flute_length |
| VtdbTreeEntry | cnc_tool.h | id, parent_group_id, tool_geometry_id, name |
| VtdbMachine | cnc_tool.h | id, name, spindle_power_watts, max_rpm, drive_type |
| MaterialRecord | material.h | id, name, category, jankaHardness |
| CalcInput | tool_calculator.h | diameter, num_flutes, tool_type, units, janka_hardness, material_name, spindle_power_watts, max_rpm, drive_type |
| CalcResult | tool_calculator.h | rpm, feed_rate, plunge_rate, stepdown, stepover, chip_load, power_required, power_limited, hardness_band, rigidity_factor |

## Architecture Patterns

### Existing Panel Pattern
All CNC panels follow this pattern:
1. Class inherits from `Panel` with title in constructor
2. `render()` method checks `m_open`, calls `ImGui::Begin()`/`End()`
3. Dependencies injected via setters (e.g., `setCncController()`, `setToolDatabase()`)
4. UIManager owns panel as `std::unique_ptr`, renders in `renderPanels()` when visibility bool is true
5. Application wiring injects dependencies in `application_wiring.cpp`

### CNC Workspace Mode Pattern
`UIManager::setWorkspaceMode(WorkspaceMode::CNC)` controls panel visibility:
- Currently shows: CncStatus, CncJog, CncConsole, CncWcs, GCode
- Hides: Library, Properties, Materials, ToolBrowser, CostEstimator, CutOptimizer
- New CncToolPanel needs to be added to CNC mode visibility set

### Calculator Invocation Pattern (from ToolBrowserPanel)
```cpp
CalcInput input;
input.diameter = geometry.diameter;
input.num_flutes = geometry.num_flutes;
input.tool_type = geometry.tool_type;
input.units = geometry.units;
input.janka_hardness = material.jankaHardness;
input.material_name = material.name;
input.spindle_power_watts = machine.spindle_power_watts;
input.max_rpm = machine.max_rpm;
input.drive_type = machine.drive_type;
CalcResult result = ToolCalculator::calculate(input);
```

### Persistence Pattern
Selection persistence uses the main SQLite database. Existing config patterns use ConfigManager or direct DB queries. Tool and material selection IDs should persist similarly. Options:
- Store in a simple `cnc_settings` table (key-value) in the main database
- Or use existing config infrastructure

### Recommended Project Structure
```
src/ui/panels/
  cnc_tool_panel.h       # New panel header
  cnc_tool_panel.cpp     # New panel implementation
src/managers/
  ui_manager.h           # Add forward decl, unique_ptr, visibility bool, accessor
  ui_manager.cpp         # Create panel, render, workspace mode
src/app/
  application_wiring.cpp # Wire ToolDatabase + MaterialManager into new panel
```

### Anti-Patterns to Avoid
- **Duplicating calculator logic:** ToolCalculator is the single source of truth -- never hand-code SFM/chip load values
- **Direct DB access from panel:** Use ToolDatabase and MaterialManager APIs, not raw SQL
- **Blocking UI thread:** All existing DB operations are fast (local SQLite), but avoid adding network calls

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Feeds/speeds math | Custom RPM/feed formulas | ToolCalculator::calculate() | Already tested, handles all material bands and tool types |
| Material hardness classification | Manual if/else chains | ToolCalculator::classifyMaterial() | Handles wood, metal, plastic, composite |
| Tool tree browsing | Custom tree data structure | ToolDatabase::getAllTreeEntries() + recursive render | Already implemented in ToolBrowserPanel |
| Material listing | Raw SQL query | MaterialManager::getAllMaterials() | Handles bundled + imported, hidden filtering |

## Common Pitfalls

### Pitfall 1: VtdbMaterial vs MaterialRecord Confusion
**What goes wrong:** ToolDatabase has VtdbMaterial (id, name) and MaterialManager has MaterialRecord (id, name, jankaHardness, etc.). The calculator needs Janka hardness which only exists in MaterialRecord.
**Why it happens:** Two material systems coexist -- Vectric .vtdb materials (for cutting data lookup) and DW materials (for Janka-based calculation).
**How to avoid:** For TAC-02 calculator, use MaterialManager::getAllMaterials() to get MaterialRecord with jankaHardness. The Vectric materials are for stored cutting data, not for calculation.

### Pitfall 2: Machine Not Selected
**What goes wrong:** If no machine is configured in ToolDatabase, calculator uses defaults (24000 RPM, belt drive, 0W spindle).
**Why it happens:** Machines are stored in the .vtdb file and may not be configured yet.
**How to avoid:** Show default machine parameters when none selected, or prompt user to configure machine. CalcInput has sensible defaults that still produce valid results.

### Pitfall 3: Auto-Calculate Timing
**What goes wrong:** Recalculating on every frame or every combo change causes visual flicker.
**Why it happens:** ImGui is immediate mode -- combos fire on every selection change.
**How to avoid:** Recalculate only when both tool AND material are selected, and only when either selection changes (track previous IDs).

### Pitfall 4: Workspace Mode Doesn't Show New Panel
**What goes wrong:** New panel doesn't appear in CNC mode.
**Why it happens:** `setWorkspaceMode(WorkspaceMode::CNC)` has a hardcoded list of panels to show/hide.
**How to avoid:** Add `m_showCncTool = true` to the CNC mode block in setWorkspaceMode().

## Code Examples

### Panel Registration Pattern (from ui_manager.cpp)
```cpp
// In UIManager::init():
m_cncToolPanel = std::make_unique<CncToolPanel>();

// In UIManager::renderPanels():
if (m_showCncTool && m_cncToolPanel) {
    m_cncToolPanel->render();
    if (!m_cncToolPanel->isOpen()) {
        m_showCncTool = false;
        m_cncToolPanel->setOpen(true);
    }
}

// In UIManager::setWorkspaceMode():
if (mode == WorkspaceMode::CNC) {
    m_showCncTool = true;
    // ... existing CNC panels
}
```

### Compact Material Dropdown (from ToolBrowserPanel)
```cpp
auto allMats = m_materialManager->getAllMaterials();
if (ImGui::BeginCombo("Material", m_selectedMaterialName.c_str())) {
    for (const auto& mat : allMats) {
        if (ImGui::Selectable(mat.name.c_str(), mat.id == m_selectedMaterialId)) {
            m_selectedMaterialId = mat.id;
            m_selectedMaterialName = mat.name;
            m_selectedJanka = mat.jankaHardness;
            recalculate(); // auto-calc on selection change
        }
    }
    ImGui::EndCombo();
}
```

### Application Wiring Pattern
```cpp
// In application_wiring.cpp:
if (auto* ctp = m_uiManager->cncToolPanel()) {
    ctp->setToolDatabase(m_toolDatabase.get());
    ctp->setMaterialManager(m_materialManager.get());
}
```

## Design Recommendation: Standalone CncToolPanel

**Recommendation:** Create a standalone `CncToolPanel` rather than integrating into CncStatusPanel.

**Rationale:**
1. CncStatusPanel is focused on real-time machine data (DRO, state, feed/spindle) -- adding tool selection dilutes its purpose
2. A separate panel can be resized/repositioned independently in the dock layout
3. Follows the existing pattern where each CNC panel has a single responsibility (status, jog, console, WCS)
4. The panel will be visible alongside the DRO and jog controls in the dock layout

**Panel content:**
- Compact tool selector (combo or small tree, not full CRUD browser)
- Material dropdown from MaterialManager
- Auto-calculated results display (RPM, feed, plunge, stepdown, stepover)
- Machine info line showing current machine parameters
- Classification and power info

## Selection Persistence Recommendation

**Recommendation:** Store selected tool geometry ID, material ID, and machine ID in a `cnc_session` table in the main SQLite database.

```sql
CREATE TABLE IF NOT EXISTS cnc_session (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
```

Keys: `selected_tool_geometry_id`, `selected_material_id`, `selected_machine_id`.

This is simpler than extending ConfigManager and keeps CNC state separate from app config.

## Open Questions

1. **Compact tree vs flat combo for tool selection?**
   - ToolBrowserPanel uses a full recursive tree -- appropriate for CRUD browsing
   - For CNC context, a combo box with flat list may be simpler
   - Recommendation: Start with a flat combo sorted by name; tool count is typically small (<100)

## Sources

### Primary (HIGH confidence)
- Direct codebase analysis of all referenced source files
- Existing ToolBrowserPanel implementation (demonstrates full integration pattern)
- Existing CNC panel implementations (CncStatusPanel, CncJogPanel, CncConsolePanel, CncWcsPanel)
- UIManager workspace mode implementation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - all components exist and are tested
- Architecture: HIGH - follows established patterns from existing CNC panels
- Pitfalls: HIGH - identified from actual code analysis, not speculation

**Research date:** 2026-02-24
**Valid until:** 2026-03-24 (stable internal codebase)
