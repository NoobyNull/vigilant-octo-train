---
area: project
priority: low
slug: save-camera-views-per-project-model
created: 2026-02-22
---

# Save/Restore Camera Views Per Project Model

When a model is part of a project, save the camera position/orientation so opening the project restores the user's preferred viewing angle.

- `project_model_views` table or JSON column on `project_models` junction
- Store: orbit distance, yaw, pitch, target position
- Restore when navigating to model from Project Panel
- Allow saving multiple named views per model (e.g., "Front", "Detail")
