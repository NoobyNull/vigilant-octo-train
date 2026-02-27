---
created: 2026-02-21
title: Dear ImGui theme modernization
area: ui
files:
  - src/ui/theme.cpp
priority: medium
---

## Summary

Modernize the Dear ImGui theme to eliminate the "blocky/antiquated" feel that may deter Windows users from adopting the application. The goal is a polished, contemporary look without switching UI frameworks.

## Key Areas

### Font Rendering
- Replace default ProggyClean with a modern sans-serif font (Inter, Segoe UI on Windows, system sans on Linux)
- Enable font atlas oversampling for crisp text at all DPI levels
- Load platform-specific fonts at runtime

### Style & Rounding
- Increase `WindowRounding`, `FrameRounding`, `GrabRounding`, `TabRounding` for softer edges
- Tune padding and spacing (`FramePadding`, `ItemSpacing`, `WindowPadding`) for a less cramped layout
- Review and update color palette for a modern, cohesive look

### DPI / HiDPI Support
- Ensure proper DPI scaling on Windows (per-monitor DPI awareness)
- Scale fonts and style values based on display DPI

### Platform Feel
- Consider platform-adaptive accent colors
- Ensure native file dialogs are used everywhere (already partially done via `file_dialog.cpp`)

## Context

Concern raised during discussion about porting to Windows. A wxWidgets migration was considered but rejected — the ImGui integration is too deep (~1,637 ImGui calls across 31 files). Theme modernization achieves the visual goal at a fraction of the cost.
