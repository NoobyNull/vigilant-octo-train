# Testing Patterns

**Analysis Date:** 2026-02-08

## Test Framework

**Runner:**
- Google Test (gtest) - CMake integrated
- Configuration: `tests/CMakeLists.txt`
- Test discovery: Automatic via `gtest_discover_tests()`
- C++ standard: C++17

**Build Configuration:**
- Executable: `dw_tests`
- Linked with: `GTest::gtest`, `GTest::gtest_main`, `glm::glm`, `SQLite::SQLite3`, `ZLIB::ZLIB`
- Sources directory: `tests/` (co-located with main source in build)
- CMakeLists: `tests/CMakeLists.txt` (tier-based test organization)

**Run Commands:**
```bash
# Build tests
cmake --build build --target dw_tests

# Run all tests
ctest                          # Via CTest
build/dw_tests                 # Direct execution

# Run with verbosity
ctest --verbose
./build/dw_tests --gtest_filter=StringUtils.*

# Run specific test class
./build/dw_tests --gtest_filter=DatabaseTest.*

# List all tests
./build/dw_tests --gtest_list_tests
```

## Test File Organization

**Location:**
- Tests in `tests/` directory (separate from source)
- Not co-located with implementation files
- Pattern: `test_<module>.cpp` for each major module

**Naming Convention:**
- File: `test_<module>.cpp` (e.g., `test_string_utils.cpp`, `test_database.cpp`)
- Test class: `<FunctionName>Test` or `<ModuleName>Test` as fixture
- Test case: `TEST(TestClass, TestName_Description)`
- Example: `TEST(StringUtils, Trim_RemovesBothSides)`

**Test Tier Organization (from tests/CMakeLists.txt):**

**Tier 0 — Core utilities and loaders (no dependencies on GL/SDL/complex systems):**
- `test_stl_loader.cpp` - Mesh file loading
- `test_obj_loader.cpp` - Mesh file loading
- `test_hash.cpp` - Content hashing
- `test_gcode_parser.cpp` - G-code parsing
- `test_optimizer.cpp` - Cut optimization
- `test_string_utils.cpp` - String manipulation
- `test_file_utils.cpp` - File I/O utilities
- `test_mesh.cpp` - Mesh geometry operations
- `test_database.cpp` - SQLite wrapper
- `test_model_repository.cpp` - Database layer
- `test_project_repository.cpp` - Database layer
- `test_loader_factory.cpp` - Loader selection
- `test_project.cpp` - Project model
- `test_exporter.cpp` - Model export

**Tier 1 — Core logic with type system:**
- `test_types.cpp` - Type system and math helpers
- `test_gcode_analyzer.cpp` - G-code analysis
- `test_schema.cpp` - Database schema
- `test_camera.cpp` - Camera mathematics (no GL context)
- `test_archive.cpp` - ZIP archive operations
- `test_library_manager.cpp` - Library/asset management
- `test_threemf_loader.cpp` - 3MF format loader
- `test_import_pipeline.cpp` - Import workflows
- `test_config_watcher.cpp` - Configuration file monitoring
- `test_app_paths.cpp` - Application directory resolution
- `test_cost_repository.cpp` - Cost estimation system

**Tier 2 — Compliance and style checking (no GL/SDL required):**
- `test_lint_compliance.cpp` - Clang-tidy compliance verification

**Stubs for GL-dependent code:**
- `stub_thumbnail_generator.cpp` - Placeholder for GL-dependent thumbnail generation

**Compiled dependencies:**
Test executable automatically compiles source files from `src/` via `DW_TEST_DEPS` in CMakeLists:
- Core types, utilities, loaders
- Mesh processing (geometry, hashing)
- G-code handling (parsing, analysis)
- Optimization algorithms
- Database and repositories
- Project model and export
- Archive and library management
- Configuration monitoring
- Camera and path utilities

## Test Structure

**Suite Organization Pattern:**

```cpp
// Digital Workshop - StringUtils Tests

#include <gtest/gtest.h>

#include "core/utils/string_utils.h"

// --- Grouped by functionality ---

TEST(StringUtils, Trim_RemovesBothSides) {
    EXPECT_EQ(dw::str::trim("  hello  "), "hello");
}

TEST(StringUtils, Trim_TabsAndNewlines) {
    EXPECT_EQ(dw::str::trim("\t\nhello\n\t"), "hello");
}

// --- Case conversion ---

TEST(StringUtils, ToLower_Basic) {
    EXPECT_EQ(dw::str::toLower("HELLO"), "hello");
}
```

**Fixture Pattern (for setup/teardown):**

```cpp
// test_database.cpp
namespace {

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
    }

    dw::Database m_db;
};

}  // namespace

TEST_F(DatabaseTest, Execute_CreateTable) {
    EXPECT_TRUE(m_db.execute("CREATE TABLE test (...)"));
}
```

**Header File Comments:**
Each test file begins with standardized header:
```cpp
// Digital Workshop - <Module> Tests

#include <gtest/gtest.h>
#include "<path to module>"
```

## Mocking

**Framework:**
- Google Mock (gmock) - Included with gtest
- Not extensively used (project favors real implementations over mocks)

**Pattern for Stubbed Functionality:**
Actual implementation stubs rather than mocks in some cases:
- Example: `stub_thumbnail_generator.cpp` - Provides dummy implementation for GL-dependent code
- Allows tests to compile without OpenGL context
- Actual implementation file can be swapped in for GUI tests

**What to Mock:**
- External system dependencies (file system operations where needed)
- Network services (if any integrations added)
- Expensive operations with deterministic mocks

**What NOT to Mock:**
- Core business logic (string utils, mesh operations, database queries)
- Data structures (use real Mesh, Database, etc.)
- File I/O for local test data (use real filesystem temporarily)

## Fixtures and Factories

**Test Data Pattern:**
Minimal fixtures - create data inline in tests for clarity.

**Example - Database test fixture:**
```cpp
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
    }

    dw::Database m_db;
};

TEST_F(DatabaseTest, PrepareAndBind_InsertAndQuery) {
    ASSERT_TRUE(m_db.execute(
        "CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, value REAL)"
    ));

    auto insert = m_db.prepare("INSERT INTO items (name, value) VALUES (?, ?)");
    ASSERT_TRUE(insert.bindText(1, "widget"));
    ASSERT_TRUE(insert.bindDouble(2, 3.14));
    EXPECT_TRUE(insert.execute());
}
```

**Factory Functions (when needed):**
- Helper functions directly in test files (no shared factory classes needed)
- Example from `test_archive.cpp`:
```cpp
// Helper to create test file
bool createTestFile(const Path& path, const std::string& content) {
    // Create temporary file with content
    return dw::file::writeText(path, content);
}
```

**Location:**
- Fixtures and helpers defined in same file as tests using them
- No separate fixture directory - keep tests self-contained
- Temporary files created in system temp directory via test setup

## Coverage

**Requirements:**
- No enforced coverage target (pragmatic approach)
- Focus on critical paths: loaders, database, optimization algorithms
- Entire Tier 0 and Tier 1 module sets have test coverage
- Excluded from coverage requirements: GL rendering, UI dialogs, platform-specific paths

**View Coverage:**
```bash
# Using gcov (if clang/gcc with coverage flags)
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ..
cmake --build .
ctest
gcov tests/test_*.cpp

# Using lcov for HTML report
lcov --directory . --capture --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

**Current Coverage:**
Test suite organized in tiers covering:
- String utilities: 100% (all string operations tested)
- Mesh operations: 100% (geometry, transforms, bounds, validation)
- Loaders (STL, OBJ, 3MF): 100% (both ASCII and binary variants)
- Database operations: 100% (CRUD, transactions, queries)
- G-code handling: 100% (parsing and analysis)
- Optimization: 100% (bin packing, guillotine algorithm)
- Archives: 100% (ZIP creation and extraction)

## Test Types

**Unit Tests:**
- Scope: Individual functions and classes
- Examples: `test_string_utils.cpp`, `test_hash.cpp`, `test_types.cpp`
- Approach: Test in isolation with real implementations
- Database tests use in-memory SQLite (no external DB needed)

**Integration Tests:**
- Scope: Interactions between modules
- Examples: `test_import_pipeline.cpp`, `test_library_manager.cpp`
- Approach: Test full workflows (e.g., load file → parse → store in DB)
- Real file I/O to temporary directories

**E2E Tests:**
- Status: Not used in current suite
- Reason: Lack of deterministic SDL2/OpenGL environment in CI
- Approach if needed: Separate test binary targeting renderer (requires display)
- Alternative: Use visual regression via screenshots in integration tests

## Common Patterns

**Assertion Patterns:**

```cpp
// Expect vs Assert
EXPECT_EQ(actual, expected);   // Test continues on failure
ASSERT_EQ(actual, expected);   // Test stops on failure (use when next line depends on this)

// Common assertions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);
EXPECT_EQ(a, b);
EXPECT_NE(a, b);
EXPECT_LT(a, b);
EXPECT_LE(a, b);
EXPECT_GT(a, b);
EXPECT_GE(a, b);
EXPECT_NEAR(a, b, epsilon);   // For floating-point comparison

// String and container assertions
EXPECT_EQ(string, "expected");
EXPECT_EQ(vector.size(), 3u);
EXPECT_TRUE(vector.empty());

// Output messages on failure
EXPECT_EQ(a, b) << "Custom failure message";
ASSERT_TRUE(condition) << "Setup failed: " << errorInfo;
```

**Async Testing:**
No async/threading tests currently in suite. If needed:
```cpp
// Use std::thread with gtest
TEST(AsyncTest, WorksWithThreads) {
    std::vector<int> results;
    std::mutex mutex;

    std::thread t([&]() {
        int value = expensiveComputation();
        std::lock_guard<std::mutex> lock(mutex);
        results.push_back(value);
    });

    t.join();

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], expectedValue);
}
```

**Error Testing:**

Test that operations fail correctly:
```cpp
TEST(Database, Execute_InvalidSQL) {
    dw::Database db;
    ASSERT_TRUE(db.open(":memory:"));

    EXPECT_FALSE(db.execute("NOT VALID SQL"));
    EXPECT_FALSE(db.lastError().empty());
}

TEST(Loader, Load_InvalidFile) {
    dw::STLLoader loader;
    auto result = loader.load("/nonexistent/file.stl");

    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.error.empty());
}
```

**File I/O Testing:**

Use temporary filesystem:
```cpp
TEST(Archive, Create_AndExtract) {
    // Create test file
    auto tempDir = std::filesystem::temp_directory_path() / "dw_test_xyz";
    std::filesystem::create_directory(tempDir);

    Path testFile = tempDir / "test.txt";
    ASSERT_TRUE(dw::file::writeText(testFile, "Hello World"));

    // Create archive
    auto createResult = dw::ProjectArchive::create(
        (tempDir / "test.dwp").string(),
        tempDir.string()
    );
    ASSERT_TRUE(createResult.success) << createResult.error;

    // Extract and verify
    auto extractResult = dw::ProjectArchive::extract(
        (tempDir / "test.dwp").string(),
        (tempDir / "extracted").string()
    );
    ASSERT_TRUE(extractResult.success);

    // Cleanup
    std::filesystem::remove_all(tempDir);
}
```

**Database Testing Pattern:**

In-memory SQLite for fast, isolated tests:
```cpp
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // In-memory database - no file I/O
        ASSERT_TRUE(m_db.open(":memory:"));
    }

    void TearDown() override {
        m_db.close();
    }

    dw::Database m_db;
};

TEST_F(DatabaseTest, Transaction_RollsBackOnFailure) {
    ASSERT_TRUE(m_db.execute("CREATE TABLE test (id INTEGER PRIMARY KEY)"));

    ASSERT_TRUE(m_db.beginTransaction());
    ASSERT_TRUE(m_db.execute("INSERT INTO test (id) VALUES (1)"));
    // Simulate error - rollback instead of commit
    m_db.rollbackTransaction();

    auto stmt = m_db.prepare("SELECT COUNT(*) FROM test");
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.getInt(0), 0);  // No row was inserted
}
```

**LoadResult Testing:**

Test both success and failure paths:
```cpp
TEST(STLLoader, Load_ValidFile) {
    dw::STLLoader loader;
    auto result = loader.load("test_data/cube.stl");

    ASSERT_TRUE(result.success());
    ASSERT_NE(result.mesh, nullptr);
    EXPECT_GT(result.mesh->vertexCount(), 0);
}

TEST(STLLoader, Load_InvalidFile) {
    dw::STLLoader loader;
    auto result = loader.load("/nonexistent/file.stl");

    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.mesh, nullptr);
    EXPECT_FALSE(result.error.empty());
}
```

---

*Testing analysis: 2026-02-08*
