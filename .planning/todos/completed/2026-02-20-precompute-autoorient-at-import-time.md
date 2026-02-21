---
title: Precompute autoOrient at import time and store in DB
area: core/import
created: 2026-02-20
priority: high
source: conversation
---

# Precompute autoOrient at import time and store in DB

## Problem
`autoOrient()` currently runs on every model load (~2-3s for large meshes like 17M+ triangle STLs). This is wasted work since the result is deterministic for a given mesh.

## Proposed Solution
1. Add orientation columns to the `models` table (`orient_yaw`, `orient_matrix` or similar)
2. Compute `autoOrient()` once during the import pipeline
3. Store the computed yaw and orientation matrix in the database
4. On model load, read stored values instead of recomputing

## Files Likely Affected
- `src/core/database/schema.cpp` — add columns to models table
- `src/core/import/import_queue.cpp` — compute autoOrient after mesh load
- `src/core/library/library_manager.h/cpp` — store/retrieve orient data
- `src/app/application.cpp` — use stored orient values in onModelSelected
- `src/ui/panels/viewport_panel.cpp` — accept pre-computed orient on setMesh
- `src/core/mesh/mesh.h/cpp` — possibly extract orient computation to a standalone function

## Context
This was identified during STL loading optimization work. Binary STL loading was optimized from ~42s to ~3s by removing vertex dedup and moving autoOrient to the worker thread. Precomputing at import would eliminate the remaining ~2-3s orient cost entirely.
