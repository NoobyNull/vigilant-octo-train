# Plan 01-06: Human Verification Checkpoint

**Objective:** Verify the complete end-to-end materials system is working correctly.

**Status:** Awaiting your confirmation. Follow the steps below and report back with "approved" if all checks pass, or describe any issues encountered.

---

## Pre-Verification Checklist

- [ ] Build is fresh: `cd /data/DW && cmake --build build --target digital_workshop 2>&1 | tail -5` shows success
- [ ] 492 tests passing: `cd /data/DW && ctest -j1 2>&1 | grep -E "passed|failed"`

## Verification Steps

Follow these steps in order. Each step should succeed before moving to the next.

### Step 1: Launch the Application
```bash
cd /data/DW && ./build/digital_workshop
```
**Expected:** Application window opens without errors.

### Step 2: Verify Materials Panel in View Menu
- Look for "View" menu at top of application
- Check if "Materials" option is available
- Click "Materials" to open the Materials panel

**Expected:** Materials panel appears as a dockable panel (can be resized, moved, closed)

### Step 3: Verify 32 Default Materials Loaded
- In Materials panel, verify you see materials listed with thumbnail grid
- Check category tabs at top: "All", "Hardwood", "Softwood", "Domestic", "Composite"
- Click each tab to verify filtering works

**Expected:**
- ~32 materials visible in "All" tab
- Each category has appropriate materials (e.g., Hardwood shows oak, maple, walnut, cherry, etc.)
- Thumbnails display (solid colors or textures)

### Step 4: Import a 3D Model
- File menu → "Import" or similar option
- Select an STL file (or any 3D model)
- Load it into the library

**Expected:** Model appears in library panel (left side)

### Step 5: Load Model into Viewport
- Double-click the imported model in the library
- Or use viewport selection to display the model

**Expected:** 3D model visible in the viewport (center of screen)

### Step 6: Assign a Material
- In Materials panel, find a material (e.g., "Red Oak" or any hardwood)
- Double-click the material to assign it to the loaded model

**Expected:**
- Model in viewport updates with material texture (or solid color if no texture in default materials)
- PropertiesPanel (right side) shows material info:
  - Material name at top
  - Category badge (Hardwood/Softwood/Domestic/Composite)
  - Read-only info: Janka Hardness, Feed Rate, Spindle Speed, Depth of Cut, Cost
  - Editable grain direction slider (0-360 degrees)
  - "Remove Material" button

### Step 7: Verify Material Display in PropertiesPanel
- Check all material properties are displayed correctly
- Format check:
  - Janka: "1,290 lbf" (with comma and units)
  - Feed: "80 in/min"
  - Speed: "18,000 RPM"
  - Depth: "0.125 in"
  - Cost: "$4.50/bf"

**Expected:** Material info displays with proper formatting

### Step 8: Adjust Grain Direction
- In PropertiesPanel, find grain direction slider
- Move the slider from 0 to 360 degrees
- Watch the 3D model in viewport

**Expected:**
- Texture orientation rotates smoothly on the 3D model
- Grain pattern (if visible) rotates as you move the slider
- No visual artifacts or lag

### Step 9: Remove Material
- Click "Remove Material" button in PropertiesPanel
- Check the viewport and PropertiesPanel

**Expected:**
- Material info section disappears from PropertiesPanel
- Model reverts to default solid color (or previous state)
- Color picker re-appears as fallback (bottom of PropertiesPanel)

### Step 10: Persist Across App Restart
- Assign a material again (any material)
- Close the application: File → Exit or close window
- Reopen the application: `./build/digital_workshop`
- Load the same model

**Expected:**
- Previously assigned material is still assigned to the model
- PropertiesPanel shows the same material info
- Grain direction slider is at the same position (if you changed it)

### Step 11: (Optional) Import a Custom Material Archive
If you have a `.dwmat` file available:
- In Materials panel, look for "Import" button (toolbar)
- Select a `.dwmat` file
- Verify the material appears in the appropriate category

**Expected:**
- Material imported successfully
- Appears in Materials panel with correct category
- Can be assigned to objects like built-in materials

---

## Verification Summary

After completing all steps, please respond with **ONE** of the following:

### ✅ APPROVED
If all steps passed and the materials system is working correctly, respond with:
```
approved
```

This will mark Plan 01-06 as complete and advance to Phase 02.

### ❌ ISSUES FOUND
If any step failed or behaved unexpectedly, describe the issue:
```
Issue in Step [#]: [Description of what went wrong]
  - Expected: [what should have happened]
  - Actual: [what actually happened]
  - Screenshot/Output: [if available]
```

Multiple issues can be reported together. Each will be triaged and fixed.

---

## Quick Reference

**Key UI Elements:**
- **View Menu:** Top menu bar, contains "Materials" toggle
- **Materials Panel:** Dockable panel, shows grid of materials with category tabs
- **PropertiesPanel:** Right side panel, shows selected object properties + material info
- **ViewportPanel:** Center panel, shows 3D object with assigned material texture
- **LibraryPanel:** Left side panel, shows imported models

**Expected Behavior Summary:**
- 32 built-in materials appear on first launch
- Materials can be assigned by double-clicking in Materials panel
- PropertiesPanel shows material info only when material assigned
- Grain direction slider rotates texture on 3D object
- Material assignments persist across app restarts
- Color picker is fallback for unassigned objects

---

*Waiting for your verification. Please run through the steps above and report back.*
