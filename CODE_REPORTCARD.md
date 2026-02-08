# Digital Workshop — Code Report Card

**Date**: 2026-02-08
**Build**: Clean (0 errors, 0 warnings from project code)
**Tests**: 377/377 passing (100%)

---

## Overall Grade: A-

| Category | Grade | Notes |
|----------|-------|-------|
| **Safety & Security** | A | All CRITICAL/HIGH issues resolved |
| **Correctness** | A | Full mesh validation, FK constraints, proper error handling |
| **Performance** | A- | GPU mesh cache, vector reserves; ViewCube caching deferred |
| **Code Organization** | A- | Good module boundaries; Application class is large (1047 lines) |
| **Style & Formatting** | A | 0 clang-format violations, 0 lint failures |
| **Test Coverage** | A | 377 tests across 35 suites covering all modules |
| **Dependencies** | A | GLM replaces custom math; all deps via FetchContent |
| **Memory Safety** | A+ | Zero raw new/delete, RAII everywhere, smart pointers |

---

## Codebase Metrics

| Metric | Value |
|--------|-------|
| Source files | 116 (.h + .cpp) |
| Source lines | 15,247 |
| Test files | 28 |
| Test lines | 4,677 |
| Test cases | 377 |
| Test suites | 35 |
| Test:Source ratio | 1:3.3 |
| Largest file | application.cpp (1,047 lines) |
| Build warnings (our code) | 0 |
| Build warnings (deps) | 428 (all GLM upstream) |

### Lines by Module

| Module | Lines | Files |
|--------|------:|------:|
| ui (panels, dialogs, widgets, theme) | 4,205 | 31 |
| core/database | 1,684 | 10 |
| render | 1,426 | 13 |
| app | 1,423 | 6 |
| core/loaders | 1,041 | 9 |
| core/utils | 875 | 6 |
| core/config | 824 | 6 |
| core/mesh | 658 | 6 |
| core/gcode | 633 | 5 |
| core/optimizer | 627 | 8 |
| core/import | 410 | 3 |
| core/archive | 315 | 2 |
| core/library | 295 | 2 |
| core/project | 265 | 2 |
| core/export | 249 | 2 |
| core/paths | 210 | 2 |

---

## Code Quality Indicators

| Indicator | Count | Status |
|-----------|------:|--------|
| TODO/FIXME/HACK/XXX markers | 0 | Clean |
| Raw new/delete allocations | 0 | Clean (all `= delete` for copy/move) |
| C-style casts | 0 | Clean (uses static_cast/reinterpret_cast) |
| `using namespace` in headers | 0 | Clean |
| Backward compatibility shims | 0 | Clean (removed per policy) |
| Deprecated code | 0 | Clean |
| clang-format violations | 0 | Clean |
| Pragma once coverage | 100% | All headers |

---

## Resolved Issues (38/42 Working Points)

### CRITICAL — All Fixed
- **WP-01**: Integer overflow in STL binary parsing (overflow guard added)
- **WP-02**: Strict aliasing violation in mesh hashing (memcpy fix)
- **WP-03**: SQL injection via string interpolation (parameterized queries)
- **WP-04**: Per-frame GPU mesh upload/destroy (hash-keyed cache added)
- **WP-05**: Custom math library replaced with GLM type aliases

### HIGH — All Fixed
- **WP-06**: Duplicated 2D canvas pan/zoom (Canvas2D widget extracted)
- **WP-07**: Duplicated dialog rendering patterns (shared base)
- **WP-08**: Duplicated optimizer part expansion (optimizer_utils.h)
- **WP-09**: Duplicated spherical coordinate math (types.h/cpp)
- **WP-11**: Duplicated filesystem error handling (fsOp/fsOp2 helpers)
- **WP-12**: Path traversal in file operations (sanitization added)
- **WP-13**: SQL LIKE injection (escape function added)
- **WP-14**: Unchecked buffer sizes in STL loader (bounds validation)
- **WP-15**: Missing null checks after mesh allocation (error propagation)

### MEDIUM — All Addressed
- **WP-16 through WP-21**: Dead code removal (YAGNI cleanup)
- **WP-23**: Simplified complex control flow (KISS)
- **WP-25**: Redundant bounds recalculation eliminated
- **WP-26**: Over-engineered abstractions simplified
- **WP-27**: Missing edge case handling added
- **WP-28**: GL error checking added to uploadMesh()
- **WP-29**: Log buffer format deduplication (detail::formatAndLog)
- **WP-30**: Buffer overflow prevention in formatted logging
- **WP-31**: Duplicated string utilities consolidated
- **WP-32**: Mesh validation method (NaN, OOB indices, degenerate tris)
- **WP-33**: Vector reserve() for loaders (OBJ, 3MF capacity pre-allocation)
- **WP-34**: Correctness fixes for edge cases
- **WP-35**: Consistent naming conventions
- **WP-36**: Consistent error handling patterns

### LOW — Addressed
- **WP-38**: `[[nodiscard]]` on critical return values (database, shader, file_utils)
- **WP-39**: `std::string_view` parameters in repository APIs
- **WP-40**: Cost repository test suite (31 test cases)
- **WP-41**: Mathematical bounds update in centerOnOrigin() (avoids full iteration)
- **WP-42**: Additional correctness improvements

### Deferred (4 items)
- **WP-10**: SQL boilerplate reduction — Low risk/reward, would require new abstraction
- **WP-22**: Application god class — 1,047 lines, needs architectural redesign
- **WP-24**: ViewCube texture caching — Requires GL rendering context for testing
- **WP-37**: Error logging policy documentation — Style documentation only

---

## Additional Bugs Found & Fixed During Refactoring

| Bug | Severity | Fix |
|-----|----------|-----|
| OBJ loader `v//vn` face format broken | Medium | Rewrote face parser to handle empty split tokens |
| Cost repo tests FK constraint violations | Low | Tests now create project records before referencing |
| Schema tests used wrong version constant | Low | Updated to match CURRENT_VERSION = 2 |

---

## Architecture Strengths

- **RAII throughout**: Database, Shader, Framebuffer, Transaction all use RAII with deleted copy constructors
- **Smart pointers**: MeshPtr = shared_ptr<Mesh>, no raw ownership
- **Namespace isolation**: All code in `dw::` namespace, no pollution
- **Module boundaries**: Clean separation between core, render, ui, app layers
- **Test infrastructure**: GoogleTest with fixture classes, lint compliance tests, multi-tier test coverage
- **Build system**: CMake with FetchContent for all dependencies (GLM, SDL2, ImGui, SQLite, GoogleTest)

## Known Technical Debt

1. **Application class size** (WP-22): At 1,047 lines, `application.cpp` acts as a god class orchestrating all subsystems. Should eventually be decomposed into smaller managers.
2. **ViewCube caching** (WP-24): ViewCube re-renders each frame; could cache to texture. Blocked on GL context testing.
3. **SQL boilerplate** (WP-10): Repository classes have repetitive prepare/bind/execute patterns that could benefit from a query builder, but current pattern is explicit and readable.
4. **GLM upstream warnings**: 428 warnings from GLM headers (sign conversion, duplicate branches) — not actionable without patching upstream.
