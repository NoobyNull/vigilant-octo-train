# Coding Standards

## Core Principles (Priority Order)

### 1. KISS (Keep It Simple, Stupid)

The simplest solution that works is the correct solution.

- No premature abstraction
- No "clever" code
- One obvious way to do things
- If you have to explain it, simplify it

```cpp
// BAD - over-engineered factory pattern for 3 formats
class MeshLoaderFactory {
    std::map<std::string, std::function<Loader*()>> m_registry;
public:
    void registerLoader(std::string ext, std::function<Loader*()> factory);
    Loader* createLoader(const std::string& ext);
};

// GOOD - simple and direct
std::unique_ptr<Mesh> loadMesh(const std::filesystem::path& path) {
    auto ext = path.extension();
    if (ext == ".stl") return loadSTL(path);
    if (ext == ".obj") return loadOBJ(path);
    if (ext == ".3mf") return load3MF(path);
    return nullptr;
}
```

### 2. Modularization

**If code is used more than once and is non-trivial, extract it to a module.**

Modularization serves two purposes:
1. **Reuse** - Don't repeat yourself
2. **Size control** - Keep parent files manageable

#### When to Extract

| Condition | Action |
|-----------|--------|
| Used 1x, small | Keep inline |
| Used 1x, large (50+ lines) | Extract to reduce parent file size |
| Used 2x, any size | Extract to module |
| Used 3x+ | Always extract |

#### Module Organization

```
core/
├── mesh/
│   ├── mesh.h              # Main Mesh class
│   ├── mesh.cpp
│   ├── mesh_utils.h        # Extracted utilities (triangulation, etc.)
│   ├── mesh_utils.cpp
│   ├── bounds.h            # BoundingBox - used by mesh + renderer
│   └── bounds.cpp
```

#### Example: Extract for Reuse (2+ uses)

```cpp
// BEFORE - duplicated in two places
// In stl_loader.cpp
glm::vec3 computeNormal(const Triangle& t) {
    auto e1 = t.v1 - t.v0;
    auto e2 = t.v2 - t.v0;
    return glm::normalize(glm::cross(e1, e2));
}

// In obj_loader.cpp (same code again)
glm::vec3 computeNormal(const Triangle& t) { /* identical */ }

// AFTER - extracted to module
// mesh_utils.h
namespace dw::mesh {
    auto computeNormal(const Triangle& t) -> glm::vec3;
}

// Used in both loaders
#include "core/mesh/mesh_utils.h"
```

#### Example: Extract for Size (large, even if 1 use)

```cpp
// BEFORE - 300 line function bloating the file
void Renderer::setupAllShaders() {
    // 50 lines of vertex shader setup
    // 50 lines of fragment shader setup
    // 50 lines of geometry shader setup
    // 50 lines of compute shader setup
    // 100 lines of pipeline creation
}

// AFTER - extracted to keep renderer.cpp focused
// shader_setup.h / shader_setup.cpp
namespace dw::render {
    void setupVertexShader(Renderer& r);
    void setupFragmentShader(Renderer& r);
    void setupGeometryShader(Renderer& r);
    void setupComputeShader(Renderer& r);
    void createPipeline(Renderer& r);
}

// renderer.cpp stays clean
void Renderer::setupAllShaders() {
    render::setupVertexShader(*this);
    render::setupFragmentShader(*this);
    render::setupGeometryShader(*this);
    render::setupComputeShader(*this);
    render::createPipeline(*this);
}
```

### 3. DRY (Don't Repeat Yourself)

A consequence of modularization. If you're copying code, you should be extracting it.

```cpp
// Copying code? Stop and extract.

// BAD - copy-paste
void importSTL(const Path& p) {
    LOG_INFO("Importing {}", p.string());
    auto data = readFile(p);
    // ... 20 lines of validation
}
void importOBJ(const Path& p) {
    LOG_INFO("Importing {}", p.string());
    auto data = readFile(p);
    // ... same 20 lines of validation
}

// GOOD - extracted module
// import_utils.h
namespace dw::import {
    auto validateAndLoad(const Path& p) -> std::optional<std::vector<uint8_t>>;
}

void importSTL(const Path& p) {
    auto data = import::validateAndLoad(p);
    if (!data) return;
    // STL-specific parsing
}
```

### 4. YAGNI (You Aren't Gonna Need It)

Don't build for hypothetical futures.

- No "just in case" parameters
- No plugin architectures unless required today
- No configuration for single-value settings
- Delete unused code, don't comment it out

```cpp
// BAD - YAGNI violation
class Renderer {
    bool m_enableVulkanBackend;      // "might need later"
    bool m_enableMetalBackend;       // "for future macOS"
    int m_maxLightSources;           // always 1 in practice
    bool m_enableRayTracing;         // "someday"
};

// GOOD - what we actually need today
class Renderer {
    // OpenGL 3.3, one directional light
};
```

---

## Size Limits

| Element | Limit | Rationale |
|---------|-------|-----------|
| `.cpp` file | 800 lines | Fits in mental model |
| `.h` file | 400 lines | Interface should be scannable |
| Function | 50 lines | One screen, one purpose |
| Class | 10-15 public methods | Single responsibility |
| Parameters | 4 max | Beyond that, use a struct |
| Nesting depth | 3 levels | Flatten with early returns |

### Flattening Deep Nesting

```cpp
// BAD - too nested
void processImport(const Path& p) {
    if (exists(p)) {
        if (isValid(p)) {
            auto data = read(p);
            if (!data.empty()) {
                // ... logic buried in nesting
            }
        }
    }
}

// GOOD - early returns, flat structure
void processImport(const Path& p) {
    if (!exists(p)) return;
    if (!isValid(p)) return;

    auto data = read(p);
    if (data.empty()) return;

    // Main logic here, not nested
}
```

---

## Language Standard

- **C++17** required
- Modern idioms: `auto`, smart pointers, `constexpr`, structured bindings
- No raw `new`/`delete` - RAII and smart pointers only
- Use `std::filesystem` for all path operations

---

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `ModelLoader` |
| Methods | camelCase | `loadModel()` |
| Variables | camelCase | `triangleCount` |
| Private members | m_ prefix | `m_vertices` |
| Constants | UPPER_CASE | `MAX_VERTICES` |
| Namespaces | lowercase | `dw` |
| Files | snake_case | `model_loader.cpp` |

### Naming Clarity

Names should make comments unnecessary.

```cpp
// BAD - needs comment
int d;  // elapsed time in days

// GOOD - self-documenting
int elapsedDays;

// BAD - abbreviations
auto calc_tm_est(const GCode& gc) -> float;

// GOOD - full words
auto calculateTimeEstimate(const GCode& gcode) -> float;

// BAD - generic
void process(Data& data);

// GOOD - specific
void triangulate(Mesh& mesh);
```

---

## File Organization

```
src/
├── main.cpp
├── app/                  # Application lifecycle
├── core/                 # Business logic (no GUI)
│   ├── loaders/
│   ├── mesh/
│   ├── gcode/
│   ├── optimizer/
│   └── database/
├── ui/                   # ImGui panels and widgets
│   ├── panels/
│   ├── dialogs/
│   └── viewport/
├── render/               # OpenGL rendering
└── platform/             # Platform-specific (minimal)
```

### Header Structure

```cpp
#pragma once

#include <memory>              // Standard library first
#include <vector>
#include <string>

#include <SDL2/SDL.h>          // Third-party second

#include "core/mesh.h"         // Project headers last
#include "core/types.h"

namespace dw {

class MeshLoader {
public:
    MeshLoader();
    ~MeshLoader();

    auto loadFromFile(const std::filesystem::path& path) -> std::unique_ptr<Mesh>;
    auto supportsFormat(std::string_view extension) const -> bool;

private:
    auto parseSTL(const std::filesystem::path& path) -> std::unique_ptr<Mesh>;
    auto parseOBJ(const std::filesystem::path& path) -> std::unique_ptr<Mesh>;

    std::vector<uint8_t> m_buffer;
};

} // namespace dw
```

---

## Formatting (Clang-Format)

**Automatic and mandatory.** Run on every file save. No exceptions.

### Configuration File

```yaml
# .clang-format
---
Language: Cpp
BasedOnStyle: LLVM

# Indentation
IndentWidth: 4
TabWidth: 4
UseTab: Never
IndentCaseLabels: false
IndentPPDirectives: BeforeHash
NamespaceIndentation: None

# Line length
ColumnLimit: 100

# Braces
BreakBeforeBraces: Attach
AllowShortBlocksOnASingleLine: Empty
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false

# Alignment
PointerAlignment: Left
ReferenceAlignment: Left
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true

# Spacing
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesInAngles: false
SpacesInParentheses: false
SpacesInSquareBrackets: false

# Includes
SortIncludes: true
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^<[a-z_]+>'        # C++ standard library
    Priority: 1
  - Regex: '^<.+>'             # Third-party
    Priority: 2
  - Regex: '^"'                # Project headers
    Priority: 3

# Other
BreakConstructorInitializers: BeforeComma
ConstructorInitializerAllOnOneLineOrOnePerLine: true
Cpp11BracedListStyle: true
DerivePointerAlignment: false
FixNamespaceComments: true
MaxEmptyLinesToKeep: 1
ReflowComments: true
SortUsingDeclarations: true
...
```

### Formatted Example

```cpp
namespace dw {

class Renderer {
public:
    Renderer();
    ~Renderer();

    void beginFrame();
    void endFrame();
    void renderMesh(const Mesh& mesh, const glm::mat4& transform);

private:
    void setupShaders();
    void createBuffers();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::unique_ptr<Shader> m_shader;
};

void Renderer::renderMesh(const Mesh& mesh, const glm::mat4& transform) {
    if (!mesh.isValid()) {
        return;
    }

    m_shader->bind();
    m_shader->setUniform("u_transform", transform);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
}

} // namespace dw
```

---

## Linting (Clang-Tidy)

**Zero tolerance for warnings. Fix the code, never suppress the check.**

### Configuration File

```yaml
# .clang-tidy
---
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  misc-*,
  modernize-*,
  performance-*,
  readability-*,
  -bugprone-easily-swappable-parameters,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay,
  -cppcoreguidelines-pro-bounds-constant-array-index,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-type-reinterpret-cast,
  -cppcoreguidelines-pro-type-union-access,
  -misc-non-private-member-variables-in-classes,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers,
  -readability-identifier-length

WarningsAsErrors: '*'

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelBack
  - key: readability-identifier-naming.VariableCase
    value: camelBack
  - key: readability-identifier-naming.PrivateMemberPrefix
    value: 'm_'
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-function-cognitive-complexity.Threshold
    value: 25
  - key: readability-function-size.LineThreshold
    value: 50
  - key: readability-function-size.ParameterThreshold
    value: 4
  - key: performance-unnecessary-value-param.AllowedTypes
    value: 'std::string_view'
...
```

### Enabled Checks Explained

| Check Category | Purpose |
|----------------|---------|
| `bugprone-*` | Catch common bugs (dangling handles, infinite loops, etc.) |
| `cppcoreguidelines-*` | Enforce C++ Core Guidelines |
| `misc-*` | Miscellaneous useful checks |
| `modernize-*` | Use modern C++ features |
| `performance-*` | Catch performance issues |
| `readability-*` | Enforce readable code |

### Disabled Checks (With Justification)

| Check | Why Disabled |
|-------|--------------|
| `bugprone-easily-swappable-parameters` | Too noisy for simple functions |
| `cppcoreguidelines-avoid-magic-numbers` | Use `readability-magic-numbers` selectively |
| `cppcoreguidelines-pro-bounds-*` | We use arrays safely, these are too strict |
| `cppcoreguidelines-pro-type-reinterpret-cast` | Needed for binary file parsing |
| `modernize-use-trailing-return-type` | Style preference, not required |
| `readability-magic-numbers` | Too noisy, use constants where it matters |
| `readability-identifier-length` | Short names like `i`, `x`, `y` are fine in context |

---

## NO SUPPRESSION POLICY

**This is non-negotiable.**

### Forbidden Practices

```cpp
// FORBIDDEN - Never do these

// clang-format off
badly_formatted = code;
// clang-format on

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
int unused;
#pragma clang diagnostic pop

// NOLINT
void badFunction(); // NOLINT(bugprone-exception-escape)

// NOLINTNEXTLINE
risky_call(); // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
```

### The Correct Approach

| Problem | Wrong Solution | Correct Solution |
|---------|----------------|------------------|
| Unused variable warning | `// NOLINT` | Delete the variable |
| Magic number warning | `// NOLINT` | Create a named constant |
| Long function warning | `// NOLINT` | Split into smaller functions |
| Cognitive complexity | `// NOLINT` | Refactor the logic |
| Unsafe cast warning | `#pragma ignore` | Use safe cast or redesign |
| Format doesn't match preference | `// clang-format off` | Accept the formatter's output |

### Real Examples

```cpp
// WARNING: readability-magic-numbers
void setTimeout(int ms) {
    if (ms > 5000) { /* ... */ }  // What is 5000?
}

// WRONG FIX
void setTimeout(int ms) {
    if (ms > 5000) { /* ... */ }  // NOLINT(readability-magic-numbers)
}

// CORRECT FIX
constexpr int MAX_TIMEOUT_MS = 5000;
void setTimeout(int ms) {
    if (ms > MAX_TIMEOUT_MS) { /* ... */ }
}
```

```cpp
// WARNING: cppcoreguidelines-owning-memory
void process() {
    auto* data = new Data();  // Raw new
    // ...
    delete data;
}

// WRONG FIX
void process() {
    auto* data = new Data();  // NOLINT(cppcoreguidelines-owning-memory)
    delete data;              // NOLINT
}

// CORRECT FIX
void process() {
    auto data = std::make_unique<Data>();
    // ... automatic cleanup
}
```

### If a Check is Truly Wrong

If a linter check is genuinely inappropriate for this project:

1. **Discuss it** - Raise the issue, explain why
2. **Disable globally** - Add to `.clang-tidy` disabled list with comment
3. **Document reason** - In this file, under "Disabled Checks"

Never suppress inline. If it's wrong for one place, it's wrong everywhere.

---

## Error Handling

### Strategy by Context

| Context | Approach |
|---------|----------|
| Expected failure (file not found) | Return `std::optional` or error struct |
| Programmer error (null pointer) | `assert()` in debug |
| Unrecoverable (out of memory) | Let it crash |
| User input validation | Return error with message |

### Pattern: Result Type

```cpp
struct LoadResult {
    std::unique_ptr<Mesh> mesh;
    std::string error;  // Empty on success

    explicit operator bool() const { return mesh != nullptr; }
};

auto loadModel(const std::filesystem::path& path) -> LoadResult {
    if (!std::filesystem::exists(path)) {
        return {nullptr, "File not found: " + path.string()};
    }

    auto mesh = parseFile(path);
    if (!mesh) {
        return {nullptr, "Failed to parse: " + path.string()};
    }

    return {std::move(mesh), ""};
}

// Usage
if (auto result = loadModel(path)) {
    useModel(std::move(result.mesh));
} else {
    showError(result.error);
}
```

### No Silent Failures

```cpp
// BAD - silent failure
void save(const Path& p) {
    std::ofstream f(p);
    f << data;  // Might fail silently
}

// GOOD - check and report
auto save(const Path& p) -> bool {
    std::ofstream f(p);
    if (!f) {
        LOG_ERROR("Failed to open {} for writing", p.string());
        return false;
    }
    f << data;
    if (!f.good()) {
        LOG_ERROR("Failed to write to {}", p.string());
        return false;
    }
    return true;
}
```

---

## Comments Philosophy

### When to Comment

| Type | When to Use |
|------|-------------|
| **Why** | Non-obvious business reason |
| **What** | Never - code should be clear |
| **How** | Only for complex algorithms |
| **TODO** | Acceptable, track in issues |

```cpp
// BAD - explains what (obvious from code)
// Loop through all models and render visible ones
for (const auto& model : models) {
    if (model.isVisible()) {
        render(model);
    }
}

// GOOD - explains why (not obvious)
// Skip hidden models early to avoid GPU state changes
for (const auto& model : models) {
    if (model.isVisible()) {
        render(model);
    }
}
```

### No Commented-Out Code

```cpp
// FORBIDDEN
void process() {
    doThing();
    // doOldThing();  // Disabled for now
    // doAnotherThing();  // TODO: re-enable
}

// CORRECT - delete it, git has history
void process() {
    doThing();
}
```

---

## Testing Standards

### What to Test

| Must Test | Skip |
|-----------|------|
| Parsers (STL, OBJ, 3MF, G-code) | ImGui UI code |
| Algorithms (optimizer, hash) | Simple getters/setters |
| Database operations | Trivial wrappers |
| Edge cases and error paths | Platform-specific glue |

### Test Naming

```cpp
TEST(STLLoader, LoadsBinaryFileCorrectly) { }
TEST(STLLoader, ReturnsNulloptForMissingFile) { }
TEST(STLLoader, RejectsCorruptedHeader) { }
TEST(GCodeParser, CalculatesCorrectTimeForLinearMoves) { }
TEST(GCodeParser, HandlesMetricAndImperialUnits) { }
```

### Test Structure (Arrange-Act-Assert)

```cpp
TEST(CutOptimizer, OptimizesSimpleRectangles) {
    // Arrange
    CutOptimizer optimizer;
    std::vector<Part> parts = {{100, 200}, {150, 300}, {100, 200}};
    Sheet sheet{1000, 1000};

    // Act
    auto result = optimizer.optimize(parts, sheet);

    // Assert
    EXPECT_EQ(result.usedSheets, 1);
    EXPECT_EQ(result.placedParts.size(), 3);
    EXPECT_LT(result.wastePercentage, 0.5f);
}
```

---

## Cross-Platform Rules

- Never use platform-specific headers in `core/`
- Use `std::filesystem` for all path operations
- Use SDL2 for windowing, input, file dialogs
- Isolate platform code in `platform/` directory
- Test on all three platforms before merge

```cpp
// BAD - Windows-specific
#include <windows.h>
void openFile() {
    OPENFILENAME ofn;
    // ...
}

// GOOD - cross-platform via SDL2
#include <SDL2/SDL.h>
void openFile() {
    // Use SDL file dialog or portable library
}
```

---

## Summary

| Aspect | Rule |
|--------|------|
| Primary principle | KISS > Modularize > DRY > YAGNI |
| Modularize threshold | 2+ uses OR 50+ lines (even if 1 use) |
| Max function | 50 lines |
| Max file | 800 lines (.cpp), 400 (.h) |
| Max parameters | 4 |
| Max nesting | 3 levels |
| Formatting | Clang-Format, automatic, no exceptions |
| Linting | Clang-Tidy, warnings as errors |
| Suppression | **FORBIDDEN** - fix code, don't hide warnings |
| Comments | Why, not what |
| Tests | Parsers, algorithms, database |
