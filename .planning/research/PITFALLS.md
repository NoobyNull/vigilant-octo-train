# Domain Pitfalls: v1.1 Library Storage & Organization

**Domain:** Adding content-addressable storage, graph DB, FTS5, category hierarchy, and project export to existing C++17 desktop app
**Researched:** 2026-02-21
**Overall Confidence:** HIGH for most areas; see individual notes

---

## STOP: Critical Pre-Decision Finding

### Kuzu (kuzudb) Was Abandoned in October 2025

**This finding overrides the stack plan.** Kuzu Inc. archived the `kuzudb/kuzu` GitHub repository on October 10, 2025 — read-only, no warning, no migration path. The `docs.kuzudb.com` domain is unreachable. The last stable release was v0.11.3 (October 10, 2025). A community fork (`bighorn` by Kineviz) exists but has "fewer than 10 developers who actually understand the codebase."

**Consequences for v1.1:**
- FetchContent will pull abandoned code — no security fixes, no C++ toolchain compatibility updates
- v0.11.0 introduced a breaking file format change (single-file database replacing directory structure), meaning any data stored before pinning would be unreadable after upgrade
- The project is listed as requiring C++20 (not C++17), which conflicts with this project's compiler constraint
- Binary footprint is large (the released precompiled library is tens of MB — far over this project's 8-25 MB budget)

**Decision required before any implementation begins:**
Option A — Use SQLite recursive CTEs + adjacency list for the relationship queries (project-model links, category hierarchy). This is fully sufficient for the query patterns described in PROJECT.md. **Recommended.**

Option B — Adopt the `bighorn` fork (Kineviz), pin to a specific commit, accept maintenance risk. Not recommended for a solo project with no DB team.

Option C — Adopt FalkorDB or another graph DB. None are embedded C++ with permissive licenses and stable C++17 APIs.

**Recommendation: Do not use Kuzu. Use SQLite recursive CTEs.** Graph queries in PROJECT.md (project links as edges, category hierarchy) are shallow trees and many-to-many joins — exactly what SQLite CTEs handle cleanly at 3000-model scale.

**Sources (HIGH confidence):**
- [The Register: KuzuDB Abandoned](https://www.theregister.com/2025/10/14/kuzudb_abandoned/)
- [Kineviz/bighorn fork](https://github.com/Kineviz/bighorn)
- [FalkorDB migration guide](https://www.falkordb.com/blog/kuzudb-to-falkordb-migration/)
- [Kuzu GitHub — archived read-only](https://github.com/kuzudb/kuzu)

---

## Critical Pitfalls

### Pitfall 1: Partial Write Corrupts the Content-Addressable Store

**What goes wrong:**
A 3D model file is written to `store/ab/cdef...` and then the app crashes, loses power, or is killed. The file is incomplete but its hash path now exists. On the next import of the same file, the deduplication logic sees the path exists, skips the copy, and returns success — but the stored file is truncated or zero-length. The model can never be loaded correctly.

**Why it happens:**
The naive implementation writes directly to the final destination path. `std::filesystem::copy_file` and custom streaming writes are not atomic — a crash mid-write leaves a partial file. This is especially dangerous when importing from a NAS where a copy of a 200MB model can take several seconds.

**Consequences:**
Silent data corruption. The database says the file is present and valid. The model fails to load with a confusing mesh error. The user cannot re-import because the hash deduplication rejects it as "already present." Recovery requires manual store inspection.

**Prevention:**
1. **Write to a temp file first, rename last.** Write to `store/.tmp/<uuid>`, then `std::filesystem::rename()` to the final hash path. Rename is atomic on POSIX (same filesystem). On Windows, use `MoveFileExW` with `MOVEFILE_REPLACE_EXISTING` — `std::filesystem::rename` is NOT atomic on Windows when the destination exists.
2. **Verify before accepting.** After write completes but before rename, hash the temp file and compare with expected hash. Reject if mismatch.
3. **On startup, scan for and remove `.tmp/` orphans** from previous crashed sessions.
4. **Check file size after copy.** Zero-length file in store is always an error.

**Warning signs:**
- Models that "imported successfully" but cannot be loaded
- Hash path exists on disk but file is 0 bytes or smaller than expected
- User cannot re-import a file they know is correct

**Phase to address:** CAS foundation phase — must be solved before any import wiring.

**Sources:**
- [borgbackup issue #170: Hash collision detection](https://github.com/borgbackup/borg/issues/170) — shows industry practice of verify-then-accept
- General POSIX rename atomicity is well-established; Windows behavior documented in Win32 docs

---

### Pitfall 2: FTS5 Index Silently Drifts from Content Table

**What goes wrong:**
The FTS5 virtual table is backed by a content table (the `models` table). Using `content=models` with UPDATE triggers is the standard approach, but there is a critical trigger ordering bug: **UPDATE triggers for FTS5 must be BEFORE UPDATE, not AFTER UPDATE.**

If AFTER UPDATE is used, FTS5 fetches the content to build the delete entry *after* the new values are already committed. It deletes tokens for the NEW content instead of the OLD content. Old tokens are never removed. After several thousand updates, the index has stale ghost tokens that return false matches and cause phantom search results.

Additionally, if any code path updates the `models` table directly (bypassing the ORM/repository layer), the triggers still fire — but if triggers are missing from that code path, the index silently goes stale.

**Why it happens:**
The SQLite documentation shows the correct trigger pattern in one section and wrong patterns in examples elsewhere. Stack Overflow answers frequently show the AFTER UPDATE variant which corrupts the index slowly.

**Consequences:**
Search returns models that were renamed/deleted. Searching for a tag that was removed from a model still returns that model. The drift accumulates silently — no errors are thrown.

**Prevention:**
1. **Use BEFORE UPDATE triggers, not AFTER UPDATE.**
   ```sql
   -- Correct FTS5 trigger for update
   CREATE TRIGGER models_fts_upd BEFORE UPDATE ON models BEGIN
     INSERT INTO models_fts(models_fts, rowid, name, tags, description)
       VALUES('delete', old.id, old.name, old.tags, old.description);
   END;
   CREATE TRIGGER models_fts_upd_post AFTER UPDATE ON models BEGIN
     INSERT INTO models_fts(rowid, name, tags, description)
       VALUES(new.id, new.name, new.tags, new.description);
   END;
   ```
2. **Add an integrity check function.** Periodically run `INSERT INTO models_fts(models_fts) VALUES('integrity-check')` and verify no errors.
3. **Provide a "rebuild FTS index" command** in settings — a full `INSERT INTO models_fts(models_fts) VALUES('rebuild')` that resynchronizes everything. This is the recovery path.
4. **Route ALL writes through a single repository class.** Never update `models` directly from two places.

**Warning signs:**
- Search returns a model that was deleted or renamed
- Searching by old tag name returns results
- `integrity-check` command returns errors

**Phase to address:** FTS5 integration phase — trigger correctness must be verified with tests.

**Sources:**
- [SQLite Forum: Corrupt FTS5 table after wrong trigger pattern](https://sqlite.org/forum/info/da59bf102d7a7951740bd01c4942b1119512a86bfa1b11d4f762056c8eb7fc4e)
- [SQLite FTS5 Extension official docs](https://sqlite.org/fts5.html)
- [SQLite Forum: FTS5 rows not searchable with match](https://sqlite.org/forum/info/db41a553368f4d4a)

---

### Pitfall 3: NAS File Operations Are Not Atomic and Lock Unreliably

**What goes wrong:**
When importing from a 10G NAS share, the application reads a file, hashes it (may take seconds for large STL files), then copies it to the local store. Between the initial read and the copy, the NAS file may be modified, moved, or deleted by another process or user. The hash will not match the copied content. The copy may be partial if the connection drops mid-copy.

Additionally, `flock()` / `LockFileEx()` advisory locks do not work reliably across NFS or SMB. A lock that appears to succeed may not actually prevent another client from writing to the file.

For the "auto-detect copy vs move on import" feature: detecting whether the source and destination are on the same filesystem uses `std::filesystem::equivalent()` — this will throw or return false incorrectly for NAS mounts, causing the app to always copy instead of move, which is actually the CORRECT behavior but may confuse logic that assumes local-means-same-device.

**Why it happens:**
Local filesystem assumptions (atomic rename, reliable locks, stable inodes) do not hold over network mounts. NFS particularly has notoriously unreliable locking (NLM for NFSv3, integrated in NFSv4 but still dependent on server implementation).

**Consequences:**
- Corrupted file in store (partial copy from interrupted NAS read)
- Wrong file stored under a hash (NAS file modified between hash and copy)
- Import reports success but stored content is wrong

**Prevention:**
1. **Never lock NAS files.** Do not attempt advisory locks on remote paths — they will silently fail or behave unpredictably.
2. **Hash AFTER copy, verify content.** Copy the file to a temp location first, hash the temp file, compare with the hash computed from the original. If they differ, the source changed mid-copy — abort and report error.
3. **For NAS source paths, always copy — never move.** The "same filesystem" heuristic is unreliable for network mounts. Explicit user configuration ("this path is a NAS — always copy") is safer.
4. **Detect unreachable mount gracefully.** Use a timeout-wrapped stat() before attempting multi-minute operations. If the mount is unresponsive, report "Network path unavailable" immediately rather than hanging.
5. **Treat NAS import errors as recoverable, not fatal.** One failed file should not abort the entire 3000-model import job.

**Warning signs:**
- Import succeeds but model fails to load (hash mismatch caught at load time)
- Import hangs indefinitely (NAS mount dropped mid-copy)
- Duplicate models after reconnecting NAS (import was retried, deduplication failed)

**Phase to address:** NAS detection and import strategy phase. Must be designed before the import pipeline is modified.

**Sources:**
- [Samba docs: NFS and file locking](https://www.samba.org/samba/docs/old/Samba3-HOWTO/locking.html)
- [NFS and file locking (O'Reilly NFS book)](https://docstore.mik.ua/orelly/networking_2ndEd/nfs/ch11_02.htm)

---

### Pitfall 4: Windows Long Paths Break the Hash Store Directory Structure

**What goes wrong:**
The CAS directory structure uses the first two hex characters as a subdirectory: `store/ab/cdef1234...`. On Windows, paths are limited to 260 characters (`MAX_PATH`) by default. If the application is installed in a long path (e.g., `C:\Users\username\AppData\Local\Programs\DigitalWorkshop\`) and the hash store is inside it, the total path to any stored file may exceed 260 characters.

`std::filesystem` on MSVC does NOT automatically handle long paths. Operations will silently fail or throw with `ERROR_PATH_NOT_FOUND` which is difficult to distinguish from "file doesn't exist."

**Why it happens:**
Linux/macOS have no practical path length limit. Developers building primarily on Linux miss this failure mode entirely. The path `C:\Users\longusername\AppData\Local\DigitalWorkshop\store\ab\cdef1234567890abcdef1234567890abcdef12` is 95 characters just for the hash path component — add typical Windows user directory lengths and it approaches 200+ characters easily.

**Consequences:**
- `std::filesystem::create_directories()` fails silently or throws on store creation
- `std::filesystem::copy_file()` fails mid-import with cryptic error
- CI on Windows passes (short CI paths) but fails for real users with long usernames

**Prevention:**
1. **Add `longPathAware` to the Windows application manifest.** This enables long path support on Windows 10 1607+ when the registry key `LongPathsEnabled` is set.
   ```xml
   <application xmlns="urn:schemas-microsoft-com:asm.v3">
     <windowsSettings>
       <longPathAware xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">true</longPathAware>
     </windowsSettings>
   </application>
   ```
2. **Use `\\?\` prefix for Win32 API calls** when `longPathAware` is not guaranteed. `std::filesystem` paths on MSVC can be prefixed with `\\?\` to bypass MAX_PATH.
3. **Test on Windows CI with a long working directory path.** Add a CI step that runs from `C:\a-very-long-path-name-to-simulate-user-directories\build`.
4. **Keep the hash store root path as short as possible** — put it in `AppData\Local\DW\store\` not inside the install directory.

**Warning signs:**
- Windows-only import failures
- `std::filesystem::create_directories()` returns false without throwing on Windows
- Tests pass in CI, fail on user machines

**Phase to address:** CAS foundation — manifest update and path handling must be done before Windows users can use the store.

**Sources:**
- [Microsoft Learn: Maximum Path Length Limitation](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation)
- [Microsoft Learn: Enable long path names in Windows 11](https://learn.microsoft.com/en-us/answers/questions/1805411/how-to-enable-long-file-path-names-in-windows-11)

---

## Moderate Pitfalls

### Pitfall 5: Orphan Files Accumulate in the Store Without Cleanup

**What goes wrong:**
The content-addressable store decouples physical files from database records. If a model is deleted from the database but the cleanup code has any bug (race condition, exception, failed transaction), the file remains in the store indefinitely. With 3000+ initial models, even a 1% orphan rate means 30+ files that consume disk space silently.

The inverse is also possible: the database record exists but the file was manually deleted from the store. This is the more dangerous case — the model appears in the library but cannot be loaded.

**Why it happens:**
Deletion is a two-phase operation (delete DB record + delete file). If they are not in the same transaction, any crash or error leaves them inconsistent. Since file deletion cannot be part of a SQLite transaction, there is always a window.

**Prevention:**
1. **Delete DB record first, file second** (not vice versa). A record with no file is a load error; a file with no record is just wasted disk space — the latter is recoverable.
2. **Implement a periodic orphan sweep** that finds files in the store with no corresponding DB record. Run on startup, log orphans found.
3. **Implement a store integrity check** that finds DB records where the file does not exist. These are broken records — mark them with a `missing` flag rather than silently failing.
4. **Reference count files in the DB** if multiple models can share the same file (same content, different metadata). The CAS should only delete a file when its reference count reaches zero.

**Warning signs:**
- Store directory grows faster than model count suggests
- Models appear in library but fail to load with "file not found"
- Disk usage is unexpectedly high after bulk deletions

**Phase to address:** CAS foundation + deletion flow. Build integrity check before shipping.

---

### Pitfall 6: Hash Store File Count Causes Directory Enumeration Performance Issues

**What goes wrong:**
With 3000 models, the two-level hash directory (`store/ab/cdef...`) distributes files into 256 top-level buckets. With uniform distribution, each bucket has ~12 files — no problem at all. However, the thumbnails are stored separately (not in the CAS), and temporary files pile up in `.tmp/`. The problem emerges if any code accidentally puts all files in one directory (misconfigured prefix length, bug in path construction), or if the thumbnail cache grows large.

**Why it happens:**
A bug in hex prefix extraction — for example, taking characters [0:2] as index instead of the actual hex string — could put all files under `store/00/` or `store/st/`. This is caught immediately if tested but could slip through if path generation is not unit-tested.

**Prevention:**
1. **Unit test the path generation function** with a known SHA-256 hash and assert the correct directory structure.
2. **Assert that directory prefix is two valid hex characters** before creating the directory.
3. **At 3000 models, the 256-bucket two-level scheme is more than sufficient.** Do not add a third level — it is unnecessary complexity at this scale.

**Warning signs:**
- One bucket directory has far more files than others (check with `ls -la store/ | wc -l`)
- Directory enumeration is slow during startup integrity check

**Phase to address:** CAS foundation — unit test path generation immediately.

---

### Pitfall 7: FTS5 unicode61 Tokenizer Strips Diacritics from Filenames

**What goes wrong:**
The `unicode61` tokenizer (FTS5 default) strips diacritics by default — "Möbius" becomes "Mobius" for indexing. For model names derived from NAS filenames, this is usually fine. However, if the user searches for "Möbius" expecting an exact match, they get a hit (stripped to "Mobius") which is actually correct behavior. The problem is the reverse: the user searches for "Mobius" and the search also returns "Möbel" (stripped to "Mobel") — different word, false positive.

Additionally, `porter` stemmer is English-only. If users have German, French, or other language model names, porter stemming will corrupt the token (it will apply English suffixing rules to non-English words, destroying the terms).

**Why it happens:**
Default FTS5 configuration is optimized for English text. A model library with international filenames (common when importing from third-party NAS shares) will have non-English names.

**Prevention:**
1. **Use `unicode61` tokenizer without porter wrapping** for model name search. This gives case-insensitive, diacritic-folding search without aggressive English stemming.
   ```sql
   CREATE VIRTUAL TABLE models_fts USING fts5(
     name, tags, description,
     content=models,
     tokenize='unicode61'
   );
   ```
2. **Do not use porter stemmer** for filename-derived model names. Porter is suitable only for English prose description fields.
3. **Test with non-ASCII model names** — import a model with a Unicode filename and verify it is searchable.

**Warning signs:**
- Non-ASCII model names are not found by search
- Searching by a model name returns unrelated models with similar-sounding names

**Phase to address:** FTS5 integration phase — tokenizer choice is a schema decision, hard to change after data is indexed.

**Sources:**
- [SQLite FTS5 Tokenizers: unicode61 and ascii (Jan 2025)](https://audrey.feldroy.com/articles/2025-01-13-SQLite-FTS5-Tokenizers-unicode61-and-ascii)
- [SQLite FTS5 Extension official docs](https://sqlite.org/fts5.html)

---

### Pitfall 8: WAL Mode + Attached Database Cross-DB Atomicity Breaks

**What goes wrong:**
The existing codebase uses WAL mode on the SQLite database. If the v1.1 implementation attaches a second SQLite database (for example, a separate graph/category database), SQLite ATTACH makes transactions *appear* atomic but they are NOT when WAL mode is active on either database.

From the SQLite documentation: "If the main database is not ':memory:' and the journal_mode is not WAL, then transactions ... are atomic. But if the host computer crashes in the middle of a COMMIT where two or more database files are updated, some of those files might get the changes where others might not."

Since the main DB is in WAL mode, any two-database transaction is not crash-safe.

**Why it happens:**
Developers ATTACH a second SQLite database and assume `BEGIN; UPDATE db1.table SET ...; UPDATE db2.table SET ...; COMMIT;` is safe. SQLite documentation describes this limitation but it is easy to miss.

**Prevention:**
1. **Do not use ATTACH for a second database in WAL mode.** If graph/category data moves to a separate SQLite file, accept that the two databases can drift on crash, and implement reconciliation on startup.
2. **Preferred: Keep all data in one SQLite database.** Category hierarchy and project-model graph edges fit naturally in the existing schema as adjacency list tables. One database = one WAL = atomic commits.
3. **If two DBs are required:** Design for eventual consistency — make each DB individually consistent, detect drift on startup, and provide a "repair" command.

**Warning signs:**
- App crashes during import, leaves category tree in different state than model table
- Project-model associations missing after unexpected shutdown
- ATTACH is used anywhere in a WAL-mode application

**Phase to address:** Schema design phase — must be resolved before writing any cross-DB transactions.

**Sources:**
- [SQLite Forum: Transactions involving multiple databases](https://sqlite.org/forum/forumpost/ecf53d4eb8)
- [How To Corrupt An SQLite Database File — SQLite Official](https://www.sqlite.org/howtocorrupt.html)

---

### Pitfall 9: Migrating Existing file_path Records to CAS Store

**What goes wrong:**
The existing `models` table has a `file_path` column pointing to original locations (NAS paths, local paths). v1.1 introduces the CAS store. During migration (which per the project rulebook is "delete and recreate DB on schema change"), all model files must be physically copied into the store. For 3000 models on a NAS, this migration could take 30-60 minutes.

If the rulebook "delete and recreate DB" applies, all model metadata (tags, thumbnails, associations) is lost unless the migration script copies everything before dropping the schema.

**Why it happens:**
The project rulebook says "no migrations — delete and recreate DB." This is safe when only schema changes. But v1.1 adds physical file relocation — the schema change must be accompanied by a data movement operation, which is a migration in all but name.

**Consequences:**
- If DB is dropped before copying files, and the copy fails mid-way, the user loses all their library metadata
- If NAS is unavailable during "migration", all models that were NAS-sourced cannot be re-imported
- Thumbnails are regenerated from mesh data, so they can be recovered — but metadata (tags, categories, custom names) cannot

**Prevention:**
1. **Run file copy BEFORE dropping the old schema.** The sequence must be: (a) copy all referenced files to store, (b) write new schema, (c) re-import metadata from old records, (d) drop old schema. Never drop the old schema first.
2. **Make the migration resumable.** If the copy is interrupted (NAS drops out), the user should be able to restart and it picks up where it left off. Use the CAS's own deduplication to skip already-copied files.
3. **Export a backup of metadata before any schema operation.** Write existing model records (name, tags, file_path, original_filename) to a CSV/JSON before dropping tables.
4. **Log every file that could not be migrated** (NAS unavailable, file moved). Present a "migration report" to the user showing what succeeded and what needs manual attention.
5. **Keep the migration out of the normal startup path.** The first run after a schema version bump triggers migration, but normal startup should not try to migrate — only verify.

**Warning signs:**
- User upgrades, app crashes during import, all metadata gone
- 30-minute startup on first run with no progress indicator (appears frozen)
- Models appear in library with no thumbnail and no tags after upgrade

**Phase to address:** Migration design must happen BEFORE any schema changes are made. It is the highest-risk operation in the milestone.

---

### Pitfall 10: Unicode Filename Encoding Mismatch Across Platforms

**What goes wrong:**
Windows filenames are UTF-16. macOS enforces UTF-8 NFC normalization. Linux makes no guarantees (can be any encoding, including ISO-8859-1). A NAS shared over SMB to all three platforms may have filenames that appear identical on screen but have different byte sequences (NFC vs NFD Unicode normalization, different encoding entirely).

When the CAS stores a file, the filename is irrelevant (only the hash matters). But the original filename must be preserved in the database for display. If the original filename is stored as raw bytes from the OS, then compared across platforms, the "same" model from a NAS share may appear twice with different raw names but identical content (duplicate detection works because hash matches, but display name is inconsistent).

**Why it happens:**
`std::filesystem::path::filename()` returns a path in the platform's native encoding. On Windows with MSVC, this is `std::wstring` internally. Converting to `std::string` uses the system code page, which may not be UTF-8 on all Windows installations.

**Prevention:**
1. **Always convert filenames to UTF-8 before storing in the database.** On Windows, use `path.wstring()` then `WideCharToMultiByte(CP_UTF8, ...)`.
2. **Normalize to NFC before storing.** Apply Unicode NFC normalization to all display names. This prevents the same filename appearing differently on macOS (NFD) vs Windows (NFC).
3. **The stored filename in the database is cosmetic only.** The CAS path is determined by content hash. The display name can be renamed freely by the user.
4. **Test with filenames that contain Chinese, Arabic, or emoji characters** — these expose normalization and encoding bugs immediately.

**Warning signs:**
- The same physical file appears twice in the library with slightly different names
- Model names appear as `?????` on one platform
- Import fails with "invalid path" for non-ASCII filenames on Windows

**Phase to address:** CAS foundation — filename handling must be correct from the start.

**Sources:**
- [Cross-Platform Unicode Support in C++](https://www.studyplan.dev/pro-cpp/character-encoding/q/cross-platform-unicode)

---

### Pitfall 11: Case-Sensitive vs Case-Insensitive Filesystem Mismatch

**What goes wrong:**
The CAS path is determined by hex hash (`ab/cdef1234...`). Hex is always lowercase — no case sensitivity issue there. However, the category hierarchy and model names stored in the database may have case sensitivity issues when compared across platforms. A model tagged `"Oak"` on macOS (case-insensitive filesystem) may conflict with `"oak"` on Linux (case-sensitive).

More critically: if any code path ever constructs a CAS path from user input (rather than from a computed hash), case mismatch on Linux will cause file-not-found errors that Windows and macOS hide silently.

**Why it happens:**
macOS APFS is case-insensitive by default. HFS+ is case-insensitive. Windows NTFS is case-insensitive. Linux ext4 is case-sensitive. Code that works perfectly on macOS/Windows may silently create wrong paths on Linux.

**Prevention:**
1. **Always use lowercase hex for CAS paths** — SHA-256 hex output is lowercase by convention and must be kept that way.
2. **Never construct CAS paths from user input.** Paths are always derived from hash computation only.
3. **Normalize category names and tags to lowercase** before storing, or enforce case-insensitive comparisons with `COLLATE NOCASE` in SQLite queries.
4. **Run tests on Linux** for all path-construction code.

**Warning signs:**
- Works on Windows/macOS, path errors on Linux
- Duplicate categories after cross-platform sync ("Oak" and "oak" as separate categories)

**Phase to address:** CAS foundation — hash-to-path function must be tested on all three platforms.

---

## Minor Pitfalls

### Pitfall 12: FTS5 Index Bloat with Large Tag Sets

**What goes wrong:**
FTS5 indexes every word in every indexed column. Tags are stored as a single concatenated string (`"oak hardwood furniture chair"`) and indexed token by token. If models accumulate many tags over time, the FTS5 index grows faster than the content table. For 3000 models with 10 tags each, this is negligible (~few MB). However, the `OPTIMIZE` command is needed periodically to merge the FTS5 B-tree segments — without it, read performance degrades as the number of segments grows.

**Prevention:**
1. Run `INSERT INTO models_fts(models_fts) VALUES('optimize')` periodically — on startup if last optimization was more than a week ago.
2. Do NOT run `optimize` after every tag update — it is expensive for frequent writes. Batch it.
3. The `automerge` FTS5 parameter (default 8) handles routine merging. Only manual `optimize` is needed for large-scale cleanup.

**Warning signs:**
- Search latency grows over weeks of use without increasing model count
- FTS5 tables are significantly larger than the content table

**Phase to address:** FTS5 integration — set automerge config at table creation time.

---

### Pitfall 13: Disk Space Accounting Is Non-Obvious with CAS

**What goes wrong:**
In the CAS model, the user has no visible filenames in the store directory — just `ab/cdef1234...` paths. When they ask "how much space are my models using?", the answer requires iterating the store and summing file sizes. If the deduplication is working correctly, two models with identical content share one stored file — so the "actual disk usage" may be significantly less than "total import size." Users may be confused when the library shows 5GB of models but the store is only 3GB.

**Prevention:**
1. **Show both "logical size" (sum of all model sizes, counting duplicates) and "stored size" (actual disk usage) in settings/library stats.**
2. **Compute stored size lazily** — walk the store directory on demand, not on every frame.
3. **Cache the stored size** and invalidate on import or deletion.

**Phase to address:** UI phase — purely informational, but important for user trust.

---

### Pitfall 14: Symlink Handling in the Store

**What goes wrong:**
If a user's library path or NAS path contains symlinks, `std::filesystem::copy_file` by default follows symlinks (copies the target file, not the link). This is usually correct. However, if the CAS store directory itself is a symlink (user put it on a different drive), operations like `std::filesystem::is_directory()` on the store root may behave differently across platforms. On Windows, symlinks require elevated privileges to create, so user-created symlinks to the store are rare but possible.

**Prevention:**
1. **Resolve symlinks before all CAS operations** using `std::filesystem::canonical()` on the store root during initialization.
2. **Do not follow symlinks when enumerating the store for integrity checks** — use `std::filesystem::directory_iterator` with `follow_directory_symlink = false` to avoid infinite loops.
3. **Warn users if the store root is a symlink** — log it clearly at startup.

**Phase to address:** CAS foundation — store initialization must handle this.

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Kuzu integration | Project abandoned October 2025 | Use SQLite recursive CTEs instead — sufficient for all described relationship queries |
| CAS store creation | Partial write on crash (Pitfall 1) | Write-to-temp + rename pattern; verify hash after copy |
| CAS first run migration | Loss of metadata if DB dropped first (Pitfall 9) | Copy files THEN drop schema; build resumable migration |
| FTS5 trigger setup | AFTER UPDATE corrupts index (Pitfall 2) | Use BEFORE UPDATE for delete half of update trigger |
| FTS5 tokenizer choice | Porter breaks non-English names (Pitfall 7) | Use unicode61 only; no porter wrapper for filenames |
| NAS import | Partial reads, unreliable locks (Pitfall 3) | Hash after copy, never lock NAS files, handle mount failures |
| Windows deployment | MAX_PATH exceeded in hash store (Pitfall 4) | longPathAware manifest + test with long CI paths |
| Cross-DB transactions | WAL mode breaks ATTACH atomicity (Pitfall 8) | Keep all data in one SQLite database |
| Category hierarchy | Graph DB required for simple tree? | No — SQLite recursive CTEs handle category trees cleanly |
| Unicode filenames | Encoding differences across platforms (Pitfall 10) | Normalize to UTF-8 NFC before DB storage |
| Orphan cleanup | Store files accumulate without DB records (Pitfall 5) | Delete DB record first; run orphan sweep on startup |

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Kuzu abandonment | HIGH | Multiple independent sources confirm: The Register, GitHub archive status, community forums (October 2025) |
| Kuzu C++20 requirement | MEDIUM | Observed in build requirements; not verified against every release; the archived codebase may have worked with C++17 on older versions |
| CAS partial write | HIGH | Well-established filesystem behavior; POSIX rename atomicity is documented |
| FTS5 trigger ordering | HIGH | SQLite forum posts show confirmed corruption from AFTER UPDATE pattern; official docs corroborate |
| WAL + ATTACH atomicity | HIGH | Official SQLite documentation explicitly states this limitation |
| NAS file locking | HIGH | Well-documented behavior across NFS/SMB literature |
| Windows MAX_PATH | HIGH | Official Microsoft documentation; confirmed behavior in C++17 std::filesystem on MSVC |
| Migration risk | HIGH | Derived from project constraints (no migrations) + data loss analysis |
| Unicode filename handling | MEDIUM | General cross-platform knowledge; specific C++17 behavior on each OS verified by community sources |
| FTS5 tokenizer impact | MEDIUM | Official SQLite FTS5 docs confirm behavior; real-world impact on non-English filenames is inference |

---

## What NOT To Do (Anti-Patterns Specific to This Milestone)

| Anti-Pattern | Why Dangerous | Correct Approach |
|--------------|--------------|------------------|
| Write CAS file directly to final path | Partial write on crash leaves corrupted store entry | Write-to-temp, verify hash, rename |
| Use `AFTER UPDATE` trigger for FTS5 external content | Silently corrupts index by deleting wrong tokens | Use `BEFORE UPDATE` for delete phase |
| Use ATTACH for a second SQLite DB in WAL mode | Non-atomic cross-DB commits on crash | One database; adjacency list for graphs |
| Pull Kuzu from FetchContent | Repository archived, no fixes, C++20 requirement, large binary | SQLite recursive CTEs for graph relationships |
| Lock files on NAS before reading/copying | Advisory locks unreliable over NFS/SMB | Read-copy-verify pattern without locking |
| Drop schema before migrating files | All metadata lost if file copy fails | Copy files first, then drop schema |
| Store Windows filenames as raw system encoding | Cross-platform encoding mismatches | Convert to UTF-8 before DB storage |
| Trust `std::filesystem::equivalent()` for NAS detection | Returns wrong results for network mounts | Use explicit configuration for NAS paths |
| Use porter tokenizer for model name FTS | Corrupts non-English tokens | Use unicode61 only |
| Skip longPathAware manifest on Windows | Store paths exceed MAX_PATH for users with long usernames | Add manifest entry; test with long paths in CI |

---

## Open Questions Requiring Phase-Specific Research

1. **Graph relationships scope:** What specific graph queries does the project need? PROJECT.md mentions "project links as graph edges." SQLite CTEs can handle: "find all models in project X", "find all projects containing model Y", "find models related to model Z (shared project)". If deeper multi-hop queries are needed, reconsider — but current scope appears CTE-sufficient.

2. **FTS5 vs SQLite LIKE for 3000 models:** At 3000 models, SQLite LIKE queries with a trigram index may be sufficient and simpler than FTS5. FTS5 is justified if substring search, ranked results, or tag proximity search is required. Verify query requirements before committing to FTS5 trigger complexity.

3. **Thumbnail CAS:** Should thumbnails be stored in the CAS or separately? They are derived content (regenerable from mesh), not original content. Storing separately (by model ID, not hash) is simpler and avoids the CAS for content that changes independently.

4. **Project export format:** The `.dwproj` zip format needs specification. What happens if a project references a model that is in the CAS store? Does the export bundle the model file? If yes, the export must read from the CAS store, which requires the store path to be resolvable at export time.

5. **Category hierarchy depth:** How deep can categories be? Recursive CTEs in SQLite handle arbitrary depth, but the UI must have a practical limit. Establish this before designing the schema.

---

## Sources

### Kuzu Abandonment
- [The Register: KuzuDB Abandoned (October 2025)](https://www.theregister.com/2025/10/14/kuzudb_abandoned/)
- [LavX News: KuzuDB Abandoned by Corporate Sponsor](https://news.lavx.hu/article/kuzudb-abandoned-open-source-graph-database-cast-adrift-by-corporate-sponsor)
- [Kineviz/bighorn — community fork](https://github.com/Kineviz/bighorn)
- [FalkorDB: KuzuDB to FalkorDB Migration](https://www.falkordb.com/blog/kuzudb-to-falkordb-migration/)
- [Kuzu GitHub Releases — last release v0.11.3 Oct 10, 2025](https://github.com/kuzudb/kuzu/releases)

### Content-Addressable Storage
- [borgbackup Issue #170: Hash collision detection in practice](https://github.com/borgbackup/borg/issues/170)
- [borgbackup Issue #7765: Hash collisions — universal solution](https://github.com/borgbackup/borg/issues/7765)
- [Content-Addressable Storage overview](https://lab.abilian.com/Tech/Databases%20&%20Persistence/Content%20Addressable%20Storage%20(CAS)/)

### SQLite FTS5
- [SQLite FTS5 Extension — Official Documentation](https://sqlite.org/fts5.html)
- [SQLite Forum: Corrupt FTS5 table after wrong trigger pattern](https://sqlite.org/forum/info/da59bf102d7a7951740bd01c4942b1119512a86bfa1b11d4f762056c8eb7fc4e)
- [SQLite Forum: FTS5 rows not searchable with match](https://sqlite.org/forum/info/db41a553368f4d4a)
- [Optimizing FTS5 External Content Tables and Vacuum Interactions](https://sqlite.work/optimizing-fts5-external-content-tables-and-vacuum-interactions/)
- [SQLite FTS5 Tokenizers: unicode61 and ascii (January 2025)](https://audrey.feldroy.com/articles/2025-01-13-SQLite-FTS5-Tokenizers-unicode61-and-ascii)

### SQLite Multi-DB Atomicity
- [SQLite Forum: Transactions involving multiple databases](https://sqlite.org/forum/forumpost/ecf53d4eb8)
- [How To Corrupt An SQLite Database File — SQLite Official](https://www.sqlite.org/howtocorrupt.html)

### NAS and Network Filesystem
- [Samba: File and Record Locking](https://www.samba.org/samba/docs/old/Samba3-HOWTO/locking.html)
- [O'Reilly: NFS and File Locking](https://docstore.mik.ua/orelly/networking_2ndEd/nfs/ch11_02.htm)

### Windows Long Paths
- [Microsoft Learn: Maximum Path Length Limitation](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation)
- [Microsoft Learn: Enable long path names in Windows 11](https://learn.microsoft.com/en-us/answers/questions/1805411/how-to-enable-long-file-path-names-in-windows-11)

### Unicode and Cross-Platform Paths
- [Cross-Platform Unicode Support in C++](https://www.studyplan.dev/pro-cpp/character-encoding/q/cross-platform-unicode)
- [pathie-cpp: C++ library for crossplatform Unicode path management](https://github.com/Quintus/pathie-cpp)

### SQLite Graph Queries
- [SQLite: Recursive Common Table Expressions](https://sqlite.org/lang_with.html)
- [SQLite Recursive Queries for Graph Traversal](https://runebook.dev/en/articles/sqlite/lang_with/rcex3)

---

*Pitfalls research for: Digital Workshop v1.1 Library Storage & Organization*
*Researched: 2026-02-21*
*Previous PITFALLS.md content (v1.0 threading/CNC work) is superseded by this file for v1.1 milestone.*
