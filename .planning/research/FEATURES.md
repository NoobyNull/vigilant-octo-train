# Feature Landscape

**Domain:** Library Storage & Organization — v1.1 Milestone
**Researched:** 2026-02-21
**Confidence:** MEDIUM-HIGH

> This file replaces the v1.0 FEATURES.md scope. It focuses exclusively on what
> is needed for the v1.1 milestone: content-addressable storage, Kuzu graph DB,
> FTS5 search, category hierarchy, and project export. Prior CNC/business-workflow
> features from the old FEATURES.md remain valid but are out of scope here.

---

## Table Stakes

Features the user will notice are missing if absent. These define the milestone's
minimum viable outcome.

| Feature | Why Expected | Complexity | Depends On |
|---------|--------------|------------|------------|
| Hash-transparent model paths | Physical file path is fragile — user has 3k+ models on NAS. Library must survive moves without broken references | Medium | Content-addressable store, DB schema |
| Invisible deduplication | User already has duplicates; importing the same model twice should silently deduplicate, not create a second entry | Low | CAS: hash-on-import already exists in v1.0 |
| Category assignment (single + multi) | Standard in every digital library tool. Without it the library is a flat list | Medium | Category hierarchy DB, UI chrome |
| Category browsing in library panel | A flat list of 3,000 models is unusable. Sidebar tree or filter chips are the minimum | Medium | Category hierarchy, LibraryPanel |
| FTS search across name/tags/notes | Ctrl+F-style instant search. Users expect it to find partial words | Medium | FTS5 virtual table, search bar in LibraryPanel |
| Search-as-you-type with debounce | Typing in a search box and waiting for Enter is a 2005 pattern. Results must update within 150–300ms of the user pausing | Low | FTS5 prefix queries, timer on UI thread |
| Project export as portable archive | A project that only works on the machine it was created on is not shareable. Export must bundle models + manifest | Medium | CAS store layout, manifest format |
| File handling mode UI (Copy/Move/Leave) | The enum exists but has no UI. For NAS users this is essential — import should offer to copy files locally | Low | FileHandlingMode enum (already in code) |

---

## Differentiators

Features that go beyond the baseline, delivering meaningful competitive advantage
for this specific user (CNC woodworker with a large NAS library).

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| iTunes-style hash folder layout (aa/bb/…) | Two-level hash prefix sharding gives fast filesystem access and automatic dedup across renames. Zero user-visible complexity | Low | `models/<AA>/<BBCC…>.stl` on disk |
| Kuzu graph edges for project↔model links | Graph queries ("which projects use this model?", "all models not in any project") are natural and fast in Kuzu vs. JOIN-heavy SQL | High | Kuzu v1.x integration, new dependency |
| Category genus hierarchy (not flat tags) | Categories organized as wood species genera (e.g. Oak > Red Oak, White Oak) match the user's domain vocabulary. Enables faceted filtering | Medium | Category DB with parent_id, UI tree |
| Auto-suggest category on import | Filename pattern matching to suggest a genus category (e.g. "oak_bracket.stl" → suggest Oak). No AI required | Low | Regex/substring match on import dialog |
| Smart file handling (same-fs move vs. cross-fs copy) | Auto-detect whether the source is on the same filesystem as the library; if yes, offer atomic move. If NAS, offer copy. Reduces user decision burden | Low | stat() + st_dev comparison |
| BM25-ranked search results | FTS5 bm25() scoring ensures a model named "oak shelf bracket" ranks above one that merely mentions oak in notes | Low | ORDER BY bm25() in FTS5 query |
| Project manifest with content hashes | Each file in the .dwproj archive is listed with its SHA-256, enabling integrity verification on import | Low | manifest.json in archive |
| Forward-compatible manifest versioning | A `format_version` field in the manifest lets future versions parse older exports gracefully. Unknown fields are ignored | Low | JSON schema design |
| Orphan detection (files not in any project) | Graph query shows models that exist in the library but are not referenced by any project. Useful for cleanup | Medium | Kuzu query over edges |

---

## Anti-Features

Features that look useful but should be explicitly excluded from v1.1.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| Expose hash paths to users | Users do not think in SHA-256 prefixes. Showing `aa/bb3c…` paths in the UI violates the core promise of content-addressable storage (that physical location is invisible) | Always display logical name; hash path is internal to FileIOManager |
| User-editable category hierarchy during import | Allowing create/rename/delete of categories inside the import dialog creates modal UI complexity and possible orphan categories | Import dialog shows existing categories with a "+ New" button that opens the dedicated category editor |
| AI genus classification | Out of scope per PROJECT.md. Adds API key requirement and latency | Defer to future milestone. Auto-suggest with regex is sufficient for v1.1 |
| Cloud sync / NAS-aware sync | Out of scope. App remains fully offline | NAS is treated as a mounted local path; no special NAS protocol |
| Full-text search across G-code content | G-code files can be megabytes. FTS on G-code content is a separate product feature with different tokenizer needs | FTS covers model metadata (name, tags, notes, category) only |
| Drag-and-drop category reordering in tree | High complexity (ImGui tree drag-drop), low value for v1.1 | Provide a dedicated "Edit Categories" dialog with up/down buttons |
| Multi-project cross-export (batch archive) | Explicitly out of scope per PROJECT.md | Single project export is sufficient for v1.1 |
| Database migrations | Per PROJECT.md rulebook: delete and recreate DB on schema change | No migration system; schema version stored in DB, mismatch triggers reseed |

---

## Feature Detail: Content-Addressable Storage UX

**How Git, iTunes, and Docker handle this (research synthesis):**

- **What users see:** Logical names, titles, or filenames. The hash is never shown.
- **What the app manages internally:** A two-level directory (`<first-2-hex>/<remaining-hex>.<ext>`) indexed by hash.
- **Deduplication is invisible:** iTunes silently deduplicated tracks by content; users see one entry regardless of how many times they dragged the same file in. Docker shows one image layer even when shared across dozens of images.
- **Consolidation is explicit:** iTunes had a "Consolidate Library" action that pulled in referenced files. We need an equivalent "Copy all referenced files to library" action for the NAS use case.
- **References vs. managed files:** Lightroom pioneered "Add" (leave in place, referenced) vs. "Copy" (managed). This is exactly the FileHandlingMode enum that already exists. The UX lesson is: make the distinction clear at import time with plain-English labels, not technical terms.

**Recommended UX for v1.1 file handling dialog:**

```
When importing, where should the file live?
  ( ) Leave where it is  — path stored as-is, may break if file moves
  ( ) Copy into library  — safe copy to local library folder [recommended for NAS]
  ( ) Move into library  — moves file from its current location (local files only)
```

Show a warning icon next to "Leave where it is" when the source is on a different
filesystem (NAS detected via `st_dev`).

---

## Feature Detail: Category Hierarchy UX

**Established patterns (research synthesis):**

- **Folders vs. tags tradeoff:** Folder hierarchies are stable and pervasive but allow only one location per item. Tags are flexible but become unmanageable with scale. Best practice for asset management: use a hierarchy for primary classification + tags for secondary attributes.
- **Recommended for v1.1:** A lightweight genus hierarchy (max 2 levels: genus > species) for primary classification, combined with the existing free-form tags system. This matches woodworking domain vocabulary without over-engineering.
- **Tree view breadcrumbs:** When a category is selected, show a breadcrumb (e.g., "Oak > Red Oak") above the model grid. This is table-stakes navigation.
- **Multi-category assignment:** A model can belong to multiple categories (e.g., an oak bracket may be in "Oak" and "Joinery"). Not controversial — standard in every DAM. Implemented as a many-to-many table.
- **Auto-suggest at import:** Match the filename substring against genus/species names. Present as a chip-style suggestion ("Looks like Oak — assign?") not a modal. User can dismiss with one click.
- **Tree testing note:** User testing on hierarchies shows users prefer shallow trees (2 levels max) with clear labels over deep taxonomies. Keep the genus hierarchy flat by default with optional expansion.

**Category DB schema (recommended):**

```sql
CREATE TABLE categories (
    id        INTEGER PRIMARY KEY,
    name      TEXT NOT NULL,
    parent_id INTEGER REFERENCES categories(id),
    sort_order INTEGER DEFAULT 0
);

CREATE TABLE model_categories (
    model_id    INTEGER REFERENCES models(id),
    category_id INTEGER REFERENCES categories(id),
    PRIMARY KEY (model_id, category_id)
);
```

Pre-seed with the 32 wood species already in the materials system, organized under
genus parents (Oak, Maple, Walnut, etc.).

---

## Feature Detail: Full-Text Search UX

**Established patterns (research synthesis):**

- **Search-as-you-type:** Results should update within 150–300ms of the user pausing. Debounce at the UI layer (start a 200ms timer on each keystroke, cancel and restart on subsequent keystrokes). FTS5 prefix queries (`name MATCH 'oak*'`) support this directly.
- **Result ranking:** BM25 is FTS5's built-in ranking. Column weights should favour `name` over `tags` over `notes`. Use `ORDER BY bm25(models_fts, 10.0, 3.0, 1.0)` where 10/3/1 maps to name/tags/notes weights.
- **Filters:** After initial text search, users want to narrow by category or file type. Implement as chip-style filter toggles below the search bar. "Models | G-code | Both" already exists as the ViewTab enum.
- **No results state:** Show a friendly empty state ("No models match 'walrut' — did you mean 'walnut'?"). FTS5 does not provide spell correction; a simple Levenshtein distance check against category names covers the most common case.
- **Escape to clear:** Pressing Esc in the search bar clears it and restores the full library view. Standard desktop app behaviour.
- **Search scope:** For v1.1, search covers: model name, tags, notes/description, category name. G-code filename. Does NOT cover G-code content (see anti-features).

**FTS5 external content table setup:**

```sql
CREATE VIRTUAL TABLE models_fts USING fts5(
    name,
    tags,
    notes,
    content='models',
    content_rowid='id',
    tokenize='unicode61 remove_diacritics 1'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER models_ai AFTER INSERT ON models BEGIN
    INSERT INTO models_fts(rowid, name, tags, notes)
    VALUES (new.id, new.name, new.tags, new.notes);
END;

CREATE TRIGGER models_au AFTER UPDATE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, tags, notes)
    VALUES ('delete', old.id, old.name, old.tags, old.notes);
    INSERT INTO models_fts(rowid, name, tags, notes)
    VALUES (new.id, new.name, new.tags, new.notes);
END;

CREATE TRIGGER models_ad AFTER DELETE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, tags, notes)
    VALUES ('delete', old.id, old.name, old.tags, old.notes);
END;
```

---

## Feature Detail: Project Export UX

**Established patterns (research synthesis):**

Every portable project archive format (JAR, Chrome extensions, Unity packages,
OSGi bundles, dbt manifest.json) converges on the same table-stakes:

| Field | Purpose | Example |
|-------|---------|---------|
| `format_version` | Forward compatibility: unknown versions display a warning on import | `1` (integer, monotonic) |
| `app_version` | Which app commit created this archive | git commit hash |
| `created_at` | ISO-8601 timestamp | `"2026-02-21T14:30:00Z"` |
| `project_name` | Human-readable name | `"Maple Chair"` |
| `project_id` | UUID for dedup on reimport | `"a3f2c1d9-…"` |
| `models[]` | Array of bundled model entries | see below |
| `gcode_files[]` | Array of bundled G-code files | see below |

**Model entry in manifest:**

```json
{
  "id": 42,
  "name": "bracket_v3",
  "sha256": "aabbcc…",
  "original_filename": "bracket_v3.stl",
  "file_in_archive": "models/aa/aabbcc….stl",
  "categories": ["Oak", "Joinery"],
  "tags": ["cnc", "bracket"],
  "notes": ""
}
```

**Forward compatibility rule:** Importers must ignore unknown top-level keys. If
`format_version` is higher than the importer supports, show a warning ("This
archive was created with a newer version of Digital Workshop — some data may not
be imported"). Never hard-fail on unknown fields.

**Archive layout inside the .dwproj ZIP:**

```
my_project.dwproj (ZIP)
├── manifest.json
├── models/
│   ├── aa/
│   │   └── aabb…stl
│   └── cc/
│       └── ccdd…obj
└── gcode/
    └── bracket_cut.gcode
```

The `models/` directory inside the archive mirrors the CAS layout on disk. This
means models can be extracted directly into an existing CAS store without path
remapping.

---

## Feature Dependencies

```
Content-Addressable Store (CAS)
    └──foundation for──> All model storage paths in DB
    └──enables──> Deduplication (hash already exists in v1.0)
    └──enables──> Project export (hash == archive path)

Category Hierarchy DB
    └──requires──> New categories + model_categories tables
    └──enables──> Category browsing in LibraryPanel
    └──enables──> Auto-suggest on import
    └──enables──> Category filter in FTS5 queries

FTS5 Virtual Table
    └──requires──> models_fts table + 3 triggers
    └──enables──> Search-as-you-type in LibraryPanel
    └──enables──> BM25 ranked results
    └──depends on──> Category Hierarchy (for category-scoped search)

File Handling Mode UI
    └──requires──> FileHandlingMode enum (✓ already exists)
    └──requires──> st_dev same-filesystem detection
    └──enables──> NAS import workflow

Kuzu Graph DB
    └──parallel to──> SQLite (not a replacement)
    └──stores──> project↔model edges, category relationships
    └──enables──> "which projects use this model?" queries
    └──enables──> Orphan detection

Project Export (.dwproj)
    └──requires──> CAS layout (archive mirrors CAS path structure)
    └──requires──> manifest.json schema
    └──requires──> Category data from Category Hierarchy DB
    └──enables──> Project sharing between installations
```

**Critical dependency ordering:**

1. CAS store layout must be decided before project export (archive mirrors CAS)
2. Category DB must be seeded before LibraryPanel category browser
3. FTS5 triggers must be added at schema creation, not retrofitted (triggers fire on INSERT — existing rows need a one-time reindex)
4. Kuzu must be integrated at the CMake level before any graph features can be prototyped

---

## MVP Recommendation

**What must ship for v1.1 to be a coherent release:**

1. **File handling mode UI** (P0 — unblocks NAS workflow, low complexity, enum exists)
2. **CAS disk layout** — migrate from stored paths to `<lib>/<AA>/<hash>.<ext>` (P0 — foundational for everything else)
3. **Category hierarchy** — DB tables, CRUD, LibraryPanel sidebar tree (P1)
4. **FTS5 search** — virtual table + triggers + debounced search bar (P1)
5. **Project export as .dwproj** — ZIP archive + manifest.json (P1)
6. **Kuzu integration** — project↔model edges, orphan detection (P2 — useful but library works without it if FTS + categories are in place)

**What to defer past v1.1:**

- Auto-suggest category on import (P3 — nice, but categories can be assigned after import)
- Orphan detection UI (P3 — depends on Kuzu; graph queries work but UI is polish)
- "Consolidate Library" action for pulling in referenced files (P3 — add after CAS stable)
- Spell-correction in search empty state (P3 — Levenshtein check is optional polish)

---

## Complexity Assessment

| Feature | Estimated Complexity | Primary Risk |
|---------|---------------------|--------------|
| File handling mode UI | LOW — enum exists, dialog is straightforward | None |
| CAS disk layout migration | MEDIUM — must migrate existing DB rows + files safely | Data integrity during migration; use transaction + rollback |
| Category DB + seed | LOW — CRUD tables, pre-seed from existing 32 species | Seed data consistency with MaterialsPanel species |
| Category UI in LibraryPanel | MEDIUM — ImGui tree widget, multi-assign modal | LibraryPanel is already large (>400 lines); may need split |
| FTS5 virtual table | LOW — SQL only, no new dependencies | Trigger correctness; test with bulk inserts |
| Search-as-you-type debounce | LOW — 200ms timer, prefix query | Thread safety: timer fires on main thread, safe |
| Kuzu CMake integration | HIGH — new dependency, C++ API learning curve | Binary size (+2–5MB), build system complexity |
| Kuzu graph schema | MEDIUM — node/edge types, Cypher queries | API stability: Kuzu v1.x is relatively new |
| Project export (ZIP + manifest) | MEDIUM — need a zip library (miniz or libzip) | ZIP dependency: confirm permissive license |
| Project import (manifest parse) | MEDIUM — version check, conflict resolution on reimport | Duplicate detection: model with same hash already in library |

---

## Sources

**Content-Addressable Storage (CAS) UX patterns:**
- iTunes "Managed vs Referenced" — [iDownloadBlog: iTunes referenced library mode](https://www.idownloadblog.com/2018/05/03/how-to-itunes-referenced-library-mode/) — MEDIUM confidence
- Docker layer deduplication — [oneuptime.com: Docker Storage Internals](https://oneuptime.com/blog/post/2026-02-08-how-to-understand-docker-storage-internals-graph-driver-layer-store/view) — MEDIUM confidence
- Git CAS abstraction — [Medium: How Git's Object Storage Actually Works](https://medium.com/@sohail_saifi/how-gits-object-storage-actually-works-and-why-it-s-revolutionary-780da2537eef) — MEDIUM confidence

**Category / Hierarchy UX:**
- Tags vs folders tradeoff — [Connecter: Why Tags Are Better Than Folders (2025)](https://blog.connecterapp.com/why-are-tags-better-than-folders-2025-update-bf73f2162510) — MEDIUM confidence
- Breadcrumb hierarchy UX — [NN/g: Breadcrumbs 11 Design Guidelines](https://www.nngroup.com/articles/breadcrumbs/) — HIGH confidence (authoritative UX research)
- Adobe Experience Manager taxonomy — [AEM: Organize your digital assets](https://experienceleague.adobe.com/en/docs/experience-manager-cloud-service/content/assets/manage/organize-assets) — MEDIUM confidence
- Tree testing — [Clay: Implementing Tree Testing in UX](https://clay.global/blog/ux-guide/tree-testing) — MEDIUM confidence

**Search UX:**
- Search-as-you-type patterns — [DesignMonks: Master Search UX in 2026](https://www.designmonks.co/blog/search-ux-best-practices) — MEDIUM confidence
- FTS5 practical guide — [thelinuxcode.com: SQLite FTS5 in Practice](https://thelinuxcode.com/sqlite-full-text-search-fts5-in-practice-fast-search-ranking-and-real-world-patterns/) — MEDIUM confidence
- FTS5 official docs — [SQLite FTS5 Extension](https://sqlite.org/fts5.html) — HIGH confidence

**File handling / import UX:**
- Lightroom Add/Copy/Move model — [Photofocus: Lightroom Classic Import Dialog](https://photofocus.com/software/understanding-lightroom-classics-import-dialog/) — MEDIUM confidence
- [Outdoor Imaging: Importing Images — Copy/Move/Add explained](https://www.outdoorimaging.net/lightroom-learning-group-blog/2017/11/19/importing-images-copy-as-dng-copy-move-add-huh) — MEDIUM confidence

**Project export / manifest:**
- Manifest file conventions — [Wikipedia: Manifest file](https://en.wikipedia.org/wiki/Manifest_file) — MEDIUM confidence
- OSGi forward-compatibility (ignore unknown headers) — [OSGi Core 7 spec](https://docs.osgi.org/specification/osgi.core/7.0.0/framework.module.html) — HIGH confidence (official spec)

---

*Feature research for: Digital Workshop v1.1 — Library Storage & Organization*
*Researched: 2026-02-21*
