---
title: Lazy-write missing computed model data back to DB on load
area: core/loading
created: 2026-02-20
priority: high
source: conversation
---

# Lazy-write missing computed model data back to DB on load

## Problem
When a model is loaded and computed metadata (like autoOrient results) is missing from the DB, the computation runs but the results are discarded. The next load recomputes the same data.

## Proposed Solution
After computing autoOrient (or other derived data) on load, write the results back to the models table if they were missing. This provides a lazy migration path for existing models that were imported before orient columns existed.

Combined with the import-time precompute todo, this gives full coverage:
- **New imports**: orient data computed and stored at import time
- **Existing models**: orient data computed on first load, written back for future loads

## Implementation
1. After autoOrient runs on the worker thread, check if the DB row has orient data
2. If missing, write the computed yaw/matrix back to the models table
3. On subsequent loads, read stored values and skip recomputation

## Files Likely Affected
- `src/core/database/schema.cpp` — add orient columns to models table
- `src/core/library/library_manager.h/cpp` — read/write orient data
- `src/app/application.cpp` — onModelSelected: check DB, compute if missing, write back

## Relationship
This is the complement to the "precompute autoOrient at import time" todo. Both should be implemented together — import-time for new models, lazy-write for existing models.
