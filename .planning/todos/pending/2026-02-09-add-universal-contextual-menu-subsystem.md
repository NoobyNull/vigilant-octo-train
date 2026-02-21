---
created: 2026-02-09T15:35:08.351Z
title: Add universal contextual menu subsystem
area: ui
files:
  - src/managers/ui_manager.h
  - src/managers/ui_manager.cpp
  - src/ui/panels/viewport_panel.cpp
  - src/ui/panels/library_panel.cpp
  - src/ui/panels/properties_panel.cpp
  - src/ui/panels/project_panel.cpp
---

## Problem

The application currently has no right-click context menu support. Each panel (Viewport, Library, Properties, Project, etc.) would benefit from contextual actions relevant to what the user is interacting with. For example: right-clicking a model in the Library could offer "Open", "Delete", "Export"; right-clicking in the Viewport could offer "Reset View", "Fit to Bounds", "Lighting Settings".

A universal system is needed so that each module/panel can register its own context menu items without duplicating ImGui context menu boilerplate.

## Solution

Design a contextual menu subsystem where:
- Each panel registers its context menu entries (label, action callback, optional icon, optional enabled-predicate)
- A central `ContextMenuManager` or similar class handles rendering the ImGui popup
- Panels call a method like `contextMenu.show(entries)` on right-click within their area
- The system handles ImGui popup lifecycle (OpenPopup, BeginPopup, EndPopup)
- Support for separators, nested submenus, and disabled items
- Potentially support dynamic entries (e.g., plugin-provided menu items in the future)
