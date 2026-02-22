# Technology Stack — v1.1 Library Storage & Organization

**Project:** Digital Workshop
**Scope:** NEW capabilities only for v1.1. Existing stack (C++17, SDL2, Dear ImGui, OpenGL 3.3,
SQLite3 3.45.0, miniz 3.0.2, zlib, GLM, nlohmann/json, libcurl, stb, GoogleTest, CMake 3.20+)
is locked and not re-evaluated.
**Researched:** 2026-02-21
**Overall confidence:** HIGH (all critical claims verified against GitHub API, official docs,
and kernel/platform documentation)

---

## Executive Summary

Five capabilities are needed for v1.1. Only ONE requires a new dependency (Kuzu). The other
four are unlocked by a single compile flag, platform syscalls, or existing libraries already
in the build.

| Capability | Approach | New Dependency? |
|------------|----------|-----------------|
| Kuzu graph database | Prebuilt `.so`/`.dll` download via FetchContent URL, C API | YES |
| FTS5 full-text search | `SQLITE_ENABLE_FTS5` compile flag on existing SQLite | NO |
| Filesystem detection (local vs NAS) | `statfs()`/`GetDriveTypeW()` platform calls | NO |
| Content-addressable storage | `std::filesystem` + existing SHA-256 hash code | NO |
| `.dwproj` ZIP export | Existing `miniz_static` (already in build) | NO |

---

## 1. Kuzu Graph Database

### Version

**v0.11.3** — released 2025-10-10. Confirmed via GitHub API query on 2026-02-21.
This is the latest stable release. The repository has been archived at this version;
the v0.11.x line is the final stable series.

Source: https://github.com/kuzudb/kuzu/releases/tag/v0.11.3

### C++20 vs C++17 — Critical Constraint

Kuzu's source requires C++20 (`set(CMAKE_CXX_STANDARD 20)` in its `CMakeLists.txt`).
This project is C++17 with `CMAKE_CXX_STANDARD_REQUIRED ON`. Building Kuzu from source
via FetchContent would require a project-wide C++ standard upgrade — unacceptable risk
for a stable shipped codebase.

**Solution: Use prebuilt shared library + C API header only.**

Kuzu ships prebuilt `libkuzu` binaries for all three platforms as GitHub release assets.
The C API header (`kuzu.h`) is pure C with `extern "C"` guards — zero C++20 contamination.

**Do NOT use `kuzu.hpp`** (the C++ wrapper). It is 338KB and requires C++20 features
(concepts, ranges). The C API in `kuzu.h` (73KB) covers everything needed.

### Prebuilt Release Assets (v0.11.3)

| Platform | Archive | Library File | Size |
|----------|---------|--------------|------|
| Linux x86_64 | `libkuzu-linux-x86_64.tar.gz` | `libkuzu.so` | 8.5 MB |
| Windows x86_64 | `libkuzu-windows-x86_64.zip` | `kuzu_shared.dll` + `kuzu_shared.lib` | 13 MB DLL |
| macOS universal | `libkuzu-osx-universal.tar.gz` | `libkuzu.dylib` | 10 MB |

Each archive contains exactly: `kuzu.h`, `kuzu.hpp` (ignore this), and the shared library.

### CMake Integration

Use `FetchContent_Declare` with a URL (downloads the prebuilt tarball, not source) and
create an `IMPORTED SHARED` target. This fits the existing FetchContent pattern in
`cmake/Dependencies.cmake`.

```cmake
# cmake/Dependencies.cmake — add after existing entries

# Kuzu graph database — prebuilt shared library (avoids C++20 build requirement)
set(KUZU_VERSION "0.11.3")

if(WIN32)
    set(KUZU_ARCHIVE  "libkuzu-windows-x86_64.zip")
    set(KUZU_LIB_NAME "kuzu_shared.lib")   # import lib for linking
    set(KUZU_DLL_NAME "kuzu_shared.dll")   # runtime DLL
elseif(APPLE)
    set(KUZU_ARCHIVE  "libkuzu-osx-universal.tar.gz")
    set(KUZU_LIB_NAME "libkuzu.dylib")
else()
    set(KUZU_ARCHIVE  "libkuzu-linux-x86_64.tar.gz")
    set(KUZU_LIB_NAME "libkuzu.so")
endif()

FetchContent_Declare(
    kuzu_prebuilt
    URL "https://github.com/kuzudb/kuzu/releases/download/v${KUZU_VERSION}/${KUZU_ARCHIVE}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(kuzu_prebuilt)

# Create IMPORTED target so consumers just link kuzu
add_library(kuzu SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(kuzu PROPERTIES
        IMPORTED_IMPLIB   "${kuzu_prebuilt_SOURCE_DIR}/${KUZU_LIB_NAME}"
        IMPORTED_LOCATION "${kuzu_prebuilt_SOURCE_DIR}/${KUZU_DLL_NAME}"
        INTERFACE_INCLUDE_DIRECTORIES "${kuzu_prebuilt_SOURCE_DIR}"
    )
else()
    set_target_properties(kuzu PROPERTIES
        IMPORTED_LOCATION "${kuzu_prebuilt_SOURCE_DIR}/${KUZU_LIB_NAME}"
        INTERFACE_INCLUDE_DIRECTORIES "${kuzu_prebuilt_SOURCE_DIR}"
    )
endif()

message(STATUS "  Kuzu:     ${KUZU_VERSION} (prebuilt shared library)")
```

**Linking:** Add `kuzu` to `target_link_libraries` for any target that calls the C API.

**Windows deployment:** `kuzu_shared.dll` must be copied next to the executable. Add a
post-build copy step or handle via the installer.

**Note on URL_HASH:** For reproducible builds, add SHA256 hash after first download:
```cmake
URL_HASH SHA256=<compute with sha256sum on downloaded archive>
```
Do this before shipping to production CI.

### Thread Safety Contract

Source: Kuzu concurrency documentation (https://docs.kuzudb.com/concurrency/)

**Rules (MEDIUM confidence — from official docs, not independently tested):**

1. **One `kuzu_database` per database directory.** Opening two separate `kuzu_database`
   instances to the same path causes undefined behavior. Enforce with a singleton.

2. **Multiple connections are safe.** Create one `kuzu_connection` per thread (or per
   logical operation). Connections are cheap to create and destroy.

3. **Write serialization is automatic.** Kuzu's transaction manager serializes concurrent
   writes. No external mutex needed for write isolation.

4. **Multi-core reads.** Query execution uses morsel-driven parallelism internally.
   Each connection's query gets multi-threaded automatically.

**Integration with existing architecture:**

The existing `ConnectionPool` owns all SQLite connections. Kuzu needs a separate, simpler
singleton (not a pool) because Kuzu handles its own internal concurrency:

```cpp
// src/core/storage/KuzuDatabase.h  (new file, max 400 lines)
#pragma once
#include <kuzu.h>
#include <filesystem>
#include <stdexcept>

// Thin RAII singleton over kuzu_database.
// One instance per app. Never copy or move.
class KuzuDatabase {
public:
    static KuzuDatabase& instance() {
        static KuzuDatabase inst;
        return inst;
    }

    void open(const std::filesystem::path& dbPath) {
        auto cfg = kuzu_default_system_config();
        if (kuzu_database_init(&db_, dbPath.string().c_str(), cfg) != KuzuSuccess) {
            throw std::runtime_error("Failed to open Kuzu database: " + dbPath.string());
        }
        opened_ = true;
    }

    // Returns a connection. Caller must destroy with kuzu_connection_destroy().
    // Thread-safe: create one connection per calling thread.
    kuzu_connection openConnection() {
        kuzu_connection conn{};
        kuzu_connection_init(&conn, &db_);
        return conn;
    }

    ~KuzuDatabase() {
        if (opened_) kuzu_database_destroy(&db_);
    }

    KuzuDatabase(const KuzuDatabase&) = delete;
    KuzuDatabase& operator=(const KuzuDatabase&) = delete;

private:
    KuzuDatabase() = default;
    kuzu_database db_{};
    bool opened_ = false;
};
```

### Kuzu C API Quick Reference

```c
// Lifecycle (once per app)
kuzu_database db;
kuzu_database_init(&db, "/path/to/kuzudb", kuzu_default_system_config());

// Per-operation connection (create per thread, destroy when done)
kuzu_connection conn;
kuzu_connection_init(&conn, &db);

// Execute query
kuzu_query_result result;
kuzu_connection_query(&conn, "MATCH (m:Model) RETURN m.hash LIMIT 10", &result);

// Iterate
while (kuzu_query_result_has_next(&result)) {
    kuzu_flat_tuple row;
    kuzu_query_result_get_next(&result, &row);
    kuzu_value val;
    kuzu_flat_tuple_get_value(&row, 0, &val);
    char* str = kuzu_value_get_string(&val);
    // use str...
    kuzu_value_destroy(&val);
    kuzu_flat_tuple_destroy(&row);
}
kuzu_query_result_destroy(&result);
kuzu_connection_destroy(&conn);

// App shutdown
kuzu_database_destroy(&db);
```

### Confidence

HIGH for version, download URLs, and C API usability from C++17 (all verified via
GitHub API and tarball inspection). MEDIUM for thread safety specifics (from documentation,
not independently tested in this project's context).

---

## 2. SQLite FTS5 Full-Text Search

### Decision: Enable via compile flag — no new dependency

FTS5 has been bundled in the SQLite amalgamation since version 3.9.0 (2015). The project
uses SQLite 3.45.0. FTS5 is present but disabled by `#ifdef SQLITE_ENABLE_FTS5`.

Source: https://sqlite.org/fts5.html

### CMake Change

One line added to the existing `sqlite3` target in `cmake/Dependencies.cmake`:

```cmake
# Existing block (lines ~88-93 in Dependencies.cmake):
add_library(sqlite3 STATIC
    ${sqlite3_SOURCE_DIR}/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

# ADD THIS LINE — enables FTS5 in the amalgamation:
target_compile_definitions(sqlite3 PUBLIC SQLITE_ENABLE_FTS5)

add_library(SQLite::SQLite3 ALIAS sqlite3)
```

`PUBLIC` propagates the definition to all consumers. Any code that includes `sqlite3.h`
and calls FTS5 SQL will match the compiled implementation.

**Verify:**
```sql
PRAGMA compile_options;
-- Expected output includes: ENABLE_FTS5
```

### Content Table + Trigger Pattern

Use a "content table" FTS5 configuration to avoid storing text twice. The real data stays
in the `models` table; FTS5 stores only the inverted index. Triggers keep them in sync.

```sql
-- FTS5 virtual table referencing models as the content source
CREATE VIRTUAL TABLE IF NOT EXISTS models_fts USING fts5(
    name,
    description,
    tags,
    content='models',
    content_rowid='id'
);

-- Rebuild index if table already has rows (idempotent)
INSERT INTO models_fts(models_fts) VALUES('rebuild');

-- Sync triggers (all three required)
CREATE TRIGGER IF NOT EXISTS models_fts_ai
AFTER INSERT ON models BEGIN
    INSERT INTO models_fts(rowid, name, description, tags)
    VALUES (new.id, new.name, new.description, new.tags);
END;

CREATE TRIGGER IF NOT EXISTS models_fts_ad
AFTER DELETE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, description, tags)
    VALUES ('delete', old.id, old.name, old.description, old.tags);
END;

CREATE TRIGGER IF NOT EXISTS models_fts_au
AFTER UPDATE ON models BEGIN
    INSERT INTO models_fts(models_fts, rowid, name, description, tags)
    VALUES ('delete', old.id, old.name, old.description, old.tags);
    INSERT INTO models_fts(rowid, name, description, tags)
    VALUES (new.id, new.name, new.description, new.tags);
END;
```

**Query (BM25-ranked results):**

```sql
SELECT m.id, m.name, bm25(models_fts) AS rank
FROM models_fts
JOIN models m ON models_fts.rowid = m.id
WHERE models_fts MATCH ?   -- bind user's search term
ORDER BY rank;
```

**Thread safety:** FTS5 tables are accessed via the existing `ConnectionPool` (WAL mode).
No additional locking. FTS5 respects SQLite's standard connection-level isolation.

**Warning:** The `content=` option means FTS5 does NOT store the text — the sync triggers
are mandatory. A missing trigger will produce stale results silently. Create the FTS5
table and all three triggers in a single transaction.

### Confidence

HIGH — verified against official SQLite documentation at sqlite.org/fts5.html.
Compile flag mechanism verified against sqlite.org/compile.html.

---

## 3. Cross-Platform Filesystem Detection (Local vs Network/NAS)

### Decision: Platform syscalls only — no library dependency

Three platforms, three different APIs, one abstraction interface. All APIs are stable
POSIX or Win32 — no new dependencies.

```cpp
// src/core/storage/FilesystemInfo.h  (new file)
#pragma once
#include <filesystem>
#include <string>

enum class StorageLocation { Local, Network, Unknown };

struct FilesystemInfo {
    StorageLocation location = StorageLocation::Unknown;
    std::string fsTypeName;  // "nfs", "smbfs", "ext4", etc. (when available)
};

// Returns the storage location for the filesystem containing `path`.
// Path does not need to exist — its parent directory suffices.
FilesystemInfo detectFilesystem(const std::filesystem::path& path);
```

### Linux Implementation

`statfs(2)` returns `f_type`, a magic number identifying the filesystem type.
Magic numbers are defined in kernel headers or can be hardcoded (they are stable ABI).

```cpp
// src/core/storage/FilesystemInfo_linux.cpp
#include "FilesystemInfo.h"
#include <sys/vfs.h>

FilesystemInfo detectFilesystem(const std::filesystem::path& path) {
    struct statfs buf{};
    auto p = path;
    // Walk up to find existing ancestor if path doesn't exist yet
    while (!std::filesystem::exists(p) && p.has_parent_path())
        p = p.parent_path();

    if (statfs(p.c_str(), &buf) != 0)
        return {StorageLocation::Unknown, ""};

    // Linux kernel magic numbers (stable, from <linux/magic.h>)
    constexpr long NFS_SUPER_MAGIC   = 0x6969;
    constexpr long SMB_SUPER_MAGIC   = 0x517B;
    constexpr long CIFS_MAGIC_NUMBER = 0xFF534D42;
    constexpr long SMB2_MAGIC_NUMBER = 0xFE534D42;  // SMB2
    constexpr long FUSE_SUPER_MAGIC  = 0x65735546;  // sshfs, rclone mounts

    bool isNetwork = (buf.f_type == NFS_SUPER_MAGIC  ||
                      buf.f_type == SMB_SUPER_MAGIC   ||
                      buf.f_type == CIFS_MAGIC_NUMBER ||
                      buf.f_type == SMB2_MAGIC_NUMBER ||
                      buf.f_type == FUSE_SUPER_MAGIC);

    return {isNetwork ? StorageLocation::Network : StorageLocation::Local, ""};
}
```

Note: Linux `statfs` does not expose `f_fstypename`. The magic number is the only reliable
discriminator. FUSE covers most sshfs/rclone/s3fuse NAS mounts.

### macOS Implementation

macOS `statfs` has `f_fstypename` — a string like `"nfs"`, `"smbfs"`, `"afpfs"`. This is
more reliable than `f_type` on macOS because `f_type` is inconsistent (autofs and APFS can
share the same value). Source: Apple Developer Forums (developer.apple.com/forums/thread/87745).

```cpp
// src/core/storage/FilesystemInfo_macos.cpp
#include "FilesystemInfo.h"
#include <sys/mount.h>   // macOS uses <sys/mount.h>, not <sys/vfs.h>
#include <string>

FilesystemInfo detectFilesystem(const std::filesystem::path& path) {
    struct statfs buf{};
    auto p = path;
    while (!std::filesystem::exists(p) && p.has_parent_path())
        p = p.parent_path();

    if (statfs(p.c_str(), &buf) != 0)
        return {StorageLocation::Unknown, ""};

    std::string fsType = buf.f_fstypename;
    bool isNetwork = (fsType == "nfs"    ||
                      fsType == "smbfs"  ||   // SMB/CIFS shares
                      fsType == "afpfs"  ||   // AFP (legacy Apple Filing Protocol)
                      fsType == "webdav" ||
                      fsType == "autofs");     // automount helper (usually network)

    return {isNetwork ? StorageLocation::Network : StorageLocation::Local, fsType};
}
```

### Windows Implementation

`GetDriveTypeW()` returns `DRIVE_REMOTE` (4) for network drives, including mapped drives
and UNC paths. Source: Microsoft Learn (updated Nov 2024).

```cpp
// src/core/storage/FilesystemInfo_win.cpp
#include "FilesystemInfo.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

FilesystemInfo detectFilesystem(const std::filesystem::path& path) {
    // GetDriveTypeW needs the root path (e.g. "C:\\" or "\\server\share\")
    std::wstring root = path.root_path().wstring();
    if (root.empty())
        return {StorageLocation::Unknown, ""};

    UINT driveType = GetDriveTypeW(root.c_str());
    bool isNetwork = (driveType == DRIVE_REMOTE);
    return {isNetwork ? StorageLocation::Network : StorageLocation::Local, ""};
}
```

Note on UNC paths: For `\\server\share\path`, `path.root_path()` returns `\\server\share\`.
`GetDriveTypeW` on a UNC root correctly returns `DRIVE_REMOTE`.

### Dispatch in CMakeLists

```cmake
# Use platform-appropriate implementation
if(WIN32)
    target_sources(dw_core PRIVATE src/core/storage/FilesystemInfo_win.cpp)
elseif(APPLE)
    target_sources(dw_core PRIVATE src/core/storage/FilesystemInfo_macos.cpp)
else()
    target_sources(dw_core PRIVATE src/core/storage/FilesystemInfo_linux.cpp)
endif()
```

### Confidence

HIGH — Linux magic numbers from kernel ABI (stable for decades).
HIGH — macOS `f_fstypename` approach recommended by Apple Developer Forums as reliable.
HIGH — Windows `GetDriveTypeW` from Microsoft Learn (updated 2024).

---

## 4. Content-Addressable Storage

### Decision: Git-style two-level prefix directory + atomic rename — std::filesystem only

No new dependency. C++17 `std::filesystem` plus the existing SHA-256 hash infrastructure
(already used for import deduplication in v1.0).

### Directory Structure

```
~/.local/share/DigitalWorkshop/cas/    (Linux default)
%APPDATA%\DigitalWorkshop\cas\         (Windows)
~/Library/Application Support/DigitalWorkshop/cas/   (macOS)
    ab/
        cdef1234ef567890...   (SHA-256 hex with first 2 chars stripped)
    fe/
        9a820b7c...
    tmp/
        import_<hash>         (in-progress staging area)
```

Two-character hex prefix = 256 subdirectories. At 3,000 models: ~12 files per bucket.
At 100,000 models: ~390 per bucket. Git uses this exact structure — proven at scale.

### Core Storage Operation

```cpp
// src/core/storage/ContentStore.cpp (relevant excerpt)

// Returns the SHA-256 hash if the file was stored (or already existed).
// Returns nullopt on error.
std::optional<std::string> ContentStore::store(
    const std::filesystem::path& sourceFile,
    bool removeSource)
{
    namespace fs = std::filesystem;

    // 1. Hash the source
    std::string hash = sha256File(sourceFile);  // existing utility
    if (hash.empty()) return std::nullopt;

    // 2. Compute final destination
    auto destDir = casRoot_ / hash.substr(0, 2);
    auto dest    = destDir  / hash.substr(2);

    // 3. Deduplication: already stored?
    if (fs::exists(dest)) {
        if (removeSource) fs::remove(sourceFile);
        return hash;
    }

    // 4. Stage in tmp/ (must be on same filesystem as casRoot_ for atomic rename)
    auto tmpDir  = casRoot_ / "tmp";
    fs::create_directories(tmpDir);
    auto tmpPath = tmpDir / ("import_" + hash);

    // Copy to tmp first (handles cross-filesystem sources, e.g. NAS)
    fs::copy_file(sourceFile, tmpPath, fs::copy_options::overwrite_existing);

    // 5. Atomic rename into final position
    fs::create_directories(destDir);
    fs::rename(tmpPath, dest);   // atomic on POSIX when same mount; no-op if already there

    // 6. Optionally remove source (only if it was local — never delete NAS originals)
    if (removeSource && !isNetworkPath(sourceFile))
        fs::remove(sourceFile);

    return hash;
}
```

**Key invariant:** `tmp/` is a subdirectory of `casRoot_`, so `rename()` is always
on the same filesystem mount. If `tmp/` were in the system `/tmp`, the rename would
fall back to copy+delete on many systems (not atomic).

**Copy vs Move policy (driven by `detectFilesystem()`):**

| Source Location | Action |
|----------------|--------|
| Local, same filesystem as CAS | Copy to `tmp/`, atomic rename to CAS, delete original |
| Local, different filesystem | Copy to `tmp/`, atomic rename to CAS, delete original |
| Network (NAS) | Always copy to `tmp/`, atomic rename to CAS, never delete original |

The user controls a "keep originals" preference; the mechanics above are transparent.

### Confidence

HIGH — pattern mirrors Git object store directly.
HIGH — `std::filesystem::rename()` atomicity on same-mount is guaranteed by POSIX.
MEDIUM — Windows `rename()` atomicity: `MoveFileEx` with `MOVEFILE_REPLACE_EXISTING` is
atomic within the same volume. `std::filesystem::rename` on MSVC calls this internally.

---

## 5. ZIP Archive Creation (.dwproj Export)

### Decision: Use existing miniz_static — no new dependency

`miniz_static` (miniz 3.0.2) is already fetched and compiled in the project.
The `.dwproj` format is a ZIP archive with a different extension — identical to how
`.dwmat` archives work today (verified by reading `cmake/Dependencies.cmake`).

The existing `miniz_static` target already links all four required source files:
- `miniz.c` — zlib deflate/inflate
- `miniz_tdef.c` — deflate compressor
- `miniz_tinfl.c` — inflate decompressor
- `miniz_zip.c` — ZIP reader/writer (`mz_zip_writer_*` / `mz_zip_reader_*`)

### Archive Creation Pattern

```cpp
#include "miniz.h"

// Creates a .dwproj ZIP at outputPath.
// manifest.json + referenced CAS files are embedded.
bool ProjectExporter::exportDwproj(
    const std::filesystem::path& outputPath,
    const ProjectManifest& manifest,
    const std::filesystem::path& casRoot)
{
    mz_zip_archive zip{};
    mz_zip_zero_struct(&zip);

    if (!mz_zip_writer_init_file(&zip, outputPath.string().c_str(), 0))
        return false;

    // 1. Write manifest JSON
    std::string json = manifest.toJson();
    mz_zip_writer_add_mem(&zip, "manifest.json",
        json.data(), json.size(), MZ_DEFAULT_COMPRESSION);

    // 2. Write each referenced model file from CAS
    for (const auto& entry : manifest.modelEntries) {
        auto casPath = casRoot / entry.hash.substr(0, 2) / entry.hash.substr(2);
        // Archive entry name: "models/<original_filename>"
        std::string archiveName = "models/" + entry.filename;
        mz_zip_writer_add_file(&zip,
            archiveName.c_str(),
            casPath.string().c_str(),
            /*pComment=*/nullptr, /*comment_size=*/0,
            MZ_DEFAULT_COMPRESSION);
    }

    // 3. Write thumbnails (optional)
    for (const auto& entry : manifest.thumbnailEntries) {
        mz_zip_writer_add_file(&zip, entry.archivePath.c_str(),
            entry.localPath.string().c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION);
    }

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    return true;
}
```

**Reading .dwproj back:**

```cpp
mz_zip_archive zip{};
mz_zip_zero_struct(&zip);
mz_zip_reader_init_file(&zip, projectPath.string().c_str(), 0);
// extract manifest.json, model files, thumbnails...
mz_zip_reader_end(&zip);
```

**No CMake changes required.** `miniz_static` is already linked to `dw_core` (or whichever
core target needs it) — just add `#include "miniz.h"` in the new exporter source.

### Confidence

HIGH — verified against existing `cmake/Dependencies.cmake` (miniz_static already built
with all four `.c` files). API usage pattern from miniz 3.0.2 official examples.

---

## Dependency Delta Table

| Capability | Library | Version | License | Binary Size Added | CMake Change |
|------------|---------|---------|---------|-------------------|--------------|
| Kuzu graph DB | libkuzu (prebuilt) | 0.11.3 | MIT | +8.5 MB (Linux `.so`) | FetchContent URL + IMPORTED target |
| FTS5 search | (SQLite already in build) | 3.45.0 | Public Domain | 0 | 1 `target_compile_definitions` line |
| Filesystem detection | (POSIX/Win32 syscalls) | — | — | 0 | Platform `target_sources` |
| CAS storage | (std::filesystem) | C++17 | — | 0 | New `.cpp` source files only |
| .dwproj export | (miniz_static already in build) | 3.0.2 | MIT | 0 | New `.cpp` source files only |

**Total new binary size impact:** ~8.5 MB (Linux), ~13 MB (Windows), ~10 MB (macOS).
All from Kuzu's shared library, shipped alongside the main binary.

---

## Critical Warnings

### Kuzu: Never include `kuzu.hpp`
`kuzu.hpp` is the C++ wrapper and requires C++20. Including it in any project `.cpp` file
will fail to compile with `-std=c++17`. Use only `kuzu.h`.

### Kuzu: One `kuzu_database` per process
Opening two `kuzu_database` instances pointing to the same directory path causes undefined
behavior (per Kuzu documentation). The singleton pattern in `KuzuDatabase` is mandatory.

### FTS5: Triggers are mandatory with `content=` option
When `content='models'` is set, FTS5 stores only the inverted index, not the text. Missing
any of the three sync triggers (INSERT, DELETE, UPDATE) silently produces stale or wrong
search results. All three triggers must be created in the same transaction as the FTS5 table.

### FTS5: Rebuild needed on schema change
Per the "no migrations" rule, the database is deleted and recreated on schema change.
Include `INSERT INTO models_fts(models_fts) VALUES('rebuild');` in the initial setup
to populate FTS from existing rows.

### CAS: `tmp/` must be on the same mount as CAS root
If `tmp/` is placed in the system `/tmp` directory, `std::filesystem::rename()` across
filesystems degrades to non-atomic copy+delete. Always use `casRoot_ / "tmp"`.

### Kuzu DLL deployment (Windows)
`kuzu_shared.dll` must be in the same directory as the `.exe` or on `PATH`. The NSIS
installer must be updated to copy this file. The existing installer handles `SDL2.dll`
already — follow that pattern.

---

## Sources

- Kuzu latest release (GitHub API, verified 2026-02-21):
  https://github.com/kuzudb/kuzu/releases/tag/v0.11.3
- Kuzu CMakeLists.txt C++ standard requirement (`cmake_minimum_required VERSION 3.11`,
  `CXX_STANDARD 20`): https://github.com/kuzudb/kuzu/blob/master/CMakeLists.txt
- Kuzu C API header (pure C, extern "C" guards, C++17-safe):
  https://github.com/kuzudb/kuzu/blob/master/src/include/c_api/kuzu.h
- Kuzu concurrency / thread safety:
  https://docs.kuzudb.com/concurrency/
- SQLite FTS5 extension (content tables, triggers, BM25):
  https://sqlite.org/fts5.html
- SQLite compile-time options (`SQLITE_ENABLE_FTS5`):
  https://www.sqlite.org/compile.html
- Linux `statfs(2)` and filesystem magic numbers:
  https://man7.org/linux/man-pages/man2/statfs.2.html
- macOS `f_fstypename` reliability over `f_type`:
  https://developer.apple.com/forums/thread/87745
- Windows `GetDriveTypeW` (DRIVE_REMOTE = 4):
  https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypew
- Hashed directory structure (2-char prefix, 256 buckets):
  https://medium.com/eonian-technologies/file-name-hashing-creating-a-hashed-directory-structure-eabb03aa4091
- miniz 3.0.2 ZIP API (already in project at cmake/Dependencies.cmake):
  https://github.com/richgel999/miniz
