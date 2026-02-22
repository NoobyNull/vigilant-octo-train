---
phase: 04-organization-and-graph
plan: 01
subsystem: database
tags: [schema, fts5, categories, search]
dependency_graph:
  requires: []
  provides: [categories-table, model-categories-junction, fts5-search, category-crud]
  affects: [model_repository, schema]
tech_stack:
  added: [FTS5]
  patterns: [external-content-fts5, before-update-trigger, unicode61-tokenizer]
key_files:
  created: []
  modified:
    - cmake/Dependencies.cmake
    - src/core/database/schema.h
    - src/core/database/schema.cpp
    - src/core/database/model_repository.h
    - src/core/database/model_repository.cpp
    - tests/test_schema.cpp
key_decisions:
  - "BEFORE UPDATE trigger for FTS5 delete phase (Pitfall 2 prevention)"
  - "unicode61 tokenizer over porter to preserve non-English model names (Pitfall 7)"
  - "BM25 weighting: name=10.0, tags=3.0 for search relevance"
  - "Prefix wildcard auto-appended for search-as-you-type"
metrics:
  duration: "2m 40s"
  completed: "2026-02-22T01:02:42Z"
  tasks: 2
  tests_passing: 540
---

# Phase 04 Plan 01: Schema v7 Categories + FTS5 Summary

Schema v7 with categories hierarchy, model-categories junction, and FTS5 full-text search using BM25-ranked queries with unicode61 tokenizer

## Task Summary

| Task | Name | Commit | Key Changes |
|------|------|--------|-------------|
| 1 | Enable FTS5 + schema v7 with categories and FTS5 | 54d9d42 | Schema bump to v7, categories + model_categories tables, models_fts virtual table, 4 sync triggers, category indexes, v7 migration |
| 2 | FTS5 search + category methods in ModelRepository | 2818a21 | CategoryRecord struct, searchFTS with BM25, category CRUD, assignCategory/removeCategory, findByCategory |

## Key Implementation Details

### Schema Changes (v7)
- **categories** table: id, name, parent_id (2-level hierarchy), sort_order, UNIQUE(name, parent_id)
- **model_categories** junction: model_id + category_id composite PK with CASCADE deletes
- **models_fts** FTS5 virtual table: external content from models(name, tags), unicode61 tokenizer
- **4 triggers**: models_fts_ai (AFTER INSERT), models_fts_bu (BEFORE UPDATE), models_fts_au (AFTER UPDATE), models_fts_ad (AFTER DELETE)
- **3 indexes**: idx_categories_parent, idx_model_categories_model, idx_model_categories_category

### Repository Methods Added
- `searchFTS(query)` -- BM25-ranked FTS5 search with auto prefix wildcard, limit 500
- `assignCategory(modelId, categoryId)` / `removeCategory(modelId, categoryId)` -- junction table ops
- `findByCategory(categoryId)` -- JOIN query for models in a category
- `createCategory(name, parentId)` / `deleteCategory(id)` -- category CRUD
- `getAllCategories()` / `getRootCategories()` / `getChildCategories(parentId)` -- category queries

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated schema test version assertions**
- **Found during:** Task 1
- **Issue:** test_schema.cpp asserted version == 6, now it is 7
- **Fix:** Updated both GetVersion_AfterInit and DoubleInit_Idempotent to expect version 7
- **Files modified:** tests/test_schema.cpp
- **Commit:** 54d9d42

## Verification Results

- Build: compiles cleanly (cmake + make)
- Tests: 540/540 passing
- Schema version: 7 (confirmed in schema.h)
- BEFORE UPDATE trigger: present in both createTables and migrate
- FTS5 flag: SQLITE_ENABLE_FTS5 in Dependencies.cmake
- unicode61 tokenizer: confirmed in models_fts creation
