# Architecture Patterns: v1.1 Library Storage & Organization

**Domain:** C++17 desktop app — content-addressable model library, graph relationships, full-text search
**Researched:** 2026-02-21
**Overall confidence:** HIGH (codebase read directly; Kuzu API confirmed via GitHub source; FTS5 from official SQLite docs)

---

## How the New Features Integrate

This document addresses v1.1 integration only. Existing architecture is treated as fixed.
All source paths are confirmed by reading the actual codebase.

---

## 1. StorageManager — Content-Addressable File Operations

### Where it sits

`StorageManager` belongs in `src/core/storage/` as a pure core component with zero UI or rendering
dependencies. It sits at the same level as `LibraryManager` — both are owned by `Application` and
injected where needed.

```
Application
  ├── StorageManager      (NEW — src/core/storage/)
  ├── KuzuManager         (NEW — src/core/graph/)
  ├── LibraryManager      (existing)
  ├── ProjectManager      (existing)
  ├── ImportQueue         (modified — receives StorageManager* + KuzuManager*)
  ├── FileIOManager       (modified — receives ProjectExportManager*)
  └── ... (all other existing members unchanged)
```

`StorageManager` is a peer of `LibraryManager`, not a subordinate. `LibraryManager` calls
`StorageManager` when resolving mesh paths for loading. `ImportQueue` calls it during the
file-handling stage of import.

### Responsibility boundary

`StorageManager` owns one thing: the blob store on disk.

| Responsibility | Owner |
|---|---|
| `hash → blob path` mapping | StorageManager |
| Copy/move file into blob store at hash path | StorageManager |
| Check if hash already in blob store | StorageManager |
| Remove orphaned blobs on model delete | StorageManager |
| `ModelRecord.file_path` column | LibraryManager / ModelRepository |
| Category assignment | LibraryManager (see §6) |
| Graph edges for model-category and model-project | KuzuManager (see §2) |

### Hash-based directory structure

iTunes-style two-level sharding on the first four hex characters of the content hash:

```
<data_dir>/blobs/
  ab/
    cd/
      abcdef1234567890.stl
  12/
    34/
      12345678abcdef00.obj
```

Path formula: `blobs / hash[0..1] / hash[2..3] / (hash + "." + ext)`

The file extension is preserved so the mesh loader can identify format without probing file content.
The hash is the same FNV-1a hex string already computed by `hash::computeBuffer()` in Stage 2.

### Interface sketch

```cpp
// src/core/storage/storage_manager.h
class StorageManager {
  public:
    explicit StorageManager(const Path& blobRoot);

    // Pure path computation — no I/O. Callable before the blob exists.
    Path blobPath(const std::string& hash, const std::string& ext) const;

    // Copy source into blob store. No-op (idempotent) if blob already exists.
    // Returns final blob path. Sets error on failure.
    Path storeFile(const Path& source, const std::string& hash,
                   const std::string& ext, std::string& error);

    // Move source into blob store. Falls back to copy+delete cross-filesystem.
    Path moveFile(const Path& source, const std::string& hash,
                  const std::string& ext, std::string& error);

    // Returns true if blob exists
    bool exists(const std::string& hash, const std::string& ext) const;

    // Delete blob. Called from LibraryManager::removeModel().
    bool remove(const std::string& hash, const std::string& ext);

    // Default root: paths::getDataDir() / "blobs"
    static Path defaultBlobRoot();

  private:
    Path m_blobRoot;
};
```

`StorageManager` replaces `FileHandler` for the content-addressable path. `FileHandler` is
retained only for the `LeaveInPlace` mode (where no copying or hashing-to-path occurs) and
for backward compatibility during the transition. It can be removed in a future cleanup pass.

---

## 2. Kuzu Graph Database — Coexistence with SQLite

### Architectural placement

A new `KuzuManager` in `src/core/graph/` owns the single Kuzu `Database` object and vends
`Connection` objects on demand — exactly mirroring how `ConnectionPool` vends SQLite `Database`
connections.

```cpp
// src/core/graph/kuzu_manager.h
class KuzuManager {
  public:
    explicit KuzuManager(const Path& dbDir); // Kuzu requires a directory, not a file

    bool open();
    void close();
    bool isOpen() const;

    // Create a new Connection for the calling thread.
    // Caller must NOT share this Connection across threads.
    std::unique_ptr<kuzu::main::Connection> acquireConnection();

    // Convenience: serialized write helper for single-statement writes
    bool write(const std::string& cypher, std::string& error);

  private:
    Path m_dbDir;
    std::unique_ptr<kuzu::main::Database> m_database;
    std::mutex m_writeMutex; // Only needed if using write() helper
};
```

The Kuzu directory lives at `paths::getDataDir() / "graph"`. This path should be added to
`app_paths.h` as `getGraphDir()`.

### Thread safety contracts

Kuzu's documented model: one `Database`, many `Connection` objects. A `Connection` is **not**
thread-safe — each thread must create its own. This mirrors SQLite NOMUTEX mode exactly.

| Aspect | Kuzu | SQLite (existing) |
|---|---|---|
| Database object | One, owned by KuzuManager | One primary + ConnectionPool |
| Per-thread connection | New `Connection` each task | `ScopedConnection` from pool |
| Write isolation | Kuzu serializes internally | App-level `std::mutex` on writes |
| Connection pooling | No — create fresh per task | Yes — pool of N |

In `ImportQueue::processTask()`, the worker creates a `Connection` via
`kuzuMgr->acquireConnection()`. This is a cheap operation once the `Database` is open.

**Critical:** Kuzu v0.11.3 (latest as of research, 2025-10-10) is the recommended version.
Pre-built shared libraries (`libkuzu.so` / `kuzu.dll`) are available from GitHub releases at
`github.com/kuzudb/kuzu/releases`. The static library requires linking ~12 transitive deps
(antlr4_cypher, re2, zstd, brotli, utf8proc, etc.) and should be avoided.

### CMake integration

Add to `cmake/Dependencies.cmake`:

```cmake
# Kuzu graph database — pre-built shared library
# Download URL must be updated at integration time; verify SHA256 from release page.
FetchContent_Declare(
    kuzu
    URL https://github.com/kuzudb/kuzu/releases/download/v0.11.3/libkuzu-linux-x86_64.tar.gz
    # URL_HASH SHA256=<verify-at-integration-time>
)
FetchContent_MakeAvailable(kuzu)
# Tarball provides kuzu.hpp and libkuzu.so at top level
```

For cross-platform, use CMake generator expressions to select the correct tarball URL
(linux-x86_64, win-x86_64) based on `CMAKE_SYSTEM_NAME`.

Confidence: MEDIUM — the pattern is confirmed from Kuzu docs, but exact tarball filenames and
hashes must be verified at integration time from the releases page.

### Schema initialization in Kuzu

On first open, `KuzuManager::open()` runs schema DDL via Cypher:

```cypher
CREATE NODE TABLE IF NOT EXISTS Model(id INT64, hash STRING, name STRING, PRIMARY KEY(id));
CREATE NODE TABLE IF NOT EXISTS Category(name STRING, PRIMARY KEY(name));
CREATE NODE TABLE IF NOT EXISTS Project(id INT64, name STRING, PRIMARY KEY(id));
CREATE REL TABLE IF NOT EXISTS IN_CATEGORY(FROM Model TO Category);
CREATE REL TABLE IF NOT EXISTS IN_PROJECT(FROM Model TO Project);
```

The `IF NOT EXISTS` syntax makes this idempotent — safe to call every startup.

---

## 3. FTS5 — Trigger-Based Sync

### Recommendation: trigger-based with external content table

Use FTS5 in **external content mode** (`content=models`) with three SQL triggers on `models`.

Rationale for triggers over manual C++ sync:
1. SQLite writes go through `ModelRepository::insert()`, `::update()`, `::remove()`, and
   `::updateTags()` — four separate write paths. Triggers catch all of them without touching
   any C++ code.
2. Triggers run in the same transaction as the write, so the FTS index is always consistent.
3. The AFTER INSERT/UPDATE/DELETE pattern is the canonical SQLite FTS5 approach per
   `sqlite.org/fts5.html`.

### Schema additions (go into `Schema::createTables`, version 7)

```sql
-- FTS5 virtual table: external content from models
CREATE VIRTUAL TABLE IF NOT EXISTS models_fts USING fts5(
    name,
    tags,
    content='models',
    content_rowid='id',
    tokenize='porter ascii'
);

-- AFTER INSERT: add to FTS index
CREATE TRIGGER IF NOT EXISTS models_ai AFTER INSERT ON models BEGIN
    INSERT INTO models_fts(rowid, name, tags)
    VALUES (new.id, new.name, new.tags);
END;

-- AFTER DELETE: remove from FTS index
CREATE TRIGGER IF NOT EXISTS models_ad AFTER DELETE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, tags)
    VALUES ('delete', old.id, old.name, old.tags);
END;

-- AFTER UPDATE: delete old entry, insert new entry
CREATE TRIGGER IF NOT EXISTS models_au AFTER UPDATE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, tags)
    VALUES ('delete', old.id, old.name, old.tags);
    INSERT INTO models_fts(rowid, name, tags)
    VALUES (new.id, new.name, new.tags);
END;
```

Note: Use AFTER triggers, not BEFORE. BEFORE UPDATE triggers cause FTS5 to read the new values
when trying to delete old tokens, corrupting the index.

### FTS search query in ModelRepository

```cpp
std::vector<ModelRecord> ModelRepository::searchFTS(const std::string& query) {
    auto stmt = m_db.prepare(
        "SELECT m.* FROM models m "
        "INNER JOIN models_fts ON models_fts.rowid = m.id "
        "WHERE models_fts MATCH ? "
        "ORDER BY models_fts.rank "
        "LIMIT 500");
    stmt.bindText(1, query);
    std::vector<ModelRecord> results;
    while (stmt.step()) {
        results.push_back(rowToModel(stmt));
    }
    return results;
}
```

Porter stemming (`tokenize='porter ascii'`) allows "dovetail" to match "dovetails" and
"dovetailed" — useful for a workshop model library.

### Schema version bump

Schema goes 6 → 7. Per project rulebook ("no user base, delete and recreate"), the
`Schema::migrate()` path is the correct place to add the FTS virtual table and triggers.

---

## 4. Import Pipeline — Stage Placement for Hash-Addressing

### Current pipeline (confirmed by reading `import_queue.cpp`)

```
Stage 1: Reading           file::readBinary → fileData
Stage 2: Hashing           hash::computeBuffer → fileHash
Stage 3: CheckingDuplicate ModelRepository::findByHash
Stage 4: Parsing           LoaderFactory::loadFromBuffer → mesh
Stage 5: Inserting         ModelRepository::insert (stores sourcePath)
Stage 5.5: FileHandling    FileHandler::handleImportedFile (copy/move)
                           ModelRepository::update to patch file_path
Stage 6: WaitingForThumbnail  pushed to main thread
```

### Required changes for content-addressable storage

Stage 5 and 5.5 change together. The blob path is computable at record construction time
(Stage 5), before the insert, because `StorageManager::blobPath()` is a pure function of
hash and extension — no I/O.

```
Stage 5: Inserting  (MODIFIED)
  blobPath = storageMgr->blobPath(task.fileHash, task.extension)  // pure computation
  record.filePath = (mode == LeaveInPlace) ? task.sourcePath : blobPath
  ModelRepository::insert(record) → modelId

Stage 5.5: Blob Store  (REPLACES FileHandler call)
  if mode == CopyToLibrary: storageMgr->storeFile(source, hash, ext)
  if mode == MoveToLibrary: storageMgr->moveFile(source, hash, ext)
  if mode == LeaveInPlace:  (no-op — sourcePath already in record)
  on failure: ModelRepository::remove(modelId); fail task

Stage 5.6: Graph Insert  (NEW)
  conn = kuzuMgr->acquireConnection()
  conn->query("CREATE (:Model {id: $id, hash: $hash, name: $name})", params)
  failure is non-fatal: log warning, continue

Stage 6: WaitingForThumbnail  (unchanged)
```

Key change: the two-step "insert sourcePath → update to blobPath" is eliminated.
`record.filePath` is correct at insert time. If the blob store step fails, the DB record
is deleted (rollback the insert). This prevents records pointing at non-existent paths.

### ImportQueue constructor change

```cpp
// src/core/import/import_queue.h
ImportQueue(ConnectionPool& pool,
            LibraryManager* libraryManager = nullptr,
            StorageManager* storageManager = nullptr,   // NEW
            KuzuManager* kuzuManager = nullptr);         // NEW
```

`Application` owns both and passes pointers at construction.

---

## 5. Project Export — Which Manager Owns It

### Recommendation: new `ProjectExportManager` in `src/core/export/`

`FileIOManager` (currently 78 lines header, moderate .cpp size) orchestrates in-flight import
and project open/save. Adding portable archive export would add a second distinct concern plus
the binary size to walk the blob store and pack archives.

`ProjectExportManager` is a focused new component:

```cpp
// src/core/export/project_export_manager.h
class ProjectExportManager {
  public:
    ProjectExportManager(Database* db, StorageManager* storageMgr,
                         KuzuManager* kuzuMgr);

    struct ExportResult {
        bool success = false;
        std::string error;
        int modelCount = 0;
        u64 totalBytes = 0;
    };

    // Export project + associated model blobs + optional graph snapshot to .dwproj zip
    ExportResult exportProject(const Project& project, const Path& outputPath,
                               std::function<void(float)> progressCallback = nullptr);

    // Import a .dwproj archive into the current installation
    ExportResult importProject(const Path& archivePath,
                               std::function<void(float)> progressCallback = nullptr);
};
```

`FileIOManager` gets a `ProjectExportManager*` member and delegates the export menu action to it.
`FileIOManager` still owns the file dialog invocation and toast/progress display — consistent with
how it handles all other file I/O actions.

### Export archive layout (.dwproj)

```
project.json            project metadata: id, name, description, model list (hashes + names)
models/
  abcdef12.stl          flat layout — hash.ext, no sharding inside the zip
  12345678.obj
graph.cypher            optional: Kuzu DUMP of relationships for models in this project
```

Flat `models/` layout inside the zip is intentional: human-inspectable, no nested directories,
and portable to any receiving installation. On import, each blob is stored at its correct
shard path in the receiving installation's blob store via `StorageManager::storeFile()`.

The `.dwproj` extension is new and distinct from `.dwp` (the existing project archive format).
`.dwproj` is the portable export; `.dwp` is the in-place project save.

---

## 6. Category Assignment — LibraryManager Responsibility

### Recommendation: extend LibraryManager, no new manager

Categories are a property of a model — same conceptual domain as tags, names, and material
assignments that `LibraryManager` already owns. A separate `CategoryManager` would create a
split with no meaningful boundary: where does "assign category to model" belong if not in
`LibraryManager`?

### Schema: two columns on `models` (Option A)

The category hierarchy is two levels (category → genus). Simple columns avoid a join on every
model fetch. The `categories` normalized table is deferred until deeper nesting is needed.

```sql
-- Schema version 7 additions (alongside FTS virtual table)
ALTER TABLE models ADD COLUMN category TEXT DEFAULT NULL;
ALTER TABLE models ADD COLUMN genus TEXT DEFAULT NULL;

CREATE INDEX IF NOT EXISTS idx_models_category ON models(category);
CREATE INDEX IF NOT EXISTS idx_models_genus ON models(genus);
```

The FTS5 triggers created for version 7 do NOT need to include `category` or `genus` in the
index — these are facet filters, not full-text searched fields. If full-text search over
category values is desired, the triggers can be extended.

### LibraryManager additions

```cpp
// In library_manager.h
bool assignCategory(i64 modelId, const std::string& category,
                    const std::string& genus = "");
std::vector<std::string> getCategories();
std::vector<std::string> getGenera(const std::string& category);
std::vector<ModelRecord> filterByCategory(const std::string& category);
std::vector<ModelRecord> filterByGenus(const std::string& genus);
```

### Dual-write: SQLite + Kuzu for categories

`LibraryManager::assignCategory()` writes:
1. SQLite: `UPDATE models SET category=?, genus=? WHERE id=?` — fast filter queries
2. Kuzu: `MERGE (:Category {name: $cat}) ... CREATE (:Model)-[:IN_CATEGORY]->(:Category)`
   — enables graph traversal queries like "all models in the same category as model X"

The Kuzu write is non-fatal. If Kuzu is unavailable (first run before schema init), log a
warning and continue. SQLite is the source of truth.

---

## 7. Complete Import Data Flow with Content-Addressable Storage

```
[Source file]   NAS / local disk / drag-drop
      |
      v
ImportQueue::enqueue()            main thread
      |
      v
ThreadPool worker → ImportQueue::processTask()
      |
      +-- Stage 1: Reading
      |   file::readBinary(sourcePath) → fileData
      |
      +-- Stage 2: Hashing
      |   hash::computeBuffer(fileData) → fileHash (hex string, existing FNV-1a)
      |
      +-- Stage 3: CheckingDuplicate
      |   ScopedConnection conn(m_pool)
      |   ModelRepository::findByHash(fileHash)
      |   → if found: record DuplicateRecord, decrement counter, return
      |   → if not found: continue
      |
      +-- Stage 4: Parsing
      |   LoaderFactory::loadFromBuffer(fileData, ext) → mesh
      |   fileData cleared (memory freed)
      |
      +-- Stage 5: Inserting  [MODIFIED]
      |   mode = Config::getFileHandlingMode()
      |   blobPath = (mode != LeaveInPlace)
      |             ? storageMgr->blobPath(fileHash, ext)   // pure computation
      |             : sourcePath
      |   record.filePath = blobPath
      |   ModelRepository::insert(record) → modelId
      |
      +-- Stage 5.5: Blob Store  [REPLACES FileHandler]
      |   if mode == CopyToLibrary:
      |     storageMgr->storeFile(sourcePath, fileHash, ext) → ok/err
      |   if mode == MoveToLibrary:
      |     storageMgr->moveFile(sourcePath, fileHash, ext)  → ok/err
      |   if mode == LeaveInPlace:
      |     (no-op)
      |   on error: ModelRepository::remove(modelId), fail task
      |
      +-- Stage 5.6: Graph Insert  [NEW]
      |   conn = kuzuMgr->acquireConnection()
      |   conn->query("CREATE (:Model {id: $id, hash: $hash, name: $name})")
      |   failure: log warning, continue (non-fatal)
      |
      +-- Stage 6: WaitingForThumbnail
          push to m_completed → main thread GL thumbnail gen (unchanged)
```

### LeaveInPlace path

When `LeaveInPlace` is selected, `ModelRecord.file_path` stores the original NAS/local path.
`StorageManager` is not called. This is the correct behavior — the user explicitly chose not to
copy. The blob store is not consulted for `LeaveInPlace` records on subsequent loads.

---

## Component Boundaries Summary

### New components

| Component | Location | Owned By | Key Dependencies |
|---|---|---|---|
| `StorageManager` | `src/core/storage/` | Application | `std::filesystem`, `app_paths` |
| `KuzuManager` | `src/core/graph/` | Application | `kuzu.hpp`, `app_paths` |
| `ProjectExportManager` | `src/core/export/` | Application | Database, StorageManager, KuzuManager, Archive |

### Modified components

| Component | What Changes |
|---|---|
| `ImportQueue` | Constructor takes `StorageManager*` and `KuzuManager*`; Stage 5 computes blob path; Stage 5.5 calls StorageManager; Stage 5.6 added for graph insert |
| `ModelRepository` | Add `searchFTS()`, `findByCategory()`, `findByGenus()`; `ModelRecord` gets `category` and `genus` fields |
| `Schema` | Version 7: `models_fts` virtual table + 3 triggers; `category`/`genus` columns on `models`; version bump in `CURRENT_VERSION` constant |
| `LibraryManager` | Add `assignCategory()`, `getCategories()`, `getGenera()`, `filterByCategory()`, `filterByGenus()`; inject `KuzuManager*` for dual-write |
| `FileIOManager` | Add `ProjectExportManager*` member; delegate export/import-project actions to it |
| `app_paths.h` | Add `getBlobStoreDir()` returning `getDataDir() / "blobs"`; add `getGraphDir()` returning `getDataDir() / "graph"` |
| `Application` | Construct and own `StorageManager`, `KuzuManager`, `ProjectExportManager`; inject into downstream components |
| `cmake/Dependencies.cmake` | Add Kuzu pre-built library via `FetchContent_Declare` with URL pointing to release tarball |

### Untouched components

- `FileHandler` — retained for `LeaveInPlace`; not removed in v1.1
- `ConnectionPool` / `Database` — no changes; SQLite thread model unchanged
- `MainThreadQueue` — no changes; thumbnail callback flow unchanged
- All UI panels — no structural changes; panels call existing manager methods for new features
- `Workspace` — no changes

---

## Build Order

Dependencies determine sequencing:

| Step | Component | Depends On | Notes |
|---|---|---|---|
| 1 | `app_paths` additions | nothing | Add `getBlobStoreDir()` and `getGraphDir()` |
| 2 | `StorageManager` | `app_paths` | Pure filesystem; no new library deps; test first |
| 3 | CMake: Kuzu FetchContent | cmake infrastructure | Add to Dependencies.cmake; verify tarball URL |
| 4 | `KuzuManager` | Kuzu CMake (step 3) | Stub open/close/acquireConnection; wire schema init |
| 5 | Schema v7 | existing Schema | FTS triggers + category columns; bump CURRENT_VERSION to 7 |
| 6 | `ModelRepository` additions | Schema v7 (step 5) | `searchFTS()`, `findByCategory()`, `findByGenus()` |
| 7 | `LibraryManager` additions | ModelRepository (step 6), KuzuManager (step 4) | Category methods + dual-write |
| 8 | `ImportQueue` modifications | StorageManager (step 2), KuzuManager (step 4) | Stage 5/5.5/5.6 changes |
| 9 | `ProjectExportManager` | StorageManager (step 2), KuzuManager (step 4) | Export + import archive |
| 10 | `FileIOManager` changes | ProjectExportManager (step 9) | Wire export/import delegation |
| 11 | `Application` wiring | All above | Construct new managers, inject pointers |
| 12 | UI: library panel category UI | LibraryManager (step 7) | Category filter/assign in LibraryPanel |
| 13 | UI: FTS search bar | ModelRepository (step 6) | Replace substring search with FTS |

Steps 2–4 can be parallelized (no inter-dependency). Steps 5–8 must be sequential.

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: KuzuManager inside LibraryManager

**What goes wrong:** LibraryManager accumulates two database dependencies; becomes hard to test;
violates single-responsibility as the model CRUD coordinator.
**Instead:** KuzuManager is a peer, injected separately into LibraryManager and ImportQueue as
distinct constructor arguments.

### Anti-Pattern 2: Manual FTS sync in C++ write paths

**What goes wrong:** Every write path — `insert`, `update`, `updateTags`, `remove` — must
explicitly call a C++ FTS update function. Easy to miss one. Results in stale FTS index after
partial updates.
**Instead:** SQL AFTER INSERT/UPDATE/DELETE triggers run automatically in the same transaction
as any write, regardless of which C++ path triggered it.

### Anti-Pattern 3: Insert sourcePath then update to blobPath

**What goes wrong:** If the process crashes between the insert and the path-update, the DB
record points at `sourcePath` which was moved/deleted by Stage 5.5. On restart the model
record is broken.
**Instead:** Compute `blobPath()` before the insert (it is a pure hash-to-path function).
Insert the blob path directly. If the subsequent file copy/move fails, delete the DB record.

### Anti-Pattern 4: Pooling Kuzu Connections

**What goes wrong:** A `kuzu::main::Connection` is not thread-safe. Sharing one via a pool
with per-connection locking introduces serialization that negates parallelism.
**Instead:** Create a fresh `Connection` per worker task. Kuzu connection creation is O(1)
overhead once the `Database` is open — same philosophy as SQLite ScopedConnection.

### Anti-Pattern 5: Treating LeaveInPlace paths as blob paths

**What goes wrong:** Code that checks blob existence for every model load breaks for
`LeaveInPlace` models whose paths start with a NAS mount point, not the blob root.
**Instead:** Distinguish the two cases at load time by checking if `file_path` is under
`getBlobStoreDir()`. Only call `StorageManager::exists()` for blob-rooted paths.

### Anti-Pattern 6: Kuzu static library linkage

**What goes wrong:** Static Kuzu requires ~12 transitive library deps (antlr4_cypher, antlr4_
runtime, brotlidec, brotlicommon, utf8proc, re2, serd, fastpfor, miniparquet, zstd, miniz,
mbedtls, lz4). This makes CMake configuration fragile and significantly increases binary size.
**Instead:** Use the pre-built shared library from GitHub releases. Consistent with existing
use of shared libcurl.

---

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| StorageManager placement and interface | HIGH | Pattern derived directly from codebase |
| Import pipeline stage ordering | HIGH | `import_queue.cpp` read in full; changes are additive |
| FTS5 trigger pattern | HIGH | Official SQLite FTS5 docs; pattern confirmed via multiple sources |
| LibraryManager for categories | HIGH | Consistent with existing ownership model |
| ProjectExportManager split from FileIOManager | HIGH | File limits + responsibility separation clear from code |
| Kuzu Connection-per-thread model | HIGH | GitHub source example + concurrency docs confirm single Database |
| Kuzu CMake via pre-built library | MEDIUM | Pattern confirmed; tarball URL + SHA256 must be verified at integration |
| Kuzu static library | LOW | Confirmed problematic by transitive dep list; avoid |

---

## Sources

- Kuzu C++ API example: [github.com/kuzudb/kuzu/blob/master/examples/cpp/main.cpp](https://github.com/kuzudb/kuzu/blob/master/examples/cpp/main.cpp)
- Kuzu releases page (v0.11.3 latest as of 2025-10-10): [github.com/kuzudb/kuzu/releases](https://github.com/kuzudb/kuzu/releases)
- Kuzu concurrency docs: [docs.kuzudb.com/concurrency/](https://docs.kuzudb.com/concurrency/)
- SQLite FTS5 external content + triggers: [sqlite.org/fts5.html](https://sqlite.org/fts5.html)
- FTS5 AFTER trigger pattern: [simonh.uk/2021/05/11/sqlite-fts5-triggers/](https://simonh.uk/2021/05/11/sqlite-fts5-triggers/)
- Codebase files read:
  - `src/core/import/import_queue.cpp` (pipeline stages, exact code)
  - `src/core/import/import_queue.h`
  - `src/core/import/file_handler.cpp` (copy/move/leave logic)
  - `src/core/import/import_task.h` (stage enum, task struct)
  - `src/core/library/library_manager.h`
  - `src/core/database/schema.cpp` (existing tables, version 6)
  - `src/core/database/connection_pool.h`
  - `src/core/database/model_repository.h`
  - `src/core/database/database.h`
  - `src/core/project/project.h`
  - `src/core/archive/archive.h`
  - `src/core/paths/app_paths.h`
  - `src/core/mesh/hash.h`
  - `src/app/application.h`
  - `src/managers/file_io_manager.h`
  - `cmake/Dependencies.cmake`
  - `.planning/PROJECT.md`
