# Gap Analysis: Existing Systems

**Generated:** 2026-02-08
**Scope:** Gaps in already-working systems only (not new features)

Remove items you don't care about, keep what matters for the next milestone.

---

## Bugs

- [ ] **BUG-01**: ViewCube geometry recomputed every frame even when camera stationary — cache and recompute on camera change only (`viewport_panel.cpp:279-451`)
- [ ] **BUG-02**: Camera angle wrapping uses O(n) loop instead of fmod — use `fmod(m_yaw, 360.0f)` (`camera.cpp:38-41`)
- [ ] **BUG-03**: Framebuffer move constructor leaves moved-from object with stale width/height — zero dimensions in source (`framebuffer.cpp:12-21`)
- [ ] **BUG-04**: Shader uniform cache stores -1 for not-found uniforms identically to valid locations — use std::optional or skip caching -1 (`shader.cpp:94-103`)
- [ ] **BUG-05**: LIKE injection in database searches — `%` and `_` in user input not escaped, "100%" matches everything (`model_repository.cpp:103,139`, `project_repository.cpp:69`)
- [ ] **BUG-06**: Import pipeline race condition — `ImportProgress::currentFileName` char[256] written by worker thread, read by UI thread without sync (`import_queue.cpp`)
- [ ] **BUG-07**: Renderer normal matrix assumes uniform scaling — incorrect lighting for non-uniformly scaled models (`renderer.cpp:108-109`)

## Dead Code / Unused Stubs

- [x] **DEAD-01**: ViewportPanel has unused fields: `m_isDragging`, `m_isPanning`, `m_lastMouseX`, `m_lastMouseY` — never set or read (`viewport_panel.h:53-56`) -- RESOLVED: Fields not present in current codebase (already removed)
- [x] **DEAD-02**: LibraryPanel has unused `m_contextMenuModelIndex` field (`library_panel.h:59`) -- RESOLVED: Field not present in current codebase (already removed)
- [x] **DEAD-03**: `ProjectPanel::refresh()` is empty method, never called (`project_panel.cpp:29-31`) -- RESOLVED: Method not present in current codebase (already removed)
- [x] **DEAD-04**: `UIManager::openFileDialog()` and `saveFileDialog()` are stubs that always return false (`ui_manager.cpp:258-268`) -- RESOLVED: Stubs not present in current codebase (already removed)
- [x] **DEAD-05**: `icons.h` defines 93 icon constants but no icon font is loaded anywhere — icons render as garbage (`icons.h`, `ui_manager.cpp`) -- RESOLVED: Replaced FontAwesome unicode constants with text labels in icons.h (font was never loaded)
- [x] **DEAD-06**: Duplicate theme code in `theme.cpp` vs `ui_manager.cpp` with slightly different color values — consolidate to single source (`theme.cpp:56-150`, `ui_manager.cpp:159-213`) -- RESOLVED: False claim. ui_manager.cpp delegates to Theme::applyDark()/applyLight(), no duplication exists

## Incomplete Implementations

- [ ] **IMPL-01**: CostPanel header exists but no .cpp implementation — panel is completely non-functional (`cost_panel.h`)
- [ ] **IMPL-02**: 3MF loader doesn't support compressed ZIP entries — only uncompressed works (`threemf_loader.cpp:195`)
- [ ] **IMPL-03**: 3MF loader ignores multiple objects/parts in single file
- [ ] **IMPL-04**: OBJ loader has no material (.mtl) file loading — colors/materials lost on import
- [ ] **IMPL-05**: OBJ loader ignores groups/objects (g/o commands) — all geometry merged into single mesh
- [ ] **IMPL-06**: OBJ export has no .mtl material file generation
- [ ] **IMPL-07**: G-code parser: incomplete arc handling (G2/G3) — no K parameter, no helical interpolation (`gcode_parser.cpp:99-100`)
- [ ] **IMPL-08**: G-code parser: missing standard codes — G04 (dwell), G17/G18/G19 (plane select), G40-G42 (cutter comp), G10 (offset setting)
- [ ] **IMPL-09**: G-code analyzer: time calculation assumes constant feed rate — no acceleration/deceleration modeling, no tool change time (`gcode_analyzer.cpp:66-80`)
- [ ] **IMPL-10**: Mesh `validate()` only checks count/divisibility — misses NaN vertices, degenerate triangles, out-of-bounds indices (`mesh.cpp:207-209`)
- [ ] **IMPL-11**: Mesh `autoOrient()` only permutes axes — no rotation within plane, doesn't handle non-convex shapes (`mesh.cpp:95-163`)
- [ ] **IMPL-12**: Recent Projects list never populated in ProjectPanel — hardcoded "No recent projects" placeholder (`project_panel.cpp:143-144`)
- [ ] **IMPL-13**: `Camera::updateVectors()` is empty — position recalculated via trig every frame instead of cached (`camera.cpp:134-135`)
- [ ] **IMPL-14**: Database schema has no migration system — comment says "just recreate if needed" (`schema.cpp:20`)
- [ ] **IMPL-15**: Config has no setting value validation — no min/max bounds checking on numeric settings
- [ ] **IMPL-16**: Loaders don't validate mesh post-load — degenerate triangles, NaN coords, extreme values (1e30), inverted winding all pass through

## Missing Error Handling

- [ ] **ERR-01**: GCodePanel `loadFile()` silently fails if file can't be read — no error shown to user (`gcode_panel.cpp:47-72`)
- [ ] **ERR-02**: LibraryPanel `loadTGATexture()` has 6 silent failure paths — returns 0 with no logging or user notification (`library_panel.cpp:38-92`)
- [ ] **ERR-03**: LibraryPanel delete: no confirmation dialog before deleting models (`library_panel.cpp:325-330`)
- [ ] **ERR-04**: CutOptimizerPanel `runOptimization()`: no error handling or validation of optimizer result (`cut_optimizer_panel.cpp:295-309`)
- [ ] **ERR-05**: ProjectPanel save: no success/failure feedback to user (`project_panel.cpp:57-62`)
- [ ] **ERR-06**: ProjectPanel close: doesn't ask to save unsaved changes (`project_panel.cpp:66`)
- [ ] **ERR-07**: Thumbnail generation is "best effort" — no retry, no error logging, no fallback placeholder (`library_manager.cpp:84-85`)
- [ ] **ERR-08**: Archive has no integrity/checksum verification on read (`archive.cpp`)
- [ ] **ERR-09**: LibraryPanel "Show in Explorer" uses `std::system()` with no error check on return code (`library_panel.cpp:335-346`)
- [ ] **ERR-10**: Model rename in LibraryPanel doesn't validate `updateModel()` result (`library_panel.cpp:385`)

## Tech Debt

- [ ] **DEBT-01**: Application god class — 1,071 lines, ~40 responsibilities — extract UIManager, FileIOManager (`application.cpp`)
- [ ] **DEBT-02**: Repository SQL boilerplate — 15+ repeated prepare/bind/step patterns with magic column indices (`model_repository.cpp`, `project_repository.cpp`, `cost_repository.cpp`)
- [ ] **DEBT-03**: Duplicate canvas/pan-zoom implementation in GCodePanel and CutOptimizerPanel — extract shared Canvas2D widget (`gcode_panel.cpp:187-270`, `cut_optimizer_panel.cpp:227-308`)
- [ ] **DEBT-04**: Single SQLite connection not thread-safe — UI + import threads can access DB concurrently (`database.cpp`)
- [ ] **DEBT-05**: GPU mesh cache unbounded — no size limit or LRU eviction, could exhaust GPU memory (`renderer.cpp`)
- [ ] **DEBT-06**: Single worker thread in import queue — large imports are slow (`import_queue.cpp`)
- [ ] **DEBT-07**: Optimizer uses only 2 algorithms (bin-pack, guillotine) — no MaxRects or genetic variants
- [ ] **DEBT-08**: Optimizer floating-point comparisons have no epsilon tolerance — near-boundary placements rejected (`bin_packer.cpp`, `guillotine.cpp`)

## Test Coverage Gaps

- [ ] **TEST-01**: Import pipeline concurrency untested — race conditions hidden (`test_import_pipeline.cpp`)
- [ ] **TEST-02**: Database concurrent access untested — undefined behavior if UI + import threads query simultaneously (`test_database.cpp`)
- [ ] **TEST-03**: Optimizer edge cases untested — very small stock, near-boundary, degenerate rectangles (`test_optimizer.cpp`)
- [ ] **TEST-04**: Loader malformed input untested — truncated files, NaN vertices, extreme coords (`test_stl_loader.cpp`, `test_obj_loader.cpp`)
- [ ] **TEST-05**: No renderer, application, or workspace tests at all
- [ ] **TEST-06**: No integration tests (import → render → export pipeline)

## CI/CD Gaps

- [ ] **CI-01**: No clang builds — only GCC tested in CI
- [ ] **CI-02**: No static analysis (clang-tidy, cppcheck) in CI pipeline
- [ ] **CI-03**: No code coverage reports generated
- [ ] **CI-04**: Linux installer has no uninstall script
- [ ] **CI-05**: Linux installer doesn't register desktop shortcut or file associations

---

**Totals:** 7 bugs, 6 dead code, 16 incomplete, 10 error handling, 8 tech debt, 6 test gaps, 5 CI gaps = **58 items**

*Prune this list, then the remaining items feed into project requirements.*
