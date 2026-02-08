# Coding Conventions

**Analysis Date:** 2026-02-08

## Naming Patterns

**Files:**
- Headers: `snake_case.h` (e.g., `string_utils.h`, `stl_loader.h`)
- Implementation: `snake_case.cpp` (e.g., `mesh.cpp`, `database.cpp`)
- Pattern: lowercase with underscores for multi-word names

**Functions:**
- Member functions: `camelCase` (e.g., `recalculateBounds()`, `bindText()`, `formatFileSize()`)
- Free functions: `camelCase` (e.g., `trim()`, `toLower()`, `loadSTL()`)
- Private helper functions: `camelCase` (e.g., `loadBinary()`, `isBinary()`)
- Getter/setter patterns: `propertyName()` and `setPropertyName()` (e.g., `vertices()`, `setName()`)

**Variables:**
- Local variables: `camelCase` (e.g., `vertexCount`, `currentTime`, `meshPtr`)
- Parameters: `camelCase` (e.g., `moduleName`, `formatString`, `archivePath`)
- Function parameters with trailing underscore if conflicts with member: `name_` (e.g., in Color constructor: `r_`, `g_`, `b_`)

**Types:**
- Classes: `PascalCase` (e.g., `Mesh`, `Database`, `STLLoader`, `MeshLoader`)
- Enums: `PascalCase` (e.g., `Level`, `CostCategory`)
- Structs: `PascalCase` (e.g., `Color`, `LoadResult`, `ArchiveResult`, `Vertex`)
- Type aliases: `PascalCase` (e.g., `Vec2`, `Vec3`, `Mat4`, `ByteBuffer`, `Path`)
- Namespaces: `lower_case` (e.g., `dw`, `dw::log`, `dw::str`)

**Constants and Enumerators:**
- Global constants: `UPPER_CASE` (e.g., `kFormatBufSize`, `kTruncationSuffix` - with `k` prefix by convention)
- Enum values: `PascalCase` (e.g., `Level::Debug`, `CostCategory::Material`)

**Member Variables:**
- Private/protected: `m_camelCase` prefix (e.g., `m_vertices`, `m_bounds`, `m_logFile`, `m_autoOriented`)
- Public: avoid direct public data; use accessors

**Type Aliases in Core Types:**
```cpp
// Integer types
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Floating point
using f32 = float;
using f64 = double;

// Size type
using usize = std::size_t;

// 3D math types
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;
```
Defined in `src/core/types.h` - use these consistently throughout codebase.

## Code Style

**Formatting:**
- Tool: `clang-format` (enforced via `.clang-format` config)
- Indentation: 4 spaces (never tabs)
- Column limit: 100 characters
- Configuration file: `.clang-format`

**Linting:**
- Tool: `clang-tidy` with custom checks
- Configuration file: `.clang-tidy`
- Policy: ZERO suppression - fix code rather than add NOLINT suppressions
- Enforced checks: `bugprone-*`, `cppcoreguidelines-*`, `misc-*`, `modernize-*`, `performance-*`, `readability-*`
- Specific exclusions: magic numbers, pointer arithmetic (legacy code patterns), reinterpret casts, union access
- Warnings treated as errors: All checks converted to errors for CI/build

**Brace Style:**
- Attach braces to control structures (no next-line opening braces)
- Single-statement blocks on separate line (never inline)
- Empty blocks allowed on single line
- Constructor initializers: break before comma, one per line

**Spacing:**
- No space after C-style casts
- Space after template keyword
- Space before assignment operators
- Space before parentheses in control statements (if, while, for, switch)
- No spaces in empty parentheses `()`
- No spaces in angle brackets `<>`
- No spaces in parentheses `()`
- No spaces in square brackets `[]`

**Pointer and Reference Alignment:**
- Left-aligned: `const Vertex* v` not `const Vertex *v`
- Applies to both pointers and references

**Include Organization:**
Priority order enforced by clang-format:
1. C++ standard library includes: `<cstdint>`, `<string>`, etc.
2. Third-party includes: `<glm/glm.hpp>`, `<gtest/gtest.h>`, etc.
3. Project headers: `"core/types.h"`, `"mesh/mesh.h"`, etc.
Sort within each category alphabetically.

## Import Organization

**Pattern:** Three categories sorted by priority

```cpp
// 1. Standard library (C and C++)
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// 2. Third-party libraries
#include <glm/glm.hpp>
#include <gtest/gtest.h>
#include <sqlite3.h>

// 3. Project headers (relative to src/)
#include "core/types.h"
#include "core/mesh/mesh.h"
#include "core/utils/log.h"
```

## Error Handling

**Pattern 1: LoadResult struct with optional mesh**
Used in loaders and operations returning optional data with error context.
- Definition: `struct LoadResult { MeshPtr mesh; std::string error; bool success() const; }`
- Location: `src/core/loaders/loader.h`
- Example: `src/core/loaders/stl_loader.cpp` - returns `LoadResult` with mesh or error message
- Usage: Check `if (result.success())` or `if (result.mesh)` before accessing data

```cpp
struct LoadResult {
    MeshPtr mesh;
    std::string error;

    bool success() const { return mesh != nullptr; }
    operator bool() const { return success(); }
};

LoadResult load(const Path& path) {
    auto data = readFile(path);
    if (!data) {
        return LoadResult{nullptr, "Failed to read file"};
    }
    // ... parse data
    return LoadResult{std::make_shared<Mesh>(vertices, indices), ""};
}
```

**Pattern 2: ArchiveResult struct with success flag and error message**
Used in operations where error messages are critical for user feedback.
- Definition: `struct ArchiveResult { bool success; std::string error; std::vector<std::string> files; }`
- Location: `src/core/archive/archive.h`
- Factory methods: `ArchiveResult::ok()`, `ArchiveResult::fail()`
- Example: `src/core/archive/archive.cpp`

```cpp
struct ArchiveResult {
    bool success = false;
    std::string error;
    std::vector<std::string> files;

    static ArchiveResult ok(std::vector<std::string> files = {}) {
        return {true, "", std::move(files)};
    }

    static ArchiveResult fail(const std::string& error) {
        return {false, error, {}};
    }
};
```

**Pattern 3: Database::Statement with bool return codes**
For database operations - check return value before proceeding.
- Returns: `true` for success, `false` for failure
- Error access: `Database::lastError()` for context
- Location: `src/core/database/database.h`
- Example: `src/tests/test_database.cpp`

```cpp
auto stmt = db.prepare("INSERT INTO items (name) VALUES (?)");
if (!stmt.bindText(1, "value")) {
    LOG_ERROR("db", "Bind failed: {}", db.lastError());
    return;
}
if (!stmt.execute()) {
    LOG_ERROR("db", "Execute failed: {}", db.lastError());
    return;
}
```

**Pattern 4: Optional results with std::optional**
Used in `src/core/types.h` - generic optional result type.
- Template alias: `template <typename T> using Result = std::optional<T>;`
- Check with: `if (result.has_value())` or `if (result)`
- Access with: `*result` or `result.value()`

**Policy on error messages:**
- Always include context (what operation was attempted)
- Avoid generic "Error" messages - be specific
- Include relevant file paths or identifiers
- Use lowercase for message body, capitalize module names

## Logging

**Framework:** Custom `dw::log` namespace in `src/core/utils/log.h`

**Levels:**
- `Level::Debug` - Development-only diagnostics (disabled in release builds)
- `Level::Info` - General operational information (default minimum)
- `Level::Warning` - Recoverable issues or suspicious conditions
- `Level::Error` - Unrecoverable errors or critical failures

**API:**
```cpp
dw::log::debug(module, message);    // Simple message
dw::log::info(module, message);
dw::log::warning(module, message);
dw::log::error(module, message);

// Formatted logging (printf-style)
dw::log::debugf(module, "format", args...);
dw::log::infof(module, "format", args...);
dw::log::warningf(module, "format", args...);
dw::log::errorf(module, "format", args...);
```

**Patterns:**
- Module name first parameter: typically source filename or subsystem name (e.g., "stl_loader", "database", "renderer")
- Use formatted variants (`debugf`, `infof`, etc.) for complex messages
- Set log file with `dw::log::setLogFile(path)` if persistent logging needed
- Example from `src/core/utils/log.cpp`:

```cpp
std::cerr << color << "[" << timestamp << "] [" << levelStr << "] "
          << "[" << module << "] " << reset << message << std::endl;
```

## Comments

**When to Comment:**
- Complex algorithms or non-obvious logic (e.g., auto-orientation math in `src/core/mesh/mesh.cpp`)
- Why a decision was made, not what the code does
- Algorithm explanations (e.g., "Compute face normals before smoothing")
- Non-standard patterns or workarounds
- Avoid comments for obvious code: `x = x + 1; // increment x` is noise

**Format:**
- Single-line: `// Comment here`
- Block style: Each line prefixed with `//`
- Trailing comments for brief clarifications: `const int MAX_SIZE = 256; // per STL spec`

**JSDoc/TSDoc:**
Not used - this is a C++ codebase, not JavaScript/TypeScript. Use inline comments in headers for API documentation.

**Header Documentation Pattern:**
Brief comment above declarations:
```cpp
// Trim whitespace from both sides
std::string trim(std::string_view s);

// Create a copy
Mesh clone() const;

// Validate mesh integrity (checks for NaN, out-of-bounds indices, degenerate triangles)
// Returns true if mesh passes all checks. Logs warnings for issues found.
bool validate() const;
```

## Function Design

**Size Guidelines (from clang-tidy config):**
- Line threshold: 50 lines per function
- Parameter threshold: 4 parameters maximum
- Cognitive complexity threshold: 25
- Functions exceeding these should be refactored

**Parameters:**
- Pass small types (int, float, enums) by value
- Pass objects by `const` reference: `const std::string&`, `const Mesh&`
- Use `std::string_view` for string parameters to avoid temporary copies
- Output parameters allowed but prefer return values or optional/result structs
- Example from `src/core/utils/string_utils.h`:
```cpp
std::string trim(std::string_view s);
std::vector<std::string> split(std::string_view s, char delimiter);
bool parseInt(std::string_view s, int& out);  // Output parameter for parse results
```

**Return Values:**
- Prefer explicit return values over output parameters
- Use `[[nodiscard]]` attribute for important return values
- Return structures with context when needed (LoadResult, ArchiveResult)
- Example with nodiscard:
```cpp
[[nodiscard]] bool bindText(int index, const std::string& value);
[[nodiscard]] bool execute();
[[nodiscard]] LoadResult load(const Path& path);
```

**RAII Pattern (Resource Acquisition Is Initialization):**
Used extensively for resource management:
- Statement wrapper auto-cleanup in `src/core/database/database.h`
- Database connection auto-close in destructor
- File handles managed through RAII
- Move semantics for efficient resource transfer

```cpp
class Statement {
    Statement(const Statement&) = delete;              // No copying
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;             // Move allowed
    Statement& operator=(Statement&& other) noexcept;
};
```

## Module Design

**Header Structure:**
- Begin with `#pragma once` (not include guards)
- Includes in standard/third-party/project order
- Namespace declaration: `namespace dw { ... }`
- Closing brace with namespace comment: `} // namespace dw`

**Exports:**
- Public API in header files
- Implementation in `.cpp` files
- Private functions in anonymous namespace in `.cpp`

**Barrel Files:**
Not used in this codebase - import individual headers.

**Const Correctness:**
- Member functions that don't modify state: `const`
- Parameters that aren't modified: `const`
- Return const references when appropriate
- Example from `src/core/mesh/mesh.h`:
```cpp
const std::vector<Vertex>& vertices() const { return m_vertices; }
std::vector<Vertex>& vertices() { return m_vertices; }
const AABB& bounds() const { return m_bounds; }
```

---

*Convention analysis: 2026-02-08*
