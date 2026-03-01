---
created: 2026-03-01T04:48:45.845Z
title: Center progress bar overlay text in direct carve panel
area: ui
files:
  - src/ui/panels/direct_carve_panel.cpp:392
  - src/ui/panels/direct_carve_panel.cpp:753
---

## Problem

The "Computing heightmap..." overlay text on the ImGui::ProgressBar drifts rightward as the bar fills instead of staying centered. ImGui's default ProgressBar overlay text is left-aligned relative to the filled portion, so as the fill grows the text shifts with it into the gray unfilled area.

## Solution

Replace the built-in overlay parameter with manual text centering:
1. Render `ImGui::ProgressBar(progress, size, "")` with empty overlay
2. Use `ImGui::CalcTextSize()` to measure the label
3. Position text at the bar's horizontal center using `SetCursorPos` or `GetItemRectMin`/`GetItemRectMax` arithmetic
4. Draw centered text with `ImGui::GetWindowDrawList()->AddText()` or `SameLine` cursor tricks

Applies to both progress bar instances (lines 392 and 753).
