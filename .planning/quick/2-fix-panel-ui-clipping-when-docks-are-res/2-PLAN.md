---
phase: quick-2
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - src/ui/panels/cut_optimizer_panel.cpp
  - src/ui/panels/gcode_panel.cpp
  - src/ui/panels/properties_panel.cpp
  - src/ui/panels/library_panel.cpp
  - src/ui/panels/project_panel.cpp
  - src/ui/panels/viewport_panel.cpp
autonomous: true
must_haves:
  truths:
    - "All panels remain fully usable when docks are resized to small widths (200-300px)"
    - "No buttons, inputs, or text are clipped or pushed off-screen when docks shrink"
    - "Split-pane panels (Cut Optimizer, G-code) degrade gracefully to stacked layout at narrow widths"
    - "Toolbar items in all panels wrap or remain accessible at narrow widths"
  artifacts:
    - path: "src/ui/panels/cut_optimizer_panel.cpp"
      provides: "Responsive split-pane layout with min-width clamping"
    - path: "src/ui/panels/gcode_panel.cpp"
      provides: "Responsive split-pane layout with min-width clamping"
    - path: "src/ui/panels/properties_panel.cpp"
      provides: "Vertically stacked transform controls instead of SameLine clipping"
    - path: "src/ui/panels/library_panel.cpp"
      provides: "Responsive toolbar search width"
  key_links:
    - from: "All panel render() methods"
      to: "ImGui::GetContentRegionAvail()"
      via: "Available width queries before layout decisions"
      pattern: "GetContentRegionAvail"
---

<objective>
Fix UI clipping in all dock panels when ImGui docks are resized smaller than content.

Purpose: Panels currently use hardcoded pixel widths for split-pane layouts and SameLine() toolbar patterns that cause buttons, inputs, and text to be clipped or pushed off-screen when docks shrink below ~300px. This makes the application unusable with narrow dock configurations.

Output: All panels gracefully handle narrow dock widths without clipping UI elements.
</objective>

<execution_context>
@/home/matthew/.claude/get-shit-done/workflows/execute-plan.md
@/home/matthew/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@src/ui/panels/cut_optimizer_panel.cpp
@src/ui/panels/gcode_panel.cpp
@src/ui/panels/properties_panel.cpp
@src/ui/panels/library_panel.cpp
@src/ui/panels/project_panel.cpp
@src/ui/panels/viewport_panel.cpp
@src/ui/panels/start_page.cpp
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix split-pane panels (Cut Optimizer + G-code) to handle narrow docks</name>
  <files>src/ui/panels/cut_optimizer_panel.cpp, src/ui/panels/gcode_panel.cpp</files>
  <action>
Both panels use hardcoded pixel widths for side-by-side BeginChild layouts that break at narrow dock widths.

**cut_optimizer_panel.cpp** (render method, ~line 26-48):
- Currently: `float configWidth = 300.0f;` hardcoded, right child gets `availWidth - configWidth - 8` which goes negative when dock < 308px.
- Fix: Query `ImGui::GetContentRegionAvail().x` and if `availWidth < 500.0f`, switch to vertical stacked layout (Config child on top with fixed height ~300px, Results child below with remaining height). When `availWidth >= 500.0f`, keep the current side-by-side layout but clamp `configWidth` to `std::min(300.0f, availWidth * 0.45f)` so the right panel always gets reasonable space.
- Also fix the toolbar (renderToolbar ~line 60-84): The toolbar uses many SameLine() calls for "Optimize", "Clear All", separator, Algorithm combo, and "Allow Rotation" checkbox. Wrap the Algorithm combo and checkbox to a new line when `availWidth < 400.0f` -- check `ImGui::GetContentRegionAvail().x` after the separator SameLine, and if insufficient, skip the SameLine and let them flow to the next line.
- Also fix renderPartsEditor (~line 86-152): The parts list uses hardcoded `SameLine(150)`, `SameLine(230)`, `SameLine(260)` positions. Replace with proportional positions: `SameLine(availWidth * 0.45f)`, `SameLine(availWidth * 0.7f)`, `SameLine(availWidth * 0.82f)`. Get `availWidth` from `ImGui::GetContentRegionAvail().x` at the start of the loop. If `availWidth < 200`, skip SameLine positioning entirely and stack the dimension/quantity/remove button vertically.
- Fix renderSheetConfig (~line 154-197): The Width/Height inputs use SameLine + "mm" text, and Kerf/Margin use SameLine. These should be fine since InputFloat is narrow, but add a width check: if `availWidth < 250.0f`, drop the SameLine between Kerf and Margin so they stack. Same for the preset buttons -- if `availWidth < 300.0f`, skip SameLine between the 3 preset buttons.

**gcode_panel.cpp** (render method, ~line 13-45):
- Currently: `float statsWidth = 250.0f;` hardcoded, same negative-width issue as Cut Optimizer.
- Fix: If `availWidth < 420.0f`, switch to vertical stacked layout (Stats child on top with height ~250px, PathView child below). When `availWidth >= 420.0f`, clamp `statsWidth` to `std::min(250.0f, availWidth * 0.4f)`.
- Fix renderToolbar (~line 83-109): When gcode is loaded, toolbar has Open, Close, separator, Travel checkbox, Extrusion checkbox all on SameLine. If `availWidth < 350.0f`, put the checkboxes on a new line (skip the SameLine before the first checkbox).
  </action>
  <verify>
Build: `cd /data/DW && cmake --build build --target DigitalWorkshop 2>&1 | tail -5` compiles with no errors.
Visual: Launch app, open Cut Optimizer and G-code panels, resize their docks from wide to very narrow (~200px). Verify no elements are clipped and layout transitions smoothly from side-by-side to stacked.
  </verify>
  <done>Cut Optimizer and G-code panels render all UI elements without clipping at dock widths from 200px to full screen. Split layouts transition to stacked when dock is narrow.</done>
</task>

<task type="auto">
  <name>Task 2: Fix Properties panel transform controls and Library/Project/Viewport toolbar clipping</name>
  <files>src/ui/panels/properties_panel.cpp, src/ui/panels/library_panel.cpp, src/ui/panels/project_panel.cpp, src/ui/panels/viewport_panel.cpp</files>
  <action>
**properties_panel.cpp** (renderTransformInfo ~line 184-296):
- The Translate, Rotate, and Scale sections each use `ImGui::DragFloat3("label", values)` followed by `ImGui::SameLine()` and an "Apply##..." button. DragFloat3 takes the full available width by default, so SameLine pushes the Apply button off-screen.
- Fix: Remove the `ImGui::SameLine()` before each Apply button. Instead, place the Apply button on its own line below the DragFloat3, using `ImVec2(-1, 0)` (full width) to make it a clear action button, consistent with the "Center on Origin" and "Normalize Size" buttons above which already use `ImVec2(-1, 0)`. This eliminates the clipping entirely.
- Specifically, at lines 249-259 (Translate), lines 262-281 (Rotate), and lines 284-293 (Scale): remove the `ImGui::SameLine();` line before each `ImGui::Button("Apply##...")` call and change the button to use `ImVec2(-1, 0)` for full width.

**library_panel.cpp** (renderToolbar ~line 137-174):
- `ImGui::SetNextItemWidth(-100.0f)` reserves 100px for the two toolbar buttons. This is fine at reasonable widths but at very narrow widths (<120px), the search input gets negative width.
- Fix: Calculate button space dynamically. Get `float avail = ImGui::GetContentRegionAvail().x;`. If `avail < 150.0f`, set search width to `avail * 0.5f` instead of `-100.0f`. Otherwise keep `-100.0f`. This ensures the search input never goes to zero/negative.

**project_panel.cpp** (renderNoProject ~line 119-145 and renderProjectInfo ~line 29-71):
- "New Project" and "Open Project" buttons use `SameLine()`. At very narrow widths, "Open Project" clips.
- Fix: In renderNoProject, check `ImGui::GetContentRegionAvail().x` after the first button. If remaining width is less than the expected button width (~100px), skip the `SameLine()` and let "Open Project" wrap to next line. Same for the "Save" / "Close" buttons in renderProjectInfo (lines 57-68).
- Pattern: After the first `ImGui::Button(...)`, check `if (ImGui::GetContentRegionAvail().x > 100.0f) { ImGui::SameLine(); }` before the second button.

**viewport_panel.cpp** (renderToolbar ~line 303-326):
- Toolbar has Reset, Fit, separator, Wireframe checkbox on SameLine. At narrow viewport widths this is less likely to be a problem since the viewport takes center space, but for consistency:
- Fix: After the separator SameLine, check available width. If `ImGui::GetContentRegionAvail().x < 100.0f`, skip the last SameLine and let Wireframe wrap to the next line.
  </action>
  <verify>
Build: `cd /data/DW && cmake --build build --target DigitalWorkshop 2>&1 | tail -5` compiles with no errors.
Visual: Launch app, resize Properties dock to ~200px width. Verify Translate/Rotate/Scale Apply buttons are fully visible (now on their own line). Resize Library dock narrow -- search bar and buttons remain visible. Resize Project dock narrow -- Save/Close buttons wrap rather than clip.
  </verify>
  <done>Properties panel transform Apply buttons are always visible (stacked below DragFloat3). Library search bar never gets negative width. Project and Viewport toolbar buttons wrap at narrow widths instead of clipping.</done>
</task>

</tasks>

<verification>
1. Build succeeds with no warnings in modified files
2. Launch application and resize each dock panel to ~200px width:
   - Cut Optimizer: transitions to stacked layout, all controls accessible
   - G-code Viewer: transitions to stacked layout, all controls accessible
   - Properties: Apply buttons visible below DragFloat3 inputs
   - Library: search bar and toolbar buttons visible
   - Project: Save/Close and New/Open buttons wrap at narrow widths
   - Viewport: toolbar items remain accessible
3. Resize docks back to normal width -- all layouts return to expected appearance
4. No visual regressions at normal (wide) dock sizes
</verification>

<success_criteria>
- All 6 panel files modified to handle narrow dock widths gracefully
- No UI elements are clipped or pushed off-screen at dock widths down to ~200px
- Split-pane panels switch from side-by-side to stacked layout at narrow widths
- Toolbar buttons and inputs remain accessible at all dock sizes
- No behavioral changes to panel functionality -- only layout responsiveness improved
</success_criteria>

<output>
After completion, create `.planning/quick/2-fix-panel-ui-clipping-when-docks-are-res/2-SUMMARY.md`
</output>
