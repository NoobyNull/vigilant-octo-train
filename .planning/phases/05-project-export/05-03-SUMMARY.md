---
phase: 05-project-export
plan: 03
subsystem: export
tags: [project-export, materials, thumbnails, gap-closure, dwproj]

requires:
  - ProjectExportManager with exportProject/importProject (05-01)
  - MaterialRepository with findById/insert (Phase 01)
  - MaterialArchive::load() for metadata extraction (Phase 01)
provides:
  - Materials bundled in .dwproj archives (materials/<id>.dwmat)
  - Thumbnails bundled in .dwproj archives (thumbnails/<hash>.png)
  - Material assignment restoration on import
  - Thumbnail restoration on import
affects:
  - src/core/export/project_export_manager.h
  - src/core/export/project_export_manager.cpp
  - tests/test_project_export_manager.cpp

tech-stack:
  added: []
  patterns: [material-dedup-in-archive, thumbnail-bundling, manifest-extension]

key-files:
  created: []
  modified:
    - src/core/export/project_export_manager.h
    - src/core/export/project_export_manager.cpp
    - tests/test_project_export_manager.cpp

decisions:
  - "Materials keyed by DB id (not hash) since materials lack content hashes"
  - "Shared materials deduplicated in archive via writtenMaterialIds set"
  - "MaterialArchive::load() used for metadata extraction with fallback to minimal record"
  - "Raw SQL for material_id assignment since MaterialManager not accessible from export context"

metrics:
  duration: "3m 47s"
  completed: "2026-02-22"
  tasks: 2
  tests: 5
---

# Phase 05 Plan 03: Gap Closure - Materials & Thumbnails in Archive Summary

**One-liner:** Added materials/*.dwmat and thumbnails/*.png bundling to .dwproj export/import with manifest schema extension and deduplication.

## What Was Built

### Export Enhancements (project_export_manager.h/.cpp)

**ManifestModel struct** extended with:
- `materialId` (optional i64) - material DB id from models table
- `materialInArchive` - archive path like "materials/3.dwmat"
- `thumbnailInArchive` - archive path like "thumbnails/<hash>.png"

**exportProject()** now includes two additional phases:
- Phase A: Reads thumbnail PNGs from disk and writes to `thumbnails/<hash>.png` in ZIP
- Phase B: Queries material_id per model, reads .dwmat files, writes to `materials/<id>.dwmat` in ZIP with deduplication for shared materials

**buildManifestJson()** signature extended to accept material and thumbnail mappings. Each model entry now includes `material_id`, `material_in_archive`, and `thumbnail_in_archive` fields.

**importProject()** now includes two restoration phases:
- Phase A: Extracts thumbnail PNGs to thumbnail cache directory, updates model records
- Phase B: Extracts .dwmat files to materials directory, creates MaterialRecords (using MaterialArchive::load for metadata), assigns material_id via SQL UPDATE. Shared materials mapped via oldToNewMaterialId to avoid duplicates.

**getModelMaterialId()** helper queries `SELECT material_id FROM models WHERE id = ?` for export.

### Test (test_project_export_manager.cpp)

New test `RoundTripPreservesMaterialsAndThumbnails` verifying:
- Exported ZIP contains materials/<id>.dwmat entry
- Exported ZIP contains thumbnails/<hash>.png entry
- Manifest model entries include material_id, material_in_archive, thumbnail_in_archive
- Imported model has material_id set in second database
- MaterialRecord exists in second database after import
- Thumbnail file exists on disk after import

All 5 ProjectExport tests pass (4 existing + 1 new).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed [[nodiscard]] warnings on SQL bind/execute calls**
- **Found during:** Task 1 build verification
- **Issue:** -Werror caught unused return values from stmt.bindInt() and stmt.execute()
- **Fix:** Used conditional chaining for bind calls and (void) cast for execute
- **Files modified:** src/core/export/project_export_manager.cpp
- **Commit:** d0fda27

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 | d0fda27 | feat(05-03): add material and thumbnail export/import to ProjectExportManager |
| 2 | c60097c | test(05-03): add material and thumbnail round-trip test |

## Self-Check: PASSED
