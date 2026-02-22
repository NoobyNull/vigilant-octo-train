# Requirements: v1.1 Library Storage & Organization

**Milestone:** v1.1
**Created:** 2026-02-21
**Status:** Approved

---

## v1.1 Requirements

### Storage (STOR)

- [ ] **STOR-01**: User's imported models are stored in content-addressable blob directories (hash-based layout: `blobs/ab/cd/abcdef...ext`)
- [ ] **STOR-02**: File writes use atomic temp+verify+rename to prevent corruption from crashes or network disconnects
- [ ] **STOR-03**: Application cleans up orphaned temp files on startup
- [ ] **STOR-04**: Application detects non-local filesystems (NAS, Google Drive, OneDrive, mapped drives) and auto-suggests copy instead of move

### Import (IMPORT)

- [ ] **IMPORT-01**: User can choose organizational strategy on import: keep in place, copy to organized local store, or move to organized local store
- [ ] **IMPORT-02**: Import dialog shows filesystem detection result and recommends copy for non-local sources

### Organization (ORG)

- [ ] **ORG-01**: User can assign models to categories in a 2-level hierarchy (genus/species)
- [ ] **ORG-02**: User can search models via full-text search across name, tags, filename, and category
- [ ] **ORG-03**: GraphQLite extension loaded at DB init for Cypher graph queries
- [ ] **ORG-04**: Models, categories, and projects represented as graph nodes with relationship edges
- [ ] **ORG-05**: User can query relationships via graph traversal (e.g., "all models in project X", "related models")

### Export (EXPORT)

- [ ] **EXPORT-01**: User can export a project as .dwproj ZIP archive containing manifest + model blobs + materials + thumbnails
- [ ] **EXPORT-02**: .dwproj archive is portable and self-contained (can be imported on another machine)

---

## Future Requirements (Deferred)

- AI genus classification via Gemini Vision on thumbnails
- Multi-project cross-optimization
- Cloud sync / collaborative features

## Out of Scope

- STEP/IGES CAD format support — not a CAD tool
- Cloud-based operation — fully offline desktop app
- CAD editing — view/import only
- Database migration system — no user base, delete and recreate per rulebook
- Real-time chat/collaboration — single-user

---

## Traceability

| REQ-ID | Phase | Plan | Status |
|--------|-------|------|--------|
| STOR-01 | — | — | Pending |
| STOR-02 | — | — | Pending |
| STOR-03 | — | — | Pending |
| STOR-04 | — | — | Pending |
| IMPORT-01 | — | — | Pending |
| IMPORT-02 | — | — | Pending |
| ORG-01 | — | — | Pending |
| ORG-02 | — | — | Pending |
| ORG-03 | — | — | Pending |
| ORG-04 | — | — | Pending |
| ORG-05 | — | — | Pending |
| EXPORT-01 | — | — | Pending |
| EXPORT-02 | — | — | Pending |
