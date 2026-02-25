# Phase 2: Status Display - Research

**Researched:** 2026-02-24
**Domain:** Dear ImGui real-time CNC status display panel, GRBL status parsing
**Confidence:** HIGH

## Summary

Phase 2 builds a dedicated CNC Status Display panel showing real-time DRO (Digital Readout) position, machine state, feed rate, and spindle RPM. The existing codebase already has all the backend infrastructure: `CncController` polls GRBL status at 5Hz, parses `MachineStatus` structs (work/machine position, state, feed rate, spindle speed, overrides), and delivers updates to the main thread via `MainThreadQueue` callbacks. The `GCodePanel` already receives these callbacks — Phase 2 needs a new dedicated panel that also receives them.

The UI framework (Dear ImGui with docking) has an established panel pattern (`Panel` base class, `UIManager` ownership, `application_wiring.cpp` callback setup). The project has FontAwesome icons, a theme system with color constants, and toast notifications.

**Primary recommendation:** Create a `CncStatusPanel` that subscribes to `CncCallbacks::onStatusUpdate` and `onConnectionChanged`, stores a copy of the latest `MachineStatus`, and renders a CNCjs-inspired DRO with color-coded state indicators — all using existing ImGui primitives and the established panel pattern.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Dedicated dockable ImGui panel (like existing Library/Properties panels)
- Always visible — shows "Disconnected" when no machine connected
- CNC workspace mode: switching to CNC mode rearranges layout to show controller panels, hides library/materials panels
- This means the app needs a workspace mode switcher (Model mode vs CNC mode)
- CNCjs-inspired status display — modern, widget-based look
- Clean DRO with clear position readout
- Status indicators should feel polished, not raw text dumps

### Claude's Discretion
- Exact DRO digit size and font choice within ImGui constraints
- Work vs machine position toggle or dual display approach
- Feed/spindle display format (numeric, gauge, or both)
- State indicator implementation (badge, bar, background color)
- Workspace mode switching mechanism (menu, toolbar button, keyboard shortcut)

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CUI-01 | DRO displays work and machine XYZ position, updated at polling rate (5Hz) | `MachineStatus` already carries `workPos` and `machinePos` as `Vec3`. Callback fires at 5Hz. Panel stores latest status and renders each frame. |
| CUI-02 | DRO displays machine state (Idle, Run, Hold, Alarm, Home, etc.) with visual indicator | `MachineState` enum covers all states. Theme::Colors provides Success/Warning/Error for color coding. |
| CUI-03 | DRO displays real-time feed rate and spindle RPM | `MachineStatus::feedRate` and `MachineStatus::spindleSpeed` already parsed from GRBL status reports. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Dear ImGui | Docking branch (already in project) | All UI rendering | Already the project's UI framework |
| FontAwesome 6 | Already merged into ImGui font | Icons for state indicators, axes, etc. | Already available via `icons.h` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| GLM | Already in project | Vec3 for position data | Position math already uses Vec3 |
| Theme::Colors | In-project | Consistent color scheme | State indicator coloring |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ImGui default font for DRO | Custom monospace font loaded separately | More work, marginal benefit — ImGui's default font at larger size is sufficient for DRO |
| Raw ImGui::Text for DRO | ImGui::PushFont with scaled font | Large readable digits require font scaling, which ImGui supports natively |

## Architecture Patterns

### Recommended Project Structure
```
src/
├── ui/panels/cnc_status_panel.h     # CncStatusPanel : Panel
├── ui/panels/cnc_status_panel.cpp   # Render logic
├── managers/ui_manager.h            # Add CncStatusPanel ownership + accessor
├── managers/ui_manager.cpp          # Create CncStatusPanel in init()
├── app/application_wiring.cpp       # Wire CNC callbacks to new panel
```

### Pattern 1: Panel Registration (follow existing convention)
**What:** New panels are `unique_ptr` members of `UIManager`, created in `init()`, wired in `application_wiring.cpp`
**When to use:** Every new panel
**Example:**
```cpp
// ui_manager.h — add member + accessor
std::unique_ptr<CncStatusPanel> m_cncStatusPanel;
CncStatusPanel* cncStatusPanel() { return m_cncStatusPanel.get(); }
bool& showCncStatus() { return m_showCncStatus; }

// application_wiring.cpp — wire callbacks
if (auto* csp = m_uiManager->cncStatusPanel()) {
    // CNC callbacks already wired to GCodePanel; extend to also update CncStatusPanel
}
```

### Pattern 2: CNC Callback Fan-out
**What:** `CncCallbacks` is currently wired to only `GCodePanel`. Phase 2 needs the same callbacks to also reach `CncStatusPanel`. Two approaches:
1. **Wrapper callbacks** — In `application_wiring.cpp`, the callback lambdas call both panels
2. **EventBus** — Use the existing EventBus for decoupled dispatch

**Recommendation:** Wrapper callbacks (option 1). The EventBus exists but CNC callbacks carry structured data (`MachineStatus`), and wrapping is simpler than defining new event types. Just modify the existing callback lambdas to call both panels.
**Example:**
```cpp
cncCb.onStatusUpdate = [gcp, csp](const MachineStatus& status) {
    gcp->onGrblStatus(status);
    csp->onStatusUpdate(status);
};
cncCb.onConnectionChanged = [gcp, csp](bool connected, const std::string& version) {
    gcp->onGrblConnected(connected, version);
    csp->onConnectionChanged(connected, version);
};
```

### Pattern 3: Workspace Mode Switching
**What:** A mode enum in UIManager that controls which panels are visible/docked
**When to use:** Toggling between Model mode (library, properties, viewport) and CNC mode (DRO, G-code, viewport)
**Example:**
```cpp
enum class WorkspaceMode { Model, CNC };

void UIManager::setWorkspaceMode(WorkspaceMode mode) {
    if (mode == WorkspaceMode::CNC) {
        m_showLibrary = false;
        m_showProperties = false;
        m_showMaterials = false;
        m_showCncStatus = true;
        m_showGCode = true;
    } else {
        m_showLibrary = true;
        m_showProperties = true;
        m_showCncStatus = false;
    }
    m_workspaceMode = mode;
}
```

### Pattern 4: Large DRO Font Rendering in ImGui
**What:** ImGui supports `ImGui::PushFont()` with fonts loaded at different sizes. Load the same font at a larger pixel size (e.g., 32px or 48px) for DRO digits.
**When to use:** DRO position display requiring large readable numbers
**Example:**
```cpp
// During font atlas build (Application::rebuildFontAtlas):
ImFontConfig cfg;
cfg.MergeMode = false;
io.Fonts->AddFontFromFileTTF("path/to/monospace.ttf", 36.0f * scale, &cfg);
// Store as m_droFont

// During render:
ImGui::PushFont(m_droFont);
ImGui::Text("X %+10.3f", status.workPos.x);
ImGui::PopFont();
```

**Alternative (simpler):** Use `ImGui::SetWindowFontScale(2.0f)` to scale the default font. Less crisp but zero font loading work.

### Anti-Patterns to Avoid
- **Polling CncController directly from render():** The MachineStatus is already delivered via callbacks on the main thread. Don't call `m_cnc->lastStatus()` from the panel — use the cached copy received in the callback.
- **Blocking UI on serial reads:** All serial IO is on the IO thread. The panel must NEVER call any CncController method that could block.
- **Global state for connection status:** Use the callback-delivered state, not a global.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Color-coded state badge | Custom OpenGL drawing | ImGui::TextColored + ImGui::ColorButton | ImGui has built-in color rendering |
| Large number display | Custom glyph rendering | ImGui::PushFont or SetWindowFontScale | Font scaling is built into ImGui |
| Panel docking | Custom window management | ImGui docking branch (already used) | Already the project standard |
| State enum to string | Manual switch per callsite | Helper function (one switch in .cpp) | Single source of truth |
| State enum to color | Repeated color logic | Helper function returning ImVec4 | Consistent coloring across the panel |

## Common Pitfalls

### Pitfall 1: Callback Thread Safety Confusion
**What goes wrong:** Developer forgets that CncCallbacks fire on the main thread (via MainThreadQueue), then adds unnecessary locking in the panel.
**Why it happens:** The IO thread is where serial reads happen, but callbacks are dispatched via MTQ — they're already on the main thread when the panel sees them.
**How to avoid:** No mutex needed in CncStatusPanel. Just store the MachineStatus directly in the callback.
**Warning signs:** Seeing `std::mutex` in a panel class.

### Pitfall 2: DRO Flicker on Missing Status Updates
**What goes wrong:** Panel shows 0,0,0 or "Unknown" between status updates because it only renders data during the callback.
**Why it happens:** Callbacks fire at 5Hz but ImGui renders at 60Hz. Between callbacks, the panel must use the last known state.
**How to avoid:** Store `MachineStatus m_lastStatus` as a member. Update in callback, render from stored state every frame.
**Warning signs:** Position flickering to zero during motion.

### Pitfall 3: Font Atlas Rebuild Crash
**What goes wrong:** Adding a new font to the ImGui atlas without rebuilding the atlas texture causes a crash or invisible text.
**Why it happens:** ImGui requires `io.Fonts->Build()` and re-uploading the atlas texture to GPU after adding fonts. This can only happen during init, not mid-frame.
**How to avoid:** Add DRO font during `Application::rebuildFontAtlas()` where all other fonts are configured. Alternatively, use `SetWindowFontScale()` which needs no atlas changes.
**Warning signs:** Invisible text, assertion failures in ImGui, corrupted font rendering.

### Pitfall 4: Workspace Mode Losing Dock Layout
**What goes wrong:** Toggling workspace mode repeatedly causes ImGui to lose the saved dock layout for each mode.
**Why it happens:** ImGui persists one dock layout in imgui.ini. Switching modes by hiding/showing panels doesn't preserve their docked positions.
**How to avoid:** For Phase 2, keep workspace mode switching simple: toggle visibility booleans. Don't try to programmatically re-dock panels. Let ImGui's docking persist via the ini file. The user can arrange panels once and the layout sticks.
**Warning signs:** Panels floating after mode switch, layout reset on restart.

### Pitfall 5: Override Percent Display Desync
**What goes wrong:** Displaying override percentages from the GRBL status but using stale data because overrides are only reported when they change.
**Why it happens:** GRBL includes `Ov:100,100,100` in status reports, but may not include it in every report.
**How to avoid:** MachineStatus already stores `feedOverride`, `rapidOverride`, `spindleOverride` with defaults of 100. The parser updates these when present. Store the full status struct — fields persist across updates.
**Warning signs:** Override display stuck at 0 or flickering.

## Code Examples

### MachineState to Display String
```cpp
static const char* machineStateName(MachineState state) {
    switch (state) {
    case MachineState::Idle:    return "Idle";
    case MachineState::Run:     return "Run";
    case MachineState::Hold:    return "Hold";
    case MachineState::Jog:     return "Jog";
    case MachineState::Alarm:   return "Alarm";
    case MachineState::Door:    return "Door";
    case MachineState::Check:   return "Check";
    case MachineState::Home:    return "Homing";
    case MachineState::Sleep:   return "Sleep";
    case MachineState::Unknown: return "Unknown";
    default:                    return "???";
    }
}
```

### MachineState to Color
```cpp
static ImVec4 machineStateColor(MachineState state) {
    switch (state) {
    case MachineState::Idle:    return ImVec4(0.3f, 0.8f, 0.3f, 1.0f); // Green
    case MachineState::Run:     return ImVec4(0.3f, 0.5f, 1.0f, 1.0f); // Blue
    case MachineState::Hold:    return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
    case MachineState::Jog:     return ImVec4(0.3f, 0.7f, 1.0f, 1.0f); // Light blue
    case MachineState::Alarm:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
    case MachineState::Door:    return ImVec4(1.0f, 0.5f, 0.2f, 1.0f); // Orange
    case MachineState::Check:   return ImVec4(0.6f, 0.6f, 0.8f, 1.0f); // Lavender
    case MachineState::Home:    return ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Cyan
    case MachineState::Sleep:   return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    default:                    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    }
}
```

### CNCjs-Style DRO Layout
```cpp
void CncStatusPanel::renderDRO() {
    ImGui::PushFont(m_droFont); // or ImGui::SetWindowFontScale(2.0f)

    const char* axes[] = {"X", "Y", "Z"};
    const ImVec4 axisColors[] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f), // X = Red
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f), // Y = Green
        ImVec4(0.3f, 0.5f, 1.0f, 1.0f), // Z = Blue
    };
    float pos[3] = {m_status.workPos.x, m_status.workPos.y, m_status.workPos.z};

    for (int i = 0; i < 3; ++i) {
        ImGui::TextColored(axisColors[i], "%s", axes[i]);
        ImGui::SameLine();
        ImGui::Text("%+10.3f", pos[i]);
    }

    ImGui::PopFont(); // or ImGui::SetWindowFontScale(1.0f)
}
```

### Disconnected State
```cpp
void CncStatusPanel::render() {
    if (!m_open) return;
    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_connected) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s Disconnected", Icons::Unlink);
        ImGui::TextDisabled("Connect a CNC machine to see status");
        ImGui::End();
        return;
    }

    renderStateIndicator();
    renderDRO();
    renderFeedSpindle();
    ImGui::End();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single-panel CNC UI | Dedicated panels per function (status, jog, console) | Modern CNC UIs (CNCjs, UGS) | Better organization, dockable layout |
| Text-only DRO | Color-coded, large-font DRO with visual state indicators | CNCjs design pattern | Glanceable from shop distance |
| Tab-based mode switching | Workspace modes (model vs CNC) | IDE pattern (VS Code, CLion) | Fast context switch without losing layout |

## Open Questions

1. **DRO font loading approach**
   - What we know: ImGui supports loading fonts at multiple sizes. The project already loads FontAwesome via font merging in `rebuildFontAtlas()`.
   - What's unclear: Whether to add a large monospace font (crisp rendering) or use `SetWindowFontScale()` (simpler, slightly blurry).
   - Recommendation: Start with `SetWindowFontScale()` for simplicity. If the user wants crisper DRO text, a dedicated monospace font can be added later.

2. **Workspace mode persistence**
   - What we know: ImGui persists dock layout via imgui.ini. Panel visibility is saved via `ConfigManager`.
   - What's unclear: Whether two separate dock layouts should be maintained (one per mode) or just toggle visibility.
   - Recommendation: Toggle visibility only. Dock layout stays the same — user positions panels once, both modes share the same docking arrangement. This avoids the complexity of dual layout management.

## Sources

### Primary (HIGH confidence)
- Codebase analysis: `src/core/cnc/cnc_controller.h` — CncController API, MachineStatus struct, callback system
- Codebase analysis: `src/core/cnc/cnc_types.h` — MachineState enum, MachineStatus fields, CncCallbacks
- Codebase analysis: `src/ui/panels/panel.h` — Panel base class pattern
- Codebase analysis: `src/ui/panels/tool_browser_panel.h/.cpp` — Full panel implementation reference
- Codebase analysis: `src/managers/ui_manager.h` — Panel ownership and visibility management
- Codebase analysis: `src/app/application_wiring.cpp` — CNC callback wiring pattern, panel registration
- Codebase analysis: `src/ui/theme.h` — Theme::Colors for consistent coloring
- Codebase analysis: `src/ui/icons.h` — FontAwesome icon constants

### Secondary (MEDIUM confidence)
- Dear ImGui documentation — PushFont, SetWindowFontScale, docking branch behavior
- CNCjs project — Visual reference for DRO layout and state indicators

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — everything is already in the project
- Architecture: HIGH — follows established panel pattern exactly
- Pitfalls: HIGH — based on actual codebase analysis of threading model and callback system

**Research date:** 2026-02-24
**Valid until:** 2026-03-24 (stable codebase, internal patterns)
