# Coding Conventions

**Analysis Date:** 2026-02-19

## Naming Patterns

**Files:**
- Headers: `PascalCase.h` (e.g., `database.h`, `event_bus.h`, `model_repository.h`)
- Source: `snake_case.cpp` corresponding to header names (e.g., `database.cpp` matches `database.h`)
- Test files: `test_<component>.cpp` (e.g., `test_database.cpp`, `test_event_bus.cpp`)

**Functions:**
- Public methods: `camelCase` (e.g., `findById()`, `insertModel()`, `isOpen()`)
- Private helper methods: `camelCase` with `m_` prefix for members (e.g., `m_db`, `m_handlers`)
- Free functions in utility modules: `camelCase` (e.g., `trim()`, `split()`, `toLower()`)

**Variables:**
- Member variables: `m_<name>` prefix with camelCase (e.g., `m_db`, `m_handlers`, `m_tmpDir`)
- Local variables: `camelCase` (e.g., `callCount`, `receivedId`, `triCount`)
- Constants: `camelCase` (e.g., `kFormatBufSize = 1024`)
- STL containers/pointers: no special prefix, use type hints in names (e.g., `handlers`, `allModels`, `stmt`)

**Types:**
- Classes: `PascalCase` (e.g., `Database`, `EventBus`, `ModelRepository`, `LibraryManager`)
- Structs: `PascalCase` (e.g., `ModelRecord`, `ImportResult`, `Color`)
- Type aliases: `PascalCase` (e.g., `i64`, `u32`, `Vec3`, `MeshPtr`)
- Enums: `PascalCase` with values in `PascalCase` (e.g., `Level::Debug`, `MaterialCategory::Wood`)
- Namespaces: `lowercase` for nested utility namespaces (e.g., `dw::str`, `dw::file`, `dw::threading`, `dw::log`)

## Code Style

**Formatting:**
- Tool: clang-format (config: `.clang-format`)
- Indentation: 4 spaces, never tabs
- Column limit: 100 characters
- Line endings: LF (Unix style)

**Linting:**
- Enforced via lint compliance test: `test_lint_compliance.cpp`
- Checks for trailing whitespace, missing newlines, line length violations
- Run as part of test suite

**Brace Style:**
- Attach braces to same line: `void foo() {`
- Single-line blocks only if empty: `if (x) {}`
- Multi-line: each statement on own line

**Pointer/Reference Alignment:**
- Pointers and references left-aligned: `int* ptr`, `int& ref`, not `int *ptr` or `int & ref`
- This matches pointer ownership being part of the type system

## Import Organization

**Order (clang-format enforced):**
1. C++ standard library (`<algorithm>`, `<functional>`, `<memory>`, `<vector>`)
2. Third-party headers (`<glm/glm.hpp>`, `<gtest/gtest.h>`, `<sqlite3.h>`)
3. Project headers in quotes (`"core/database/database.h"`, `"core/types.h"`)

**Path Aliases:**
- All includes from project use `"core/..."` prefix (no relative paths)
- Include directories configured in CMake: `${CMAKE_SOURCE_DIR}/src` and `${CMAKE_BINARY_DIR}/generated`
- Leaf modules (tests) include from root: `#include "core/utils/log.h"` not `#include "../core/utils/log.h"`

**Namespace Organization:**
- Main namespace: `dw` (Digital Workshop)
- Nested namespaces for utility categories: `dw::str`, `dw::file`, `dw::threading`, `dw::log`
- Anonymous namespace `namespace {}` in `.cpp` files for file-local helpers and test fixtures
- Each header fully qualified: `namespace dw { ... }` with closing comment `} // namespace dw`

## Error Handling

**Patterns:**
- **Optional returns**: Use `std::optional<T>` for operations that may fail (e.g., `findById(i64 id)` returns `std::optional<ModelRecord>`)
- **Check with `.has_value()`**: `if (result.has_value()) { auto value = result.value(); }`
- **Database operations**: Return `bool` for success/failure; use `lastError()` method to get error message
- **No exceptions for control flow**: C++ exceptions not used for expected failures, only for truly exceptional conditions
- **Assertions for preconditions**: Use `ASSERT_*` macros in tests to catch setup failures, `EXPECT_*` for actual test assertions

**Example pattern from codebase:**
```cpp
// Option 1: optional return with value() call
std::optional<ModelRecord> found = m_repo->findById(42);
if (found.has_value()) {
    processModel(found.value());
}

// Option 2: optional return with optional chaining (C++23 style)
if (auto found = m_repo->findById(42)) {
    processModel(found.value());
}

// Option 3: database bool return with error check
if (!m_db.execute(sql)) {
    logError("Failed to execute: " + m_db.lastError());
}
```

## Logging

**Framework:** Custom module-based logging in `core/utils/log.h`
- Levels: `Debug`, `Info`, `Warning`, `Error`
- Default: Info in release, Debug in debug builds

**Patterns:**
- Use module string as first parameter: `log::info("LibraryManager", "Imported model");`
- Formatted logging: `log::infof("Database", "Query returned %d rows", count);`
- Printf-style with format specifiers: `%d` (int), `%ld` (long), `%f` (float), `%s` (string)
- Level check optimization: use `if (getLevel() <= Level::Debug)` before expensive logging

**Example from codebase:**
```cpp
log::debug("EventBus", "Published event");
log::warningf("Database", "Connection pool exhausted (%d/%d)", active, max);
log::error("LibraryManager", "Failed to load mesh: " + filePath.string());
```

## Comments

**When to Comment:**
- **No obvious comments**: Don't comment what code already says: `i++; // increment i` is noise
- **Why comments**: Explain non-obvious reasoning: why a workaround exists, why locking is needed, why a specific order matters
- **Complex logic**: For algorithms or business rules that aren't immediately clear
- **Public API docs**: Use header comments for class/function purpose

**JSDoc/Doxygen style:**
- Not enforced; optional for public APIs
- When used, follow standard pattern:
```cpp
// Load a 3D model mesh from disk
// Returns nullptr if file not found or format unsupported
MeshPtr loadMesh(const Path& filePath);
```

**Inline comments:**
- Sparingly; prefer self-documenting code
- Use `// NOTE:` for important context
- Use `// THREADING:` to document concurrency contracts
- Single-line with `//`, no block `/* */` comments for inline

## Function Design

**Size:** Keep functions focused on single responsibility
- Small helpers: 5-20 lines typical
- Complex logic: up to 50-100 lines acceptable if cohesive
- Decompose larger functions into named helpers

**Parameters:**
- Prefer const references for input parameters: `const ModelRecord& model`
- Use pointers for optional output: `void getStatus(int* outCode)` (rare)
- Avoid output parameters; prefer return types or std::optional

**Return Values:**
- Use `std::optional<T>` for operations that may not produce a result
- Return by value for small types (primitives, Vec3, Color)
- Return const reference for large read-only structures when available
- Return bool only for yes/no operations

**Example pattern:**
```cpp
// Prefer this:
std::optional<ModelRecord> findById(i64 id);

// Over this:
bool findById(i64 id, ModelRecord& outRecord);
```

## Module Design

**Exports:**
- Headers declare public interface; keep implementation in `.cpp`
- Use `#pragma once` guards (not macro guards)
- Header must be self-contained: include all dependencies it references

**Barrel Files:**
- Not used; include directly from source modules
- No single `#include "core/all.h"` pattern

**Class Design:**
- Public constructor that sets up dependencies (e.g., `explicit ModelRepository(Database& db)`)
- Member variables private with `m_` prefix
- No getters/setters unless needed; expose via methods
- RAII pattern for resources (constructors acquire, destructors release)

**Example from codebase (ModelRepository):**
```cpp
class ModelRepository {
  public:
    explicit ModelRepository(Database& db);
    std::optional<i64> insert(const ModelRecord& model);
    std::optional<ModelRecord> findById(i64 id);
    bool update(const ModelRecord& model);

  private:
    static ModelRecord rowToMaterial(Statement& stmt);
    Database& m_db;
};
```

## Type System

**Type Aliases (in `core/types.h`):**
- Signed integers: `i8`, `i16`, `i32`, `i64` (replacing `int8_t`, `int32_t`, etc.)
- Unsigned integers: `u8`, `u16`, `u32`, `u64`
- Floating point: `f32` (float), `f64` (double)
- Size type: `usize` (std::size_t)
- 3D math: `Vec2`, `Vec3`, `Vec4`, `Mat4` (GLM aliases)
- Filesystem: `Path` (std::filesystem::path alias)
- Result type: `Result<T>` = `std::optional<T>`
- Buffer type: `ByteBuffer` (std::vector<u8>)

**Use these types consistently across the codebase** - never mix `int32_t` with `i32`, or `float` with `f32`.

## Standard Library Usage

**Preferred patterns:**
- Containers: `std::vector`, `std::string`, `std::optional`, `std::unordered_map`
- Memory: `std::unique_ptr`, `std::shared_ptr`, explicit ownership semantics
- Strings: `std::string_view` for read-only parameters to avoid copies
- Algorithms: use STL algorithms (`std::find`, `std::any_of`, etc.) sparingly; simple loops are often clearer

**Avoid:**
- Raw pointers for ownership (use smart pointers)
- C-style arrays (use `std::vector`)
- `printf` style logging (use log framework)

---

*Convention analysis: 2026-02-19*
