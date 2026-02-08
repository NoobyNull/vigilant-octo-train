# Codebase Concerns

**Analysis Date:** 2026-02-08

## Tech Debt

### Application God Class
- **Issue**: `src/app/application.cpp` is 1,071 lines with ~40 distinct responsibilities. Orchestrates 7 UI panels, 2+ dialogs, 6 core systems (Database, LibraryManager, ProjectManager, Workspace, ImportQueue, ThumbnailGenerator), 15+ event callback methods, menu setup, config watching, and layout management in single class.
- **Files**: `src/app/application.h`, `src/app/application.cpp`
- **Impact**: Difficult to test individual concerns, high coupling, makes adding features risky. Every UI event flows through Application, increasing complexity.
- **Fix approach**: Extract UIManager for panel/dialog lifecycle, FileIOManager for import/export/project operations, keep Application as thin orchestrator. Requires careful dependency injection redesign.

### SQL Boilerplate Repetition
- **Issue**: Repository classes (`src/core/database/model_repository.cpp`, `project_repository.cpp`, `cost_repository.cpp`) repeat 15+ SELECT/UPDATE patterns: prepare statement, call isValid(), bind parameters, step(), extract row, construct object. Column indices are magic numbers that break silently on schema changes (no constants/enums).
- **Files**: `src/core/database/model_repository.cpp:52-67`, `project_repository.cpp:41-67`, `cost_repository.cpp:78-95`
- **Impact**: Schema changes require hunting through multiple files. Easy to miss column index updates. Low maintainability.
- **Fix approach**: Create base repository helper with templated rowToObject<T>(), or use column enum/constants for index consistency.

### Duplicate Canvas/Pan-Zoom Implementation
- **Issue**: Both `GCodePanel` and `CutOptimizerPanel` independently implement invisible button capture, mouse wheel zoom, drag pan, and double-click reset for 2D canvas. Code is identical in logic but separate.
- **Files**: `src/ui/panels/gcode_panel.cpp:187-270` (GCode visualization canvas), `src/ui/panels/cut_optimizer_panel.cpp:227-308` (Cut list canvas)
- **Impact**: Bug fixes or UX improvements to one canvas don't apply to the other. Risk of divergence.
- **Fix approach**: Extract shared `Canvas2D` widget class with configurable draw callback.

### Duplicate Theme System
- **Issue**: Two nearly identical theme code paths exist: `src/ui/theme.cpp:56-150` vs `src/ui/ui_manager.cpp:159-213` with slightly different values (hardcoded colors differ: 0.910f vs 0.92f). One is dead code.
- **Files**: `src/ui/theme.cpp`, `src/ui/ui_manager.cpp`
- **Impact**: Theme changes must be made in two places or risk inconsistency. Dead code adds cognitive load.
- **Fix approach**: Remove dead theme code path, consolidate to single source of truth in theme.cpp.

### Unused UI State and Stubs
- **Issue**: Multiple unused fields and dead methods across UI:
  - `ViewportPanel` class has `m_isDragging`, `m_isPanning`, `m_lastMouseX`, `m_lastMouseY` fields never set or read
  - `LibraryPanel` has unused `m_contextMenuModelIndex` field
  - `ProjectPanel::refresh()` is empty method, never called
  - `UIManager` file dialog stubs `openFileDialog()` and `saveFileDialog()` always return false
  - `icons.h` has 93 lines of icon constants but no icon font is loaded
- **Files**: `src/ui/panels/viewport_panel.h:53-56`, `src/ui/panels/library_panel.h:59`, `src/ui/panels/project_panel.cpp:29-31`, `src/ui/ui_manager.cpp:258-268`, `src/ui/icons.h`
- **Impact**: Code clutter increases maintenance burden and confusion. Dead stubs suggest incomplete features.
- **Fix approach**: Remove unused fields and methods. Implement icon font or remove icon system stubs.

## Known Bugs

### ViewCube Recomputed Every Frame
- **Symptoms**: Viewport ViewCube is recomputed every frame (8 vertices, 6 faces, rotation matrix, depth sort, hit test detection) even when camera is stationary.
- **Files**: `src/ui/panels/viewport_panel.cpp:279-451`
- **Cause**: No cache layer; logic executes every render cycle unconditionally.
- **Workaround**: Performance impact is minor for modern GPUs but adds unnecessary CPU work.
- **Fix approach**: Cache ViewCube geometry and recompute only on camera change detection.

### Angle Wrapping O(n) in Camera
- **Symptoms**: Camera yaw/pitch wrapping uses loop instead of modulo: `while (m_yaw > 360) m_yaw -= 360;`
- **Files**: `src/render/camera.cpp:38-41`
- **Cause**: Inefficient pattern when angles become very large (edge case).
- **Workaround**: Works correctly, just slower than necessary.
- **Fix approach**: Use `fmod(m_yaw, 360.0f)` for O(1) wrapping.

### Framebuffer Move Constructor Incomplete
- **Symptoms**: Moved-from framebuffer object retains width/height values while GL handles are zeroed, leaving dimensions inconsistent with resource state.
- **Files**: `src/render/framebuffer.cpp:12-21`
- **Cause**: Move constructor didn't zero width/height in source object.
- **Workaround**: None — code works but moved-from object is in undefined state.
- **Fix approach**: Zero width/height in moved-from object for consistency.

### Shader Uniform Cache Stores -1
- **Symptoms**: `glGetUniformLocation` failures (returns -1 for not found) are cached identically to valid locations. Subsequent lookups never re-attempt.
- **Files**: `src/render/shader.cpp:94-103`
- **Cause**: Cache doesn't distinguish between "uniform not found" (-1) and "cached valid location".
- **Workaround**: Only affects unused uniforms; existing code doesn't try to update invalid uniforms.
- **Fix approach**: Don't cache -1, use `std::optional<GLint>` or similar.

## Security Considerations

### LIKE Injection in Database Searches
- **Risk**: User search terms concatenated with `%` wildcards without escaping. Characters `%` and `_` in input not escaped, allowing LIKE metacharacters to pass through. Searching "100%" matches everything (% is wildcard).
- **Files**: `src/core/database/model_repository.cpp:103,139` (findByName), `project_repository.cpp:69` (findByName)
- **Current mitigation**: Parameterized queries are used for the search term itself, but `%` wildcards added in C++ string concatenation before binding.
- **Recommendations**: Implement LIKE escape function: replace `%` with `\%`, `_` with `\_`, use `ESCAPE '\'` in SQL. Example: `... LIKE ? ESCAPE '\'` where ? is escaped input.

### Integer Overflow in STL Binary Parsing
- **Risk**: Triangle count from untrusted file data multiplied by 50 without prior bounds check. Crafted STL file with `triangleCount=0xFFFFFFFF` could wrap the size validation, enabling out-of-bounds read.
- **Files**: `src/core/loaders/stl_loader.cpp:88-99`
- **Current mitigation**: Overflow guard is now in place: `if (triangleCount > (SIZE_MAX - 84) / 50)` before multiplication.
- **Recommendations**: Guard is good. Keep in place. Apply same pattern to all binary loaders (OBJ, 3MF).

### Path Traversal in File Operations
- **Risk**: No validation that loaded file paths don't escape library root. Untrusted metadata containing `../../etc/passwd` could be written/read outside intended directory.
- **Files**: `src/core/archive/archive.cpp`, `src/core/export/exporter.cpp`
- **Current mitigation**: File path sanitization added.
- **Recommendations**: Maintain path canonicalization (use `std::filesystem::canonical()`) before all file I/O with user-influenced paths.

## Performance Bottlenecks

### GPU Mesh Cache Improves Frame Rate
- **Problem**: Previously uploaded mesh per frame with no caching. Fixed in recent refactoring.
- **Files**: `src/render/renderer.cpp:82-89`
- **Current state**: Hash-keyed mesh cache now caches GPUMesh uploads by mesh content hash.
- **Status**: RESOLVED — Cache layer eliminates redundant uploads.

### Vector Reserve for Large Loaders
- **Problem**: STL loader reserves capacity via `mesh->reserve()` to prevent reallocations. OBJ and 3MF loaders did not, causing repeated vector reallocations on large files.
- **Files**: `src/core/loaders/obj_loader.cpp:29-35`, `src/core/loaders/threemf_loader.cpp`
- **Current state**: Reserve calls added to OBJ and 3MF loaders with size-based estimates.
- **Status**: RESOLVED — Pre-allocation prevents reallocation thrashing.

### Bounds Recalculation Efficiency
- **Problem**: `centerOnOrigin()` and `normalizeSize()` each iterated all vertices to recalculate bounds, then applied transform.
- **Files**: `src/core/mesh/mesh.cpp:72-89`
- **Current state**: Mathematical bounds transformation added, avoiding full vertex iteration.
- **Status**: RESOLVED — Bounds updated via matrix transformation, O(1) instead of O(n).

### No GL Error Checking on Buffer Uploads
- **Problem**: `uploadMesh()` calls `glGen*`, `glBufferData`, `glVertexAttribPointer` with zero error checks. Failures silently pass.
- **Files**: `src/render/renderer.cpp:173-217`
- **Current state**: GL_CHECK macro added for critical calls.
- **Status**: RESOLVED — GL_CHECK validates error state post-operation.

## Fragile Areas

### Mesh Validation Post-Load
- **Files**: `src/core/loaders/stl_loader.cpp`, `src/core/loaders/obj_loader.cpp`, `src/core/loaders/threemf_loader.cpp`
- **Why fragile**: Loaders accept any parsed data without validation. Degenerate triangles (zero area), NaN coordinates, extreme values (1e30), inverted winding are not caught.
- **Safe modification**: Add `Mesh::validate()` method that checks: triangle area > epsilon, vertices in reasonable range (-1e6 to 1e6), normals not NaN. Call post-load.
- **Test coverage**: Test suite includes mesh validation tests but not all loaders tested against malformed input.

### Floating-Point Comparisons in Optimizers
- **Files**: `src/core/optimizer/bin_packer.cpp` (placement checks), `src/core/optimizer/guillotine.cpp` (placement checks)
- **Why fragile**: Rectangle dimensions compared with `>` and `<` without tolerance. Rounding errors accumulate near boundaries, causing valid placements to be rejected.
- **Safe modification**: Add epsilon tolerance (1e-5f) to all dimension comparisons. Example: `rect.x + rect.width + epsilon < stock.width`.
- **Test coverage**: Optimizer tests pass but edge cases with very small stock or near-boundary placements untested.

### Import Pipeline Thread Safety
- **Files**: `src/core/import/import_queue.cpp`
- **Why fragile**: `ImportProgress::currentFileName` is a `char[256]` written by worker thread, read by UI thread without synchronization. Comment claims "benign race" but undefined behavior.
- **Safe modification**: Protect with mutex or use atomic flag + locked string. Simplest: `std::atomic<bool>` dirty flag, UI only reads when flag stable.
- **Test coverage**: Import tests don't include concurrent access tests.

### Database Connection Single-Threaded
- **Files**: `src/core/database/database.cpp`, `src/core/database/database.h`
- **Why fragile**: Single SQLite connection object. If multiple threads execute queries simultaneously, undefined behavior (SQLite is thread-hostile with single connection).
- **Safe modification**: Add mutex guard on all `prepare()` calls, or implement connection pool.
- **Test coverage**: Database tests single-threaded. No concurrent access tests exist.

## Scaling Limits

### Single Worker Thread in Import Queue
- **Current capacity**: Single worker thread processes files sequentially. Large import (1000 files × 10 seconds each) = ~3 hours.
- **Limit**: Hits CPU-bound bottleneck on large multi-file imports.
- **Scaling path**: Implement configurable worker pool (default 2-4 threads) with thread-safe task distribution.
- **Effort**: Moderate — requires thread pool abstraction and proper synchronization.

### SQLite Single Connection
- **Current capacity**: Single database connection. Simultaneous queries block or error.
- **Limit**: Under high concurrency (multiple import workers + UI queries) will bottleneck.
- **Scaling path**: Implement connection pool (3-5 connections) with checkout/checkin semantics.
- **Effort**: High — requires connection pool abstraction and careful transaction handling.

### Mesh Cache Unbounded
- **Current capacity**: GPU mesh cache has no size limit. Cache key is mesh content hash.
- **Limit**: If user loads thousands of unique models, GPU memory exhausted without warning.
- **Scaling path**: Add LRU eviction policy with configurable max cache size (default 512MB).
- **Effort**: Moderate — requires LRU data structure and memory tracking.

## Missing Critical Features

### No Undo/Redo System
- **Problem**: No undo/redo for any operation. User can't recover from destructive operations (delete model, clear project).
- **Blocks**: Multi-step workflows are risky. Users hesitant to modify projects.
- **Approach**: Implement Command pattern with undo stack (max 50 commands). Start with ModelSelected, ProjectClear, ModelDelete.

### No Advanced Search
- **Problem**: Only basic `findByName` with SQL LIKE. No query language, filters, or pagination.
- **Blocks**: Large libraries (1000+ models) unsearchable by category, format, rating, date, or combinations.
- **Approach**: Add search engine with token-based query parsing (tag=wood, format=stl, rating>=4).

### No Watch Folder / Auto-Import
- **Problem**: No folder monitoring. User must manually drag/drop or import files.
- **Blocks**: Workflow integration; can't auto-ingest new designs.
- **Approach**: Add WatchFolderManager with file system notifications, size stability detection, auto-import.

### No CNC Control System
- **Problem**: No machine control. G-code can be analyzed but not sent to machine or monitored during run.
- **Blocks**: Machine feedback loop unavailable; real-time override/estop not possible.
- **Approach**: Implement NcSender REST client with WebSocket for machine state tracking (see OBSERVATION_REPORT.md for full spec).

### No Costing/Quoting System
- **Problem**: No cost calculation, quote generation, or customer management.
- **Blocks**: Business workflow (quoting, invoicing) requires external tools.
- **Approach**: Implement CostCalculator with material/tool/labor breakdown, CostEstimate persistence, PDF export (see OBSERVATION_REPORT.md).

### No Tool Database
- **Problem**: No CNC tool library. Feed/speed calculations manual.
- **Blocks**: Optimization requires manual tool input; no material/tool recommendations.
- **Approach**: Implement ToolRepository with geometry, feeds/speeds, Vectric tool import.

### No Advanced G-Code Analysis
- **Problem**: Basic path visualization only. No simulation, 3D view, or live machine tracking.
- **Blocks**: Can't verify tool path before run; post-run diagnostics limited.
- **Approach**: Add GCodeSimulator with playback speed control, 3D rotation, block-by-block inspection.

## Test Coverage Gaps

### Import Pipeline Concurrency Untested
- **What's not tested**: Concurrent import + database query, concurrent imports from multiple sources.
- **Files**: `src/core/import/import_queue.h/cpp`, `tests/test_import_pipeline.cpp`
- **Risk**: Race conditions hidden. Data corruption possible under concurrent load.
- **Priority**: HIGH — affects production robustness.

### Database Concurrent Access Untested
- **What's not tested**: Multiple threads executing queries simultaneously, transaction isolation.
- **Files**: `src/core/database/database.h/cpp`, `tests/test_database.cpp`
- **Risk**: Undefined behavior if UI + import threads access DB concurrently.
- **Priority**: HIGH — fundamental correctness.

### Optimizer Edge Cases Untested
- **What's not tested**: Very small stock (< 1mm), near-boundary placements, rounding edge cases, degenerate rectangles (zero width/height).
- **Files**: `src/core/optimizer/bin_packer.cpp`, `src/core/optimizer/guillotine.cpp`, `tests/test_optimizer.cpp`
- **Risk**: Silent placement failures on edge cases.
- **Priority**: MEDIUM — affects correctness for extreme inputs.

### Loader Malformed Input Untested
- **What's not tested**: Truncated files, corrupted vertices (NaN, infinity), invalid normals, extreme coordinate ranges (>1e10).
- **Files**: All loaders in `src/core/loaders/`, test suite exists but only tests valid inputs.
- **Risk**: Crashes or undefined behavior on malformed files.
- **Priority**: MEDIUM — robustness against user files.

### UI Theme and Rendering Untested
- **What's not tested**: Theme switching, multi-monitor rendering, window resize corner cases.
- **Files**: `src/ui/panels/`, `src/ui/theme.cpp`
- **Risk**: UI glitches on user systems.
- **Priority**: LOW — visual issues only.

---

*Concerns audit: 2026-02-08*
