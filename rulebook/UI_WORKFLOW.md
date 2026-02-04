# UI Design Workflow

## Visual Feedback Loop

Since this is AI-generated code, visual verification requires a workflow.

### Screenshot Capture

Use Spectacle (Linux/KDE) to capture the running application:

```bash
# Capture active window
spectacle -a -b -n -o /path/to/scratchpad/screenshot.png

# Capture full screen
spectacle -f -b -n -o /path/to/scratchpad/screenshot.png

# Capture region (interactive)
spectacle -r -b -n -o /path/to/scratchpad/screenshot.png
```

**Flags:**
| Flag | Purpose |
|------|---------|
| `-a` | Active window only |
| `-f` | Full screen |
| `-r` | Rectangular region |
| `-b` | Background mode (no GUI) |
| `-n` | No notification |
| `-o` | Output path |

### Workflow

1. **Build and run** the application
2. **Capture** screenshot with Spectacle
3. **Analyze** the image (AI can view it)
4. **Iterate** on code based on visual feedback
5. **Delete** screenshot when done

Screenshots are temporary - save to scratchpad, delete after analysis.

---

## ASCII Wireframes

Before coding UI, sketch layouts in ASCII:

```
┌─────────────────────────────────────────────────────────────┐
│  File   Edit   View   Tools                          [─][□][×]│
├───────────┬─────────────────────────────┬───────────────────┤
│  Library  │                             │    Properties     │
│           │                             │                   │
│  [thumb]  │                             │  Name: ________   │
│  model1   │                             │  File: ________   │
│           │      3D Viewport            │  Size: _____ MB   │
│  [thumb]  │                             │  Triangles: ____  │
│  model2   │        [model]              │                   │
│           │                             ├───────────────────┤
│  [thumb]  │                             │    G-Code Info    │
│  model3   │                             │                   │
│           │                             │  Time: 00:00:00   │
│  [thumb]  │                             │  Distance: ___ mm │
│  model4   │                             │  Tool changes: _  │
│           │                             │                   │
├───────────┴─────────────────────────────┴───────────────────┤
│  Ready                                        Models: 45     │
└─────────────────────────────────────────────────────────────┘
```

Use ASCII wireframes to:
- Plan layout before implementation
- Communicate visual intent
- Agree on structure before coding

---

## Reference Images

User can provide:
- Screenshots of other applications ("like this")
- Hand-drawn mockups (photo)
- Annotated screenshots ("move this here")

Drop images in conversation for visual context.

---

## ImGui Considerations

ImGui is immediate mode - layout is in code:

```cpp
// Docking layout is defined programmatically
ImGui::DockBuilderDockWindow("Library", dockLeft);
ImGui::DockBuilderDockWindow("Properties", dockRight);
ImGui::DockBuilderDockWindow("Viewport", dockCenter);
```

**Layout persistence:** ImGui saves dock layout to `imgui.ini`. User arrangements persist across sessions.

**Theming:** Use ImGui's built-in dark theme or customize:
```cpp
ImGui::StyleColorsDark();
// Or customize individual colors
```

---

## Panel Inventory

| Panel | Purpose | Default Position |
|-------|---------|------------------|
| Library | Browse imported models | Left |
| Viewport | 3D model preview | Center |
| Properties | Selected model details | Right |
| G-Code | G-code analysis results | Right (tabbed) |
| Optimizer | 2D cut planning | Right (tabbed) |

---

## Responsive Design

ImGui handles window resizing automatically. Guidelines:

- Use relative sizing where possible
- Set minimum panel widths to prevent unusable states
- Test at different window sizes
- Allow user to resize/rearrange panels (ImGui docking)
