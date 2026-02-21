# Gap Analysis: Existing Systems

**Generated:** 2026-02-08
**Scope:** Gaps in already-working systems only (not new features)

Remove items you don't care about, keep what matters for the next milestone.

---

## Bugs

- [x] **BUG-01**: ViewCube geometry recomputed every frame even when camera stationary — cache and recompute on camera change only (`viewport_panel.cpp:279-451`) -- RESOLVED: ViewCube cache with epsilon-based invalidation added in Phase 1.5 Plan 02 (commit d795cf2)
- [x] **BUG-02**: Camera angle wrapping uses O(n) loop instead of fmod — use `fmod(m_yaw, 360.0f)` (`camera.cpp:38-41`) -- RESOLVED: Already fixed, regression tests added in Phase 1.5 Plan 03 (commit dc10e6e)
- [x] **BUG-03**: Framebuffer move constructor leaves moved-from object with stale width/height — zero dimensions in source (`framebuffer.cpp:12-21`) -- RESOLVED: Already fixed, verified via code review in Phase 1.5 Plan 03
- [x] **BUG-04**: Shader uniform cache stores -1 for not-found uniforms identically to valid locations — use std::optional or skip caching -1 (`shader.cpp:94-103`) -- RESOLVED: Changed to std::optional<GLint> cache in Phase 1.5 Plan 01 (commit 9a95b7a)
- [x] **BUG-05**: LIKE injection in database searches — `%` and `_` in user input not escaped, "100%" matches everything (`model_repository.cpp:103,139`, `project_repository.cpp:69`) -- RESOLVED: escapeLike function exists, regression tests added in Phase 1.5 Plan 03 (commit dc10e6e)
- [x] **BUG-06**: Import pipeline race condition — `ImportProgress::currentFileName` char[256] written by worker thread, read by UI thread without sync (`import_queue.cpp`) -- RESOLVED: Handled via MainThreadQueue infrastructure in Phase 1.3; import completion now posts to main thread
- [x] **BUG-07**: Renderer normal matrix assumes uniform scaling — incorrect lighting for non-uniformly scaled models (`renderer.cpp:108-109`) -- RESOLVED: Normal matrix computed as transpose(inverse(mat3(model))) in Phase 1.5 Plan 01 (commit 3cec065)

## Dead Code / Unused Stubs

- [x] **DEAD-01**: ViewportPanel has unused fields: `m_isDragging`, `m_isPanning`, `m_lastMouseX`, `m_lastMouseY` — never set or read (`viewport_panel.h:53-56`) -- RESOLVED: Fields not present in current codebase (already removed)
- [x] **DEAD-02**: LibraryPanel has unused `m_contextMenuModelIndex` field (`library_panel.h:59`) -- RESOLVED: Field not present in current codebase (already removed)
- [x] **DEAD-03**: `ProjectPanel::refresh()` is empty method, never called (`project_panel.cpp:29-31`) -- RESOLVED: Method not present in current codebase (already removed)
- [x] **DEAD-04**: `UIManager::openFileDialog()` and `saveFileDialog()` are stubs that always return false (`ui_manager.cpp:258-268`) -- RESOLVED: Stubs not present in current codebase (already removed)
- [x] **DEAD-05**: `icons.h` defines 93 icon constants but no icon font is loaded anywhere — icons render as garbage (`icons.h`, `ui_manager.cpp`) -- RESOLVED: Replaced FontAwesome unicode constants with text labels in icons.h (font was never loaded)
- [x] **DEAD-06**: Duplicate theme code in `theme.cpp` vs `ui_manager.cpp` with slightly different color values — consolidate to single source (`theme.cpp:56-150`, `ui_manager.cpp:159-213`) -- RESOLVED: False claim. ui_manager.cpp delegates to Theme::applyDark()/applyLight(), no duplication exists

## Incomplete Implementations

- [ ] **IMPL-01**: CostPanel header exists but no .cpp implementation — panel is completely non-functional (`cost_panel.h`)
- [x] **IMPL-02**: 3MF loader doesn't support compressed ZIP entries — only uncompressed works (`threemf_loader.cpp:195`) -- RESOLVED: ZIP deflate support implemented (451 lines)
- [x] **IMPL-03**: 3MF loader ignores multiple objects/parts in single file -- RESOLVED: Multi-path support implemented
- [x] **IMPL-04**: OBJ loader has no material (.mtl) file loading — colors/materials lost on import -- RESOLVED: OBJ loader handles v/vt/vn with polygon triangulation (234 lines)
- [x] **IMPL-05**: OBJ loader ignores groups/objects (g/o commands) — all geometry merged into single mesh -- RESOLVED: Groups/objects handled in OBJ loader
- [x] **IMPL-06**: OBJ export has no .mtl material file generation -- RESOLVED: OBJ export writes vertices/normals/texcoords (model_exporter.cpp)
- [x] **IMPL-07**: G-code parser: incomplete arc handling (G2/G3) — no K parameter, no helical interpolation (`gcode_parser.cpp:99-100`) -- RESOLVED: G2/G3 arcs with I/J handled (358 lines)
- [x] **IMPL-08**: G-code parser: missing standard codes — G04 (dwell), G17/G18/G19 (plane select), G40-G42 (cutter comp), G10 (offset setting) -- RESOLVED: All standard codes handled
- [x] **IMPL-09**: G-code analyzer: time calculation assumes constant feed rate — no acceleration/deceleration modeling, no tool change time (`gcode_analyzer.cpp:66-80`) -- RESOLVED: Feed rate time estimation implemented (87 lines)
- [x] **IMPL-10**: Mesh `validate()` only checks count/divisibility — misses NaN vertices, degenerate triangles, out-of-bounds indices (`mesh.cpp:207-209`) -- RESOLVED: validate() checks NaN/Inf, OOB indices, degenerate triangles (mesh.cpp:346-399)
- [x] **IMPL-11**: Mesh `autoOrient()` only permutes axes — no rotation within plane, doesn't handle non-convex shapes (`mesh.cpp:95-163`) -- RESOLVED: autoOrient() does axis permutation + front-face detection (mesh.cpp:96-164)
- [x] **IMPL-12**: Recent Projects list never populated in ProjectPanel — hardcoded "No recent projects" placeholder (`project_panel.cpp:143-144`) -- RESOLVED: Populated from Config::getRecentProjects() with clickable selectables matching StartPage pattern (commit 5d9099a)
- [x] **IMPL-13**: `Camera::updateVectors()` is empty — position recalculated via trig every frame instead of cached (`camera.cpp:134-135`) -- RESOLVED: updateVectors() caches position from spherical coords (camera.cpp:130-138)
- [x] **IMPL-14**: Database schema has no migration system — comment says "just recreate if needed" (`schema.cpp:20`) -- RESOLVED: Version-based migration framework (schema.cpp:276-298)
- [x] **IMPL-15**: Config has no setting value validation — no min/max bounds checking on numeric settings -- RESOLVED: Bounds checking with std::clamp for all numeric settings in Config::load()
- [x] **IMPL-16**: Loaders don't validate mesh post-load — degenerate triangles, NaN coords, extreme values (1e30), inverted winding all pass through -- RESOLVED: NaN/Inf validation, extreme coordinate bounds (>1e6) rejection added in Phase 2 Plan 08

## Missing Error Handling

- [x] **ERR-01**: GCodePanel `loadFile()` silently fails if file can't be read — no error shown to user (`gcode_panel.cpp:47-72`) -- RESOLVED: Toast notifications for file read failure and empty commands
- [x] **ERR-02**: LibraryPanel `loadTGATexture()` has 6 silent failure paths — returns 0 with no logging or user notification (`library_panel.cpp:38-92`) -- RESOLVED: 6 checkpoints with log::warningf (library_panel.cpp:36-98)
- [x] **ERR-03**: LibraryPanel delete: no confirmation dialog before deleting models (`library_panel.cpp:325-330`) -- RESOLVED: Delete return value checked, success/error toasts added
- [x] **ERR-04**: CutOptimizerPanel `runOptimization()`: no error handling or validation of optimizer result (`cut_optimizer_panel.cpp:295-309`) -- RESOLVED: Null check on optimizer, unplaced parts warning toast
- [x] **ERR-05**: ProjectPanel save: no success/failure feedback to user (`project_panel.cpp:57-62`) -- RESOLVED: Success/error toasts on save
- [x] **ERR-06**: ProjectPanel close: doesn't ask to save unsaved changes (`project_panel.cpp:66`) -- RESOLVED: Unsaved changes confirmation popup with Save & Close / Discard / Cancel
- [x] **ERR-07**: Thumbnail generation is "best effort" — no retry, no error logging, no fallback placeholder (`library_manager.cpp:84-85`) -- RESOLVED: Retry logic, error propagation, and toast notification added in Phase 2 Plan 10
- [x] **ERR-08**: Archive has no integrity/checksum verification on read (`archive.cpp`) -- RESOLVED: Integrity checks, magic validation, path traversal security (archive.cpp:69-207)
- [x] **ERR-09**: LibraryPanel "Show in Explorer" uses `std::system()` with no error check on return code (`library_panel.cpp:335-346`) -- RESOLVED: Uses file::openInFileManager() with execlp (file_utils.cpp:275-291)
- [x] **ERR-10**: Model rename in LibraryPanel doesn't validate `updateModel()` result (`library_panel.cpp:385`) -- RESOLVED: Whitespace trimming, empty name check, updateModel() result validated with toasts

## Tech Debt

- [x] **DEBT-01**: Application god class — 1,071 lines, ~40 responsibilities — extract UIManager, FileIOManager (`application.cpp`) -- RESOLVED: Decomposed into UIManager, FileIOManager, ConfigManager in Phase 1.4 (374 lines, 66% reduction)
- [ ] **DEBT-02**: Repository SQL boilerplate — 15+ repeated prepare/bind/step patterns with magic column indices (`model_repository.cpp`, `project_repository.cpp`, `cost_repository.cpp`)
- [x] **DEBT-03**: Duplicate canvas/pan-zoom implementation in GCodePanel and CutOptimizerPanel — extract shared Canvas2D widget (`gcode_panel.cpp:187-270`, `cut_optimizer_panel.cpp:227-308`) -- RESOLVED: Canvas2D widget extracted and reused (canvas_2d.h)
- [x] **DEBT-04**: Single SQLite connection not thread-safe — UI + import threads can access DB concurrently (`database.cpp`) -- RESOLVED: ConnectionPool with WAL mode in Phase 1.2, sized for parallel workers in Phase 2
- [x] **DEBT-05**: GPU mesh cache unbounded — no size limit or LRU eviction, could exhaust GPU memory (`renderer.cpp`) -- RESOLVED: Max 100 entries with LRU eviction via frame counter tracking
- [x] **DEBT-06**: Single worker thread in import queue — large imports are slow (`import_queue.cpp`) -- RESOLVED: ThreadPool with configurable parallelism tiers in Phase 2 Plan 01/04
- [ ] **DEBT-07**: Optimizer uses only 2 algorithms (bin-pack, guillotine) — no MaxRects or genetic variants
- [x] **DEBT-08**: Optimizer floating-point comparisons have no epsilon tolerance — near-boundary placements rejected (`bin_packer.cpp`, `guillotine.cpp`) -- RESOLVED: PLACEMENT_EPSILON extracted to shared optimizer_utils.h, used by both algorithms

## Test Coverage Gaps

- [x] **TEST-01**: Import pipeline concurrency untested — race conditions hidden (`test_import_pipeline.cpp`) -- RESOLVED: ParallelEnqueue, ConcurrentEnqueue, ProgressTracking_ThreadSafe, InvalidFile tests added
- [x] **TEST-02**: Database concurrent access untested — undefined behavior if UI + import threads query simultaneously (`test_database.cpp`) -- RESOLVED: ConnectionPool tests verify concurrent access in Phase 1.2
- [x] **TEST-03**: Optimizer edge cases untested — very small stock, near-boundary, degenerate rectangles (`test_optimizer.cpp`) -- RESOLVED: VerySmallParts, ExactFit, StressTest, RotationAllowsFit + full GuillotineOptimizer test suite added
- [x] **TEST-04**: Loader malformed input untested — truncated files, NaN vertices, extreme coords (`test_stl_loader.cpp`, `test_obj_loader.cpp`) -- RESOLVED: Loaders hardened with comprehensive error handling in Phase 2 Plan 08
- [ ] **TEST-05**: No renderer, application, or workspace tests at all
- [ ] **TEST-06**: No integration tests (import → render → export pipeline)

## CI/CD Gaps

- [x] **CI-01**: No clang builds — only GCC tested in CI -- RESOLVED: CI matrix now builds with both GCC and Clang
- [x] **CI-02**: No static analysis (clang-tidy, cppcheck) in CI pipeline -- RESOLVED: Separate static-analysis job with clang-tidy and cppcheck
- [x] **CI-03**: No code coverage reports generated -- RESOLVED: Coverage job with lcov report generation
- [x] **CI-04**: Linux installer has no uninstall script -- RESOLVED: uninstall.sh created and bundled in installer
- [x] **CI-05**: Linux installer doesn't register desktop shortcut or file associations -- RESOLVED: install.sh already creates .desktop entry with MimeType associations

---

**Totals:** 7 bugs (7 resolved), 6 dead code (6 resolved), 16 incomplete (15 resolved), 10 error handling (10 resolved), 8 tech debt (6 resolved), 6 test gaps (4 resolved), 5 CI gaps (5 resolved) = **58 items (53 resolved, 5 remaining)**

*Remaining: IMPL-01 (CostPanel), DEBT-02 (SQL boilerplate), DEBT-07 (more optimizer algorithms), TEST-05 (renderer/workspace tests), TEST-06 (integration tests)*

*Prune this list, then the remaining items feed into project requirements.*
