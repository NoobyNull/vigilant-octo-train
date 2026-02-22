# Requirements: v1.2 UI Freshen-Up

**Milestone:** v1.2
**Created:** 2026-02-21
**Status:** Approved

---

## v1.2 Requirements

### Typography (FONT)

- [ ] **FONT-01**: Application uses a modern proportional sans-serif font (Inter or equivalent) instead of the default ImGui bitmap font
- [ ] **FONT-02**: FontAwesome 6 icons remain merged and render correctly at all font sizes
- [ ] **FONT-03**: Font atlas uses oversampling (2x or 3x) for crisp rendering at non-integer scales

### Scaling (DPI)

- [ ] **DPI-01**: Application detects display DPI on startup and scales UI elements accordingly
- [ ] **DPI-02**: Font size, padding, spacing, and rounding values scale proportionally with DPI
- [ ] **DPI-03**: Application handles DPI changes at runtime (e.g., moving window between monitors on Windows)

### Theme Polish (THEME)

- [ ] **THEME-01**: Dark theme uses a refined, cohesive color palette with clear visual hierarchy
- [ ] **THEME-02**: Light theme is fully customized (not delegating to ImGui's default `StyleColorsLight`)
- [ ] **THEME-03**: High contrast theme updated to match new spacing/font conventions
- [ ] **THEME-04**: All three themes share consistent spacing, padding, and rounding values (via `applyBaseStyle`)

### Visual Consistency (VIS)

- [ ] **VIS-01**: Panels and dialogs have consistent margins, header styling, and section spacing
- [ ] **VIS-02**: Buttons, inputs, and interactive elements have visually distinct hover/active states across all themes

---

## Future Requirements (Deferred)

- Custom window chrome / frameless title bar
- Platform-adaptive accent colors (Windows accent color API)
- Font ligature support
- Icon font upgrade to FontAwesome 6 Pro (licensing)

## Out of Scope

- wxWidgets migration — too deep an ImGui integration
- New functional features — this milestone is purely visual/UX
- Animation/transition effects — ImGui doesn't support them natively

---

## Traceability

| REQ-ID | Phase | Plan | Status |
|--------|-------|------|--------|
| FONT-01 | Phase 5 | — | Pending |
| FONT-02 | Phase 5 | — | Pending |
| FONT-03 | Phase 5 | — | Pending |
| DPI-01 | Phase 5 | — | Pending |
| DPI-02 | Phase 5 | — | Pending |
| DPI-03 | Phase 5 | — | Pending |
| THEME-01 | Phase 5 | — | Pending |
| THEME-02 | Phase 5 | — | Pending |
| THEME-03 | Phase 5 | — | Pending |
| THEME-04 | Phase 5 | — | Pending |
| VIS-01 | Phase 5 | — | Pending |
| VIS-02 | Phase 5 | — | Pending |
