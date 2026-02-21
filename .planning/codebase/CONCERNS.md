# Codebase Concerns

**Analysis Date:** 2026-02-19

## Tech Debt

### WP-05: Custom Math Types Duplicate GLM

- **Files:** `src/core/types.h:34-102`, `src/core/types.cpp`
- **Impact:** Unmaintained vector/matrix code (Vec2, Vec3, Vec4, Mat4) when GLM is already linked in `src/CMakeLists.txt:108`. GLM includes SIMD optimizations. Custom code is ~140 lines of hand-rolled matrix operations (perspective, lookAt, ortho, rotations).
- **Fix approach:** Replace custom types with GLM type aliases. Keep Color type and application-specific aliases.

### WP-06: Duplicate Theme Code

- **Files:** `src/ui/theme.cpp:56-150` vs `src/ui/ui_manager.cpp:159-213`
- **Impact:** Two nearly identical theme systems with slightly different values (0.910f vs 0.92f). One path is dead code, making theme changes unpredictable.
- **Fix approach:** Identify which theme is actually used, remove dead path entirely. Consolidate to single source of truth.

### WP-07: Duplicate Canvas/Pan-Zoom Logic

- **Files:** `src/ui/panels/gcode_panel.cpp:187-270` vs `src/ui/panels/cut_optimizer_panel.cpp:227-308`
- **Impact:** Both panels independently implement invisible button capture, mouse wheel zoom, drag pan, double-click reset. Maintenance burden when fixing pan/zoom bugs.
- **Fix approach:** Extract to shared `Canvas2D` widget class with pan/zoom/reset as composable methods.

### WP-08: Duplicate Optimizer Expansion/Sorting

- **Files:** `src/core/optimizer/bin_packer.cpp:16-34` and `src/core/optimizer/guillotine.cpp:16-34`
- **Impact:** Identical `ExpandedPart` struct, expansion loop, and area-descending sort duplicated across algorithms.
- **Fix approach:** Extract to `optimizer_utils.h` with shared helper functions.

### WP-09: Duplicate Spherical Coordinate Math

- **Files:** `src/ui/panels/viewport_panel.cpp:96-123` and `src/ui/dialogs/lighting_dialog.cpp:18-40`
- **Impact:** Same azimuth/elevation to/from direction vector conversion in two places. Affects both viewport camera and lighting direction.
- **Fix approach:** Move to core math utility (`src/core/math/spherical.h` or add to `Vec3` methods).

### WP-10: Repeated SQL Boilerplate

- **Files:** `src/core/database/model_repository.cpp`, `project_repository.cpp`, `cost_repository.cpp`
- **Impact:** 6+ SELECT methods with identical prepare/isValid/bind/step/extract boilerplate. Column indices are magic numbers that silently break on schema changes.
- **Fix approach:** Create base repository helper with template methods or column index enums to centralize schema awareness.

### WP-11: Repeated Error_Code Pattern in File Utils

- **File:** `src/core/utils/file_utils.cpp:68-191`
- **Impact:** 15+ repetitions of `std::error_code ec; auto result = fs::op(..., ec); if (ec) log::errorf(...);`
- **Fix approach:** Extract to `fsOp()` helper that takes operation lambda and logging context.

### WP-31: Duplicate Number Formatting

- **Files:** `src/ui/panels/properties_panel.cpp:63-87`, `library_panel.cpp:250-257`, `gcode_panel.cpp:134-140`
- **Impact:** File size, triangle count, and time formatting logic repeated in 5+ places.
- **Fix approach:** Create utility functions: `formatBytes()`, `formatNumber()`, `formatTime()`.

---

## Known Bugs

### WP-01: Integer Overflow in STL Binary Parsing

- **File:** `src/core/loaders/stl_loader.cpp:88`
- **Symptoms:** Malformed STL files with crafted triangle count could cause out-of-bounds reads or memory corruption.
- **Trigger:** Load STL with `triangleCount = 0xFFFFFFFF`. Multiplication by 50 bytes per triangle wraps size validation.
- **Root cause:** `triangleCount * 50` overflows before bounds check.
- **Workaround:** Only load STL files from trusted sources.
- **Fix approach:** Add overflow guard: `if (triangleCount > (SIZE_MAX - 84) / 50) { return error; }`

### WP-27: Framebuffer Move Constructor Incomplete

- **File:** `src/render/framebuffer.cpp:12-21`
- **Symptoms:** Moved-from framebuffer object has zeroed GL handles but retains `m_width` and `m_height` values. Double-delete or reuse if accidentally accessed.
- **Root cause:** Move constructor only clears GL handles, not dimensions.
- **Fix approach:** In moved-from object, also zero `m_width = 0; m_height = 0;`

### WP-15: Transaction Error Handling Gap

- **File:** `src/core/database/schema.cpp:148-155` (in `createTables()`)
- **Symptoms:** If `setVersion()` fails during schema initialization, transaction commits partially instead of rolling back.
- **Trigger:** Database write permission lost mid-initialization.
- **Root cause:** Missing rollback on `setVersion()` failure.
- **Fix approach:** Add explicit `txn.rollback()` before `return false` on `setVersion()` failure.

---

## Security Considerations

### WP-12: SQL LIKE Injection

- **Files:** `src/core/database/model_repository.cpp:103,139`, `project_repository.cpp:69`
- **Risk:** User search terms concatenated with `%` wildcards without escaping. Input like "100%" matches all content. Potential information disclosure.
- **Current mitigation:** Database is local file, not network-exposed. Low practical risk but incorrect pattern.
- **Recommendations:** Implement LIKE escape function: replace `%` → `\%`, `_` → `\_`, append `ESCAPE '\'` to query.

### WP-02: Strict Aliasing Violation in Mesh Hashing

- **File:** `src/core/mesh/hash.cpp:58`
- **Risk:** `reinterpret_cast<const u32*>(&pos[i])` violates C++ strict aliasing rules. Compiler optimizations could produce incorrect hash values or memory corruption.
- **Current mitigation:** In practice, most compilers treat this as benign, but it is undefined behavior.
- **Recommendations:** Use `std::memcpy` to copy into `u32` local, or C++20 `std::bit_cast`.

### WP-13: JSON Serialization Has No Escaping

- **Files:** `src/core/database/model_repository.cpp:291`, `cost_repository.cpp:216`
- **Risk:** Tags containing quote characters produce invalid JSON. Could cause parsing errors or data corruption.
- **Current mitigation:** Tags are user-supplied but stored locally. Low risk of exploitation but causes data loss.
- **Recommendations:** Use `nlohmann/json` library or implement proper JSON string escaping (escape `"`, `\`, `/`, control characters).

### WP-30: Fallback to /tmp on Missing HOME

- **File:** `src/core/paths/app_paths.cpp:45`
- **Risk:** If `$HOME` environment variable unset on Linux, config and data paths default to `/tmp`. /tmp is world-readable/writable and not persistent across reboots.
- **Current mitigation:** On most systems, `$HOME` is set. Development risk only.
- **Recommendations:** Fail explicitly with helpful error message, or use `getpwuid(getuid())` to retrieve user home directory as fallback.

---

## Performance Bottlenecks

### WP-04: GPU Mesh Uploaded and Destroyed Every Frame

- **File:** `src/render/renderer.cpp:80-85`
- **Problem:** `renderMesh(const Mesh&)` calls `uploadMesh()` then `destroy()` per frame. No caching. Each mesh re-uploaded every frame from CPU to GPU.
- **Impact:** For a 100k-triangle mesh at 60 FPS: ~30 MB/s sustained VRAM bandwidth per mesh. Multiple meshes compound the waste. Frame time increases with mesh complexity.
- **Cause:** No GPU mesh cache (hash → GPUMesh mapping).
- **Improvement path:**
  1. Add `std::unordered_map<std::string, GPUMesh>` cache keyed by mesh hash
  2. Create RAII `GPUMesh` wrapper with constructor/destructor for proper cleanup
  3. Invalidate cache entry on mesh modification

### WP-24: ViewCube Recomputed Every Frame

- **File:** `src/ui/panels/viewport_panel.cpp:279-451`
- **Problem:** 8 vertices, 6 faces, rotation matrix, depth sort, and hit testing recalculated every frame, even when camera unchanged.
- **Impact:** ~400 extra matrix multiplications per frame unnecessary.
- **Cause:** No caching logic.
- **Improvement path:** Cache vertex positions and faces; recompute only when camera orientation changes.

### WP-25: Angle Wrapping Uses Loops

- **File:** `src/render/camera.cpp:38-41`
- **Problem:** `while (m_yaw > 360) m_yaw -= 360;` is O(n) in angle magnitude.
- **Impact:** If yaw exceeds 360 by many rotations (rare but possible), scales linearly.
- **Fix:** Use `fmod()` for O(1) wrap: `m_yaw = fmod(m_yaw, 360.0f);`

### WP-33: OBJ/3MF Loaders Don't Reserve Vector Capacity

- **Files:** `src/core/loaders/obj_loader.cpp:29-35`, `src/core/loaders/threemf_loader.cpp`
- **Problem:** Unlike STL loader, no `reserve()` call. Causes repeated reallocations on large files as vertices/triangles are appended.
- **Impact:** For 1M-triangle model: ~20 reallocations + memcopies during parsing.
- **Fix approach:** First-pass count (e.g., scan OBJ for vertex count) or size-based estimate for `reserve()`.

### WP-41: Bounds Recalculation Inefficiency

- **File:** `src/core/mesh/mesh.cpp:72-89`
- **Problem:** `centerOnOrigin()` and `normalizeSize()` each call `recalculateBounds()`, iterating all vertices twice for same operation.
- **Impact:** For 500k-vertex mesh: 1M vertex iterations instead of 500k.
- **Fix approach:** Add dirty flag; only recalculate when modified. Or compute bounds mathematically during transform.

---

## Fragile Areas

### WP-03: Data Race on Import Progress

- **File:** `src/core/import/import_queue.cpp:110` (in progress tracking)
- **Files involved:** `src/core/import/import_task.h:85` (ImportProgress::setCurrentFileName)
- **Why fragile:** Worker threads write `m_progress.currentFileName` (char[256]) while UI thread reads simultaneously without synchronization. Code comment calls this "benign" but it is undefined behavior.
- **Safe modification:** Replace manual char buffer with `std::atomic<std::string>` under a dedicated mutex, or use `std::atomic_flag` + double-buffering.
- **Test coverage:** No thread safety tests for ImportProgress.

### WP-14: Config Watcher Not Thread-Safe

- **File:** `src/core/config/config_watcher.cpp:44-46`
- **Why fragile:** `m_lastMtime` read/written without atomics or locks in both file watch callback and main thread config reload.
- **Safe modification:** Use `std::atomic<int64_t>` for `m_lastMtime`.

### WP-28: No GL Error Checking on Buffer Uploads

- **File:** `src/render/renderer.cpp:173-217` (in uploadMesh)
- **Why fragile:** `glGenBuffers`, `glBufferData`, `glBindTexture` called with zero error checks. GPU out-of-memory or driver errors silently fail.
- **Safe modification:** Add GL_CHECK macro or validation after critical calls; propagate error status to caller.

### WP-42: Shader Uniform Cache Stores -1 Without Distinction

- **File:** `src/render/shader.cpp:94-103`
- **Why fragile:** Caches `glGetUniformLocation` result of -1 (uniform not found) identically to valid locations. Invalid location reused without re-lookup attempt.
- **Safe modification:** Don't cache -1, or use `std::optional<GLint>` to distinguish "not found" from "valid location 0".

---

## Scaling Limits

### Database Connection Pool Exhaustion

- **File:** `src/core/database/connection_pool.h`, `connection_pool.cpp:41-46`
- **Current capacity:** Fixed pool size at application startup (default 5 connections, configurable).
- **Limit:** If import thread count exceeds pool size, threads block in `ScopedConnection::acquire()` waiting for connection. With 8 import workers and 5 connections, 3 workers stall until connections release.
- **Note:** `ImportQueue::enqueue()` line 50 warns about this but cannot resize pool dynamically. Pool size must match worst-case thread count.
- **Scaling path:**
  1. Either increase pool size before creating ImportQueue
  2. Or implement dynamic pool expansion (complex; requires thread-safe resizing)
  3. Or reduce import thread count via config to ≤ pool size

### Mesh Vertex Count Limits

- **Files:** All loaders store vertex count as `u32` (4 billion max)
- **Practical limit:** ~500M vertices in memory at 32 bytes/vertex = 16 GB. Most machines have 16-32 GB RAM.
- **Beyond scaling:** Requires streaming/out-of-core geometry system.

### Library Size Limits

- **Issue:** All models loaded into memory (`getAllModels()` in `library_manager.h:23`). With 10k models (typical large shop), ~100 MB metadata in memory.
- **Scaling path:** Implement lazy loading with pagination or database query filters.

---

## Test Coverage Gaps

### WP-40: Cost System Untested

- **What's not tested:** `src/core/database/cost_repository.cpp/h`
- **Files:** Not in test dependencies, no test file exists.
- **Risk:** Cost calculations and persistence untested; regressions silently introduced.
- **Priority:** Medium — cost system used in phase plans but not yet exercised in live code.
- **Fix approach:** Add `cost_repository.cpp` to test deps in `tests/CMakeLists.txt`, create `tests/test_cost_repository.cpp`.

### WP-32: No Mesh Validation Post-Loading

- **What's not tested:** All loaders (`stl_loader.cpp`, `obj_loader.cpp`, `threemf_loader.cpp`, `gcode_loader.cpp`)
- **Risk:** Degenerate triangles (zero area), NaN normals, extreme coordinates, or invalid topology silently accepted.
- **Impact:** Can cause renderer crashes (division by zero in normal calculation) or optimization failures.
- **Fix approach:** Add optional validation pass (or mandatory in debug builds): check for degenerate triangles, NaN/Inf coordinates, isolated vertices.

### Thread Safety Tests

- **Current:** No concurrent import tests. ImportProgress data race (WP-03) untested.
- **Fix approach:** Add tests with ThreadSanitizer (asan/tsan) running multiple import workers while reading progress.

---

## Dependency Issues

### Missing Version Constraints

- **File:** `src/CMakeLists.txt`
- **Issue:** Third-party dependencies (SDL2, glad, zlib, etc.) have no version constraints in FetchContent/find_package calls.
- **Impact:** Breaking changes in future versions could silently break builds.
- **Fix:** Add `VERSION_RANGE` or `REQUIRE_VERSION` to dependency declarations.

### Custom Math Types Create Maintenance Burden

- **Issue:** (Related to WP-05) Custom matrix operations diverge from GLM. If GLM is updated or features added, custom code remains stale.
- **Fix:** Replace with GLM to leverage community-maintained SIMD implementations.

---

## Missing Critical Features (Blocking Future Phases)

### Schema Versioning Not Implemented

- **File:** `src/core/database/schema.cpp:20` (TODO comment)
- **Problem:** Comment reads: "TODO(v2.0): Implement version-aware ALTER TABLE migrations."
- **Current behavior:** On schema version mismatch, logs warning but continues. Tables use `IF NOT EXISTS`, so missing columns silently stay missing.
- **Blocks:** Adding new database columns in future versions risks data loss or corruption.
- **Priority:** High — needed before any database schema change.
- **Fix approach:**
  1. Implement `migrate(oldVersion, newVersion)` function with per-version ALTER TABLE statements
  2. Call within `Schema::initialize()` before returning success
  3. Test migration path in unit tests

### Import Queue Hard Stops on Pool Exhaustion

- **File:** `src/core/database/connection_pool.cpp:45`
- **Issue:** `throw std::runtime_error("ConnectionPool exhausted")` with no graceful degradation or queuing.
- **Blocks:** Scaling import to many workers without pre-calculating required pool size.
- **Fix:** Either size pool dynamically or implement work-stealing between workers.

### UIManager StartPage Callback Wiring Incomplete

- **File:** `src/managers/ui_manager.cpp:69` (NOTE comment)
- **Issue:** "StartPage callbacks are NOT wired here. Application wires those after both UIManager and FileIOManager exist."
- **Impact:** Risk of callbacks firing before managers initialized.
- **Fix approach:** Formalize this coupling with explicit initialization order documentation or dependency injection.

---

## Code Quality Issues

### WP-22: Application God Class

- **File:** `src/app/application.h`
- **Issue:** Owns 7 panels, 2+ dialogs, 6 core systems, 15+ callback methods (~40 distinct concerns).
- **Impact:** Hard to test, understand, or modify. Single point of failure.
- **Already partially addressed:** UIManager and FileIOManager extracted, but Application still coordinates them.
- **Recommendation:** Continue extraction pattern; keep Application as thin orchestrator only.

### WP-23: PropertiesPanel Static Locals as Hidden State

- **File:** `src/ui/panels/properties_panel.cpp:237,260,273,293`
- **Issue:** `static float targetSize`, `translate[3]`, `rotateDeg[3]`, `scaleVal[3]` inside render method. Hidden state not visible in class definition.
- **Impact:** Difficult to reason about state lifetime and persistence.
- **Fix:** Move to class member variables with clear initialization.

### WP-29: Fixed 1024-Byte Log Buffer

- **File:** `src/core/utils/log.h:23-51`
- **Issue:** `snprintf` into `char[1024]` silently truncates. Four near-identical template functions (error, warning, info, debug).
- **Impact:** Long error messages or format strings truncated silently.
- **Fix:** Use dynamic allocation (`std::string`) or single parameterized helper function.

---

## Design Inconsistencies

### WP-16: Unused UIManager File Dialog Stubs

- **File:** `src/ui/ui_manager.cpp:258-268`
- **Issue:** `openFileDialog()` and `saveFileDialog()` return `false`. Dead code.
- **Fix:** Remove or implement.

### WP-17: Unused ViewportPanel Input State

- **File:** `src/ui/panels/viewport_panel.h:53-56`
- **Issue:** `m_isDragging`, `m_isPanning`, `m_lastMouseX`, `m_lastMouseY` never set or read.
- **Fix:** Remove fields.

### WP-18: Unused LibraryPanel Field

- **File:** `src/ui/panels/library_panel.h:59`
- **Issue:** `m_contextMenuModelIndex` never used.
- **Fix:** Remove field.

### WP-19: Empty ProjectPanel::refresh()

- **File:** `src/ui/panels/project_panel.cpp:29-31`
- **Issue:** Empty method, never called.
- **Fix:** Remove method and declaration.

### WP-20: Icons System Unused

- **File:** `src/ui/icons.h`
- **Issue:** 93 lines of icon constants with no icon font loaded.
- **Fix:** Remove until icon font integration is implemented.

### WP-21: Database::handle() Exposes Raw Pointer

- **File:** `src/core/database/database.h:89-90`
- **Issue:** Breaks encapsulation, no callers found.
- **Fix:** Remove or make private.

---

## Minor Issues

### WP-26: Archive Uses Manual Path Operations

- **File:** `src/core/archive/archive.cpp:45-72`
- **Issue:** Hand-rolled `makeRelativePath()` and recursive traversal instead of `std::filesystem::relative()` and `recursive_directory_iterator`.
- **Impact:** More code to maintain; potential edge cases in path handling.
- **Fix:** Use std::filesystem utilities directly.

### WP-34: Floating-Point Comparisons Without Epsilon

- **Files:** `src/core/optimizer/bin_packer.cpp`, `guillotine.cpp` (placement checks)
- **Issue:** Rectangle dimensions compared with `>` and `<` without tolerance. Rounding errors near boundaries cause placement failures.
- **Impact:** Packing sometimes fails near tight fits due to floating-point imprecision.
- **Fix:** Define epsilon constant; use `fabs(a - b) < epsilon` for equality checks.

---

## Summary by Priority

**CRITICAL (Fix immediately for safety/correctness):**
- WP-01: Integer overflow in STL parsing
- WP-02: Strict aliasing violation
- WP-03: Data race on import progress
- WP-12: SQL LIKE injection (low risk but incorrect pattern)
- WP-13: JSON escaping missing

**HIGH (Fix soon for maintainability and performance):**
- WP-04: GPU mesh upload every frame (50% perf win possible)
- WP-05: Custom math types duplicate GLM
- WP-06: Duplicate theme code
- WP-15: Transaction rollback missing
- WP-24: ViewCube recomputed every frame
- Schema versioning (blocks future DB changes)

**MEDIUM (Fix when touching related code):**
- WP-07 through WP-11: DRY violations
- WP-14: Config watcher thread safety
- WP-22: Application god class
- WP-28: GL error checking
- WP-33: Vector reserve for loaders

**LOW (Track for later):**
- WP-16 through WP-20: Dead code cleanup
- WP-25, WP-31: Style improvements
- WP-26: Archive path operations
- WP-34: Floating-point tolerance

---

*Concerns audit: 2026-02-19*
