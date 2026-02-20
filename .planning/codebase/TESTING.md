# Testing Patterns

**Analysis Date:** 2026-02-19

## Test Framework

**Runner:**
- GoogleTest (gtest) - version managed via CMake FetchContent
- Config: `tests/CMakeLists.txt`
- Test executable: `dw_tests`

**Assertion Library:**
- GoogleTest's built-in assertions: `EXPECT_*`, `ASSERT_*`

**Run Commands:**
```bash
# Build and run all tests
cd build
cmake --build .
ctest --verbose

# Or run directly
./dw_tests

# Run specific test
ctest -R "DatabaseTest" -V

# With regex filter
./dw_tests --gtest_filter="*Database*"
```

## Test File Organization

**Location:**
- Co-located in `tests/` directory
- Each component gets one `test_<component>.cpp` file
- No nested test subdirectories

**Naming:**
- Test files: `test_<component>.cpp` (e.g., `test_database.cpp`, `test_model_repository.cpp`)
- Test fixtures: `<Component>Test` (e.g., `DatabaseTest`, `ModelRepoTest`, `EventBusTest`)
- Standalone tests: `<Module>` (e.g., `Database` for non-fixture tests)

**Structure:**
```
tests/
├── CMakeLists.txt              # Test configuration
├── test_main.cpp               # Bootstrap (minimal GTest setup)
├── test_database.cpp           # Database component tests
├── test_model_repository.cpp   # Repository layer tests
├── test_event_bus.cpp          # Event system tests
├── test_library_manager.cpp    # Integration tests with DB
├── stub_thumbnail_generator.cpp # GL-dependent stubs
└── data/                       # Test fixtures (commented in CMakeLists)
```

## Test Structure

**Suite Organization:**
```cpp
// Digital Workshop - Component Tests

#include <gtest/gtest.h>
#include "core/database/database.h"

namespace {

// Fixture: provides setup/teardown per test
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
    }

    void TearDown() override {
        m_db.close();
    }

    dw::Database m_db;
};

}  // namespace

// --- Test Groups ---

TEST_F(DatabaseTest, Open_InMemory) {
    EXPECT_FALSE(m_db.isOpen());
    EXPECT_TRUE(m_db.open(":memory:"));
    EXPECT_TRUE(m_db.isOpen());
}

TEST_F(DatabaseTest, Execute_CreateTable) {
    EXPECT_TRUE(m_db.execute("CREATE TABLE test (id INTEGER PRIMARY KEY)"));
}
```

**Patterns:**
- **Setup (SetUp override)**: Initialize resources needed by all tests in fixture
  - Database open, schema init, temp directory creation
  - Use `ASSERT_*` for setup failures (stops test if setup fails)

- **Teardown (TearDown override)**: Clean up resources
  - Database close, temp directory removal, file cleanup
  - Not required unless resource cleanup needed

- **Assertion pattern**: Use `EXPECT_*` for actual test checks
  - `EXPECT_EQ(actual, expected)` for equality
  - `EXPECT_TRUE(condition)` for boolean checks
  - `EXPECT_FALSE(condition)` for negation
  - `EXPECT_GT(actual, min)` for comparisons
  - `EXPECT_NEAR(actual, expected, tolerance)` for floats
  - Chain assertions: `ASSERT_TRUE(setup); EXPECT_EQ(result, value);`

## Mocking

**Framework:** Not explicitly used; instead, tests use:
- **In-memory databases**: SQLite `:memory:` for database tests
- **Real file I/O with temp directories**: `std::filesystem::temp_directory_path()` for file tests
- **Test data builders**: Helper methods like `makeModel()` to construct test objects

**Patterns:**
```cpp
// Example from test_library_manager.cpp - builder pattern for test data
dw::ModelRecord makeModel(const std::string& hash, const std::string& name) {
    dw::ModelRecord rec;
    rec.hash = hash;
    rec.name = name;
    rec.filePath = "/models/" + name + ".stl";
    rec.fileFormat = "stl";
    rec.fileSize = 1024;
    rec.vertexCount = 100;
    rec.triangleCount = 50;
    rec.boundsMin = dw::Vec3(0, 0, 0);
    rec.boundsMax = dw::Vec3(1, 1, 1);
    return rec;
}

// Use in test
TEST_F(ModelRepoTest, Insert_ReturnsId) {
    auto rec = makeModel("abc123", "cube");
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}
```

**What to Mock:**
- Nothing; integration tests preferred
- Use real components with in-memory equivalents (e.g., `:memory:` SQLite database)
- Dependencies injected via constructor (e.g., `ModelRepository(Database& db)`)

**What NOT to Mock:**
- Don't mock filesystem operations; use temp directories
- Don't mock databases; use in-memory SQLite
- Don't mock internal implementation; test the public interface

## Fixtures and Factories

**Test Data:**
```cpp
// From test_library_manager.cpp
void writeMiniSTL(const std::string& name) {
    auto path = m_tmpDir / (name + ".stl");

    dw::ByteBuffer buf(80 + 4 + 50, 0);
    dw::u32 triCount = 1;
    std::memcpy(buf.data() + 80, &triCount, sizeof(triCount));

    float tri[12] = {
        0, 0, 1,    // normal
        0, 0, 0,    // v0
        1, 0, 0,    // v1
        0, 1, 0     // v2
    };
    std::memcpy(buf.data() + 84, tri, sizeof(tri));

    EXPECT_TRUE(dw::file::writeBinary(path, buf));
    return path;
}
```

**Location:**
- Test helper methods defined as protected methods in fixture class
- Reused across multiple tests in same fixture
- Keep helper logic focused and testable

## Coverage

**Requirements:** No enforced coverage targets

**View Coverage:** Not configured (would require gcov/lcov setup)

**Practice:**
- Test all public methods of classes
- Test error conditions and edge cases
- Focus on behavior, not coverage percentage
- Integration tests preferred over unit tests for coverage (fewer mocks = fewer gaps)

## Test Types

**Unit Tests:**
- Test individual components in isolation
- Examples: `test_string_utils.cpp`, `test_types.cpp`, `test_hash.cpp`
- Use real dependencies; mock only external systems
- Fast execution (no I/O, no network)

**Integration Tests:**
- Test multiple components working together
- Examples: `test_library_manager.cpp`, `test_model_repository.cpp`, `test_database.cpp`
- Use in-memory databases instead of mocking
- Verify data flows correctly through layers

**E2E Tests:**
- Not implemented
- Application runs in GUI mode; E2E would require SDL/OpenGL setup
- Covered by integration tests instead

**Tier Organization (from CMakeLists.txt):**
```
Tier 0 — existing tests (stl_loader, obj_loader, hash, gcode_parser)
Tier 0 — added in first pass (string_utils, file_utils, mesh, database, repositories)
Tier 1 — EventBus, ConnectionPool, MainThreadQueue, core logic
Tier 2 — lint compliance (no GL/SDL required)
```

## Common Patterns

**Async Testing:**
Not implemented; application architecture doesn't use async I/O testing.

**Error Testing:**
```cpp
// Test error condition
TEST_F(DatabaseTest, Execute_InvalidSQL) {
    EXPECT_FALSE(m_db.execute("NOT VALID SQL"));
    EXPECT_FALSE(m_db.lastError().empty());
}

// Test optional returns
TEST_F(ModelRepoTest, FindById_NotFound) {
    auto found = m_repo->findById(999);
    EXPECT_FALSE(found.has_value());
}

// Test exception safety (rare in this codebase)
TEST_F(EventBusTest, ThrowingHandler_DoesNotStopOthers) {
    // (EventBus itself doesn't throw; handlers are responsible)
}
```

**Test Independence:**
- Each test is independent; no test should depend on another
- SetUp/TearDown ensures clean state for each test
- Use anonymous namespace `namespace {}` to avoid test name collisions
- Temporary files cleaned up in TearDown

**Assertion Order:**
```cpp
// ASSERT_* for setup/preconditions (stops test on failure)
ASSERT_TRUE(m_db.open(":memory:"));
ASSERT_TRUE(dw::Schema::initialize(m_db));

// EXPECT_* for actual test assertions (continues to show all failures)
EXPECT_EQ(actual, expected);
EXPECT_TRUE(condition);
```

## Test Data Management

**In-Memory Databases:**
```cpp
// From fixture
void SetUp() override {
    ASSERT_TRUE(m_db.open(":memory:"));  // RAM-based, cleared per test
    ASSERT_TRUE(dw::Schema::initialize(m_db));
}
```

**Temporary Files:**
```cpp
// From fixture
void SetUp() override {
    m_tmpDir = std::filesystem::temp_directory_path() / "dw_test_component";
    std::filesystem::create_directories(m_tmpDir);
}

void TearDown() override {
    std::filesystem::remove_all(m_tmpDir);  // Clean up after test
}
```

**Builder Pattern:**
```cpp
// Helper to create valid test objects
dw::ModelRecord makeModel(const std::string& hash, const std::string& name) {
    dw::ModelRecord rec;
    rec.hash = hash;
    rec.name = name;
    // ... set required fields
    return rec;
}
```

## Building and Running Tests

**Build configuration (CMakeLists.txt):**
```cmake
# Enable testing
if(DW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# In tests/CMakeLists.txt:
add_executable(dw_tests ${DW_TEST_SOURCES} ${DW_TEST_DEPS})
target_link_libraries(dw_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    imgui
    glm::glm
    SQLite::SQLite3
    ZLIB::ZLIB
)
```

**Dependencies included in tests:**
- GoogleTest (gtest, gtest_main)
- Core libraries: imgui, glm, SQLite3, ZLIB
- Main source files compiled together with tests

**Test discovery:**
```cmake
include(GoogleTest)
gtest_discover_tests(dw_tests)  # Automatically registers tests with CTest
```

## Special Considerations

**Threading Tests:**
- EventBus requires main thread initialization: `dw::threading::initMainThread()`
- Use in EventBusTest fixture: `void SetUp() override { dw::threading::initMainThread(); }`
- No concurrent tests (application is single-threaded for UI)

**GL-Dependent Code:**
- Classes requiring SDL/OpenGL skipped in test suite
- Stub implementation provided: `stub_thumbnail_generator.cpp`
- This allows core logic to be tested without GUI dependencies

**Lint Compliance Tests:**
- `test_lint_compliance.cpp` verifies code formatting
- Checks: trailing whitespace, proper newlines, line length (100 chars max)
- Enforces clang-format compliance at test time

---

*Testing analysis: 2026-02-19*
