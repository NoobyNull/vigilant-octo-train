# Phase 2: Import Pipeline - Research

**Researched:** 2026-02-09
**Domain:** Parallel file import with C++ thread pools, G-code parsing, SQLite concurrency, cross-platform file operations
**Confidence:** MEDIUM-HIGH

## Summary

This phase overhauls the import pipeline from single-threaded sequential processing to parallel workers with configurable thread pools. The research reveals that the existing architecture (single worker thread with ConnectionPool) provides a solid foundation but needs expansion to support multiple concurrent workers. The critical technical challenges are SQLite write serialization (only one writer at a time), thread pool sizing based on user preferences, G-code parsing for Grbl-family dialects, and cross-platform file copy/move operations.

The existing codebase already has the infrastructure pieces in place: ConnectionPool for database access, MainThreadQueue for GL operations, EventBus for notifications, and ImportTask/ImportQueue for pipeline orchestration. The main work is extending these patterns to support parallel workers, adding G-code format support, implementing file management modes, and replacing the modal progress overlay with background status indicators.

**Primary recommendation:** Use C++ standard library thread pool (vector of std::thread workers with task queue), leverage existing ConnectionPool with WAL mode for read concurrency (serialize writes with application-level mutex), add G-code as new format in LoaderFactory, implement file operations with std::filesystem, and extend ImportTask to handle both mesh and G-code types.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Pipeline Architecture
- Switch from sequential per-file processing to **parallel workers** using a thread pool
- Three parallelism tiers (user-configurable in Settings):
  - **Auto** (default): 60% of CPU cores
  - **Fixed/Performance**: capped at 90% of cores
  - **Expert**: 100% of cores (user accepts UI responsiveness risk)
- Each worker runs the full pipeline independently (read → hash → dedup → parse → insert)

#### File Handling After Import
- Three modes: **copy into library**, **move to library**, or **leave in place**
- Configured as a **global setting** in the Settings app — applies to all imports silently
- Default: leave in place (current behavior)
- When copy/move is selected, files go to a managed library directory

#### G-code Support
- Add G-code as a new importable format alongside STL/OBJ/3MF
- Target dialects: **Grbl, grblHAL, FluidNC** (all Grbl-compatible core)
- G-code imports produce **toolpath visualization** rendered as 3D lines in the viewport
- G-code can exist **standalone** in the library (association with a model is optional)
- Basic metadata extracted on import: bounding box, total travel distance, estimated run time, feed rates used, tool numbers referenced

#### G-code Hierarchy & Organization
- Models can have associated G-code files organized in a hierarchy: **Model → Operation Groups → ordered G-code files**
- **Templates** are opt-in — user can apply a template (e.g., CNC Router Basic) or skip and create groups manually
- **CNC Router Basic template**: Roughing, Finishing, Profiling, Drilling groups
- **Custom/Minimal**: start with no groups, user creates their own
- User picks one template or no template — no automatic mapping
- G-code imported standalone goes to a general G-code area; user can drag-and-drop to associate with models and organize into groups later
- Auto-detect attempts to match G-code to models by filename

#### Duplicate Handling
- Duplicates are **skipped automatically** (current behavior preserved)
- **Summary shown at end** of batch listing all skipped duplicates with names

#### Error Handling
- Errors do not interrupt the batch — other files continue importing
- **Summary at end** lists all errors with file names and failure reasons
- **Toast notifications** for individual errors, configurable on/off in settings

#### Import UX
- Import runs in **background** with a **status bar indicator** — non-blocking, user can keep working
- Replace current modal overlay with background processing
- After batch completes, **viewport focus does not change** — stay on whatever user was viewing
- Tags assigned **after import only**, not during

#### Format Polish
- Improve robustness of existing STL/OBJ/3MF loaders (error handling, edge cases)
- No new mesh formats this phase (STEP/DXF/SVG deferred)

#### Metadata
- Mesh files: capture what's naturally available (current fields sufficient)
- G-code files: basic stats (bounding box, travel distance, estimated time, feed rates, tool numbers)
- Richer metadata (material, stock info, source tracking) deferred to export system phase

### Claude's Discretion
- Managed library directory structure and naming
- G-code parser implementation approach
- Toolpath rendering style (colors, line widths, rapid vs cutting move differentiation)
- Status bar indicator design
- Toast notification styling and duration
- Thread pool implementation details
- Database schema extensions for G-code and hierarchy

### Deferred Ideas (OUT OF SCOPE)
- STEP/IGES import — CAD solid model support, requires tessellation (future format phase)
- DXF/SVG import — 2D vector formats for cutting paths (future format phase)
- Export system — when built, will need material/stock info, source tracking, attribution metadata
- Full G-code analysis — rapid vs cutting breakdown, depth of cuts, spindle speeds, axis limits (future enhancement)
- Tag UI during import — tags exist in schema, UI deferred to library management phase
</user_constraints>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++ Standard Library | C++17+ | Thread pool (`std::thread`, `std::mutex`, `std::condition_variable`), filesystem ops | Native, no dependencies, sufficient for bounded thread pool pattern |
| SQLite | 3.x | Database with WAL mode | Already in use, handles concurrent reads well with proper connection pooling |
| ImGui | Current | Toast notifications, status bar UI | Already in use for all UI, header-only notification wrappers available |
| xxHash | Latest | Fast non-cryptographic hashing | 50x faster than SHA256 for duplicate detection ([xxHash benchmark](https://create.stephan-brumme.com/xxhash/)) |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| std::hardware_concurrency() | C++11+ | CPU core detection | Use for Auto/Fixed/Expert tier calculation, provide fallback if returns 0 |
| std::filesystem | C++17 | Cross-platform file copy/move/rename | File handling modes (copy/move/leave-in-place) |
| FNV-1a hash | Current impl | File content hashing | Already implemented in codebase, sufficient for deduplication |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom thread pool | [BS::thread_pool](https://github.com/bshoshany/thread-pool) | Header-only C++17 library, very popular, but adds dependency for functionality we can implement with std lib |
| FNV-1a | xxHash library | xxHash is 10x faster on small data, 50x on large files, but requires integration; FNV-1a is already working and "good enough" for this phase |
| Custom filesystem | [cppfs](https://github.com/cginternals/cppfs) | Cross-platform abstraction with advanced features, but std::filesystem is native and sufficient |

**Installation:**
No external dependencies required. Use C++ standard library features already available in C++17.

## Architecture Patterns

### Recommended Project Structure

Current structure is appropriate, extensions needed:

```
src/core/
├── import/
│   ├── import_queue.h/cpp       # Extend to multi-worker pool
│   ├── import_task.h            # Add GCodeTask variant
│   └── import_worker.h/cpp      # New: worker thread logic
├── gcode/
│   ├── gcode_parser.h/cpp       # Already exists, extend for metadata
│   ├── gcode_analyzer.h/cpp     # Already exists
│   ├── gcode_types.h            # Already exists
│   └── gcode_loader.h/cpp       # New: implements MeshLoader interface
├── database/
│   ├── schema.h/cpp             # Extend with gcode table, hierarchy tables
│   ├── gcode_repository.h/cpp   # New: CRUD for G-code records
│   └── connection_pool.h/cpp    # Already thread-safe
├── loaders/
│   └── loader_factory.cpp       # Register gcode_loader
└── threading/
    ├── thread_pool.h/cpp        # New: generic worker pool
    └── main_thread_queue.h/cpp  # Already exists
```

### Pattern 1: Bounded Thread Pool with Task Queue

**What:** Fixed number of worker threads with shared task queue. Workers pull tasks, process independently, post results to main thread queue for GL operations.

**When to use:** Embarrassingly parallel workloads where tasks don't block for long periods and don't require inter-task communication.

**Implementation approach:**
```cpp
// Simplified structure (not complete implementation)
class ThreadPool {
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_shutdown{false};

    void workerLoop() {
        while (!m_shutdown) {
            std::function<void()> task;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return !m_tasks.empty() || m_shutdown;
                });
                if (m_shutdown && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            task(); // Execute outside lock
        }
    }
};
```

**Source:** Standard pattern from [C++ thread pool implementations](https://github.com/progschj/ThreadPool), adapted to bounded size.

**Application to import pipeline:**
- Replace single `std::thread m_worker` in ImportQueue with `ThreadPool`
- Each worker independently processes `read → hash → dedup → parse → insert` pipeline
- Use ScopedConnection from ConnectionPool (already thread-safe)
- Post completed tasks to MainThreadQueue for thumbnail generation

### Pattern 2: SQLite Concurrent Access with WAL Mode

**What:** Use Write-Ahead Logging mode to allow concurrent reads while single writer operates. Application-level mutex serializes writes.

**When to use:** When multiple threads need database access but write volume is manageable and writes can be serialized without blocking reads.

**Current implementation:**
```cpp
// From src/core/database/connection_pool.h
class ConnectionPool {
    std::deque<std::unique_ptr<Database>> m_available;
    std::vector<Database*> m_inUse;
    std::mutex m_mutex; // Application-level synchronization
};
```

**Critical insight:** SQLite allows multiple readers OR one writer. With WAL mode, readers don't block each other and aren't blocked by writer. ConnectionPool already has application-level mutex controlling access. For import, each worker gets its own connection from pool for reads (dedup check), but must serialize writes (insert).

**Source:** [SQLite threading documentation](https://www.sqlite.org/threadsafe.html), [WAL mode concurrency](https://www.slingacademy.com/article/managing-concurrent-access-in-sqlite-databases/)

### Pattern 3: G-code Parser with State Machine

**What:** Line-by-line parser maintaining modal state (current position, units, positioning mode, feed rate).

**When to use:** G-code is inherently stateful—commands affect future commands (e.g., G90 sets absolute mode for all subsequent moves).

**Current implementation exists:**
```cpp
// From src/core/gcode/gcode_parser.h
class Parser {
    Program parse(const std::string& content);
    Command parseLine(const std::string& line, int lineNumber);
};
```

**Grbl compatibility:** Grbl, grblHAL, FluidNC share core G-code commands (G0/G1/G2/G3, G20/G21, G90/G91, M3/M4/M5). Differences are in extended commands and configuration, not core motion parsing.

**Source:** [Grbl G-code compatibility](http://wiki.fluidnc.com/en/features/grbl_compatibility), [grblHAL core](https://github.com/grblHAL/core)

### Pattern 4: Two-Phase Import (Worker Thread + Main Thread)

**What:** Worker thread does I/O and CPU-intensive work (read, hash, parse, DB insert). Main thread does OpenGL work (thumbnail generation, UI updates).

**When to use:** When operations require resources bound to specific threads (OpenGL context, UI framework).

**Current implementation:**
```cpp
// From src/core/import/import_queue.cpp
void ImportQueue::processTask(ImportTask& task) {
    // ... stages: Reading, Hashing, CheckingDuplicate, Parsing, Inserting ...
    task.stage = ImportStage::WaitingForThumbnail;
    // Main thread polls pollCompleted() and generates thumbnails
}
```

**Extend to parallel workers:** Each worker processes tasks independently until `WaitingForThumbnail`, posts to MainThreadQueue, main thread drains queue each frame.

### Pattern 5: Hardware Concurrency Detection with Tiers

**What:** Use `std::thread::hardware_concurrency()` to detect CPU cores, apply percentage-based caps for user tiers.

**When to use:** Letting users control parallelism vs UI responsiveness tradeoff.

**Implementation:**
```cpp
size_t getThreadCount(ParallelismTier tier) {
    unsigned int hwCores = std::thread::hardware_concurrency();
    if (hwCores == 0) hwCores = 4; // Fallback

    switch (tier) {
        case ParallelismTier::Auto:
            return static_cast<size_t>(hwCores * 0.6);
        case ParallelismTier::Fixed:
            return static_cast<size_t>(hwCores * 0.9);
        case ParallelismTier::Expert:
            return static_cast<size_t>(hwCores);
    }
    return 1; // Minimum
}
```

**Caveat:** `hardware_concurrency()` can return 0 if not computable, and may not respect CPU affinity masks (taskset, Docker limits). Provide fallback and allow explicit override.

**Source:** [std::hardware_concurrency documentation](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency), [thread pool sizing](https://medium.com/@sharpsanjay/mastering-c-threads-a-dive-into-std-hardware-concurrency-27d9e091c3ea)

### Pattern 6: Cross-Platform File Operations with std::filesystem

**What:** Use `std::filesystem::copy`, `std::filesystem::rename`, and `std::filesystem::remove` for copy/move/delete operations.

**When to use:** File management modes (copy into library, move to library, leave in place).

**Example:**
```cpp
void copyToLibrary(const Path& source, const Path& dest) {
    namespace fs = std::filesystem;
    fs::create_directories(dest.parent_path());
    fs::copy(source, dest, fs::copy_options::overwrite_existing);
}

void moveToLibrary(const Path& source, const Path& dest) {
    namespace fs = std::filesystem;
    fs::create_directories(dest.parent_path());
    fs::rename(source, dest); // Atomic on same filesystem
}
```

**Important:** `rename` is atomic on same filesystem but fails across filesystems. Fallback: copy + verify + delete original.

**Source:** [std::filesystem operations](https://en.cppreference.com/w/cpp/filesystem), [atomic file operations](https://github.com/jeffreyrogers/AtomicWrite)

### Anti-Patterns to Avoid

- **Too many threads:** Don't create more threads than `hardware_concurrency()` for CPU-bound work. Context switching overhead dominates. Synchronization on work queue becomes bottleneck.
- **Shared mutable state between workers:** Each worker should operate independently. Share read-only data (ConnectionPool), but not task-specific state.
- **Blocking main thread for I/O:** Keep file operations on worker threads. Main thread only processes results (thumbnails, UI updates).
- **Fine-grained database locking:** Don't try to lock individual tables/rows. SQLite's coarse locking means application-level serialization is clearer and performs equally well.
- **Unbounded task queue:** Limit pending imports to avoid memory exhaustion on large batch imports (e.g., user drops 10,000 files).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Thread pool core logic | Custom scheduler with work stealing, priority queues | Bounded thread pool with simple FIFO queue | Work stealing adds complexity without benefit for uniform import tasks. Priority is handled by task ordering before enqueue. |
| G-code interpreter | Full CNC motion controller, real-time path planning | Line-by-line parser for visualization metadata | Import phase only needs toolpath geometry and stats, not real-time motion control. Full interpreter is orders of magnitude more complex. |
| File format detection | Binary signature sniffing, magic numbers | File extension + LoaderFactory::isSupported() | Extension is sufficient and matches user expectations. Binary detection adds edge cases (e.g., STL ASCII vs binary). |
| Atomic file copy | Custom read-verify-write with checksums | std::filesystem::copy with error checking | Filesystem APIs handle edge cases (permissions, symlinks, special files) better than custom code. Verification can be added if needed. |
| Toast notification system | Custom widget manager with animations, stacking, dismissal | [ImGui-notify wrapper](https://github.com/TyomaVader/ImGuiNotify) | Header-only, 200 LOC, handles all common notification patterns (types, durations, stacking, fade). |
| Duplicate detection for G-code | Semantic equivalence (same toolpath, different formatting) | File hash (same as mesh imports) | Semantic analysis is undecidable (whitespace, comments, G-code ordering variations). Hash matches user intent: "same file = duplicate." |

**Key insight:** Import pipeline is inherently embarrassingly parallel (each file independent) with a single serialization point (database insert). Over-engineering the parallel scheduler or trying to parallelize the serialization point adds complexity without performance gain. The win comes from parallelizing the 95% (I/O, parsing) and accepting serialization of the 5% (DB write).

## Common Pitfalls

### Pitfall 1: SQLite Write Contention with Many Workers

**What goes wrong:** Multiple workers try to write to database simultaneously, causing `SQLITE_BUSY` errors and retries, slowing overall throughput.

**Why it happens:** SQLite allows only one writer at a time. With many workers and no coordination, they thrash trying to acquire write lock.

**How to avoid:** Serialize database inserts at application level. Options:
1. **Single writer thread:** Workers post completed tasks to dedicated writer thread with own connection
2. **Application mutex:** Workers acquire mutex before DB insert (current ConnectionPool approach)
3. **Batch inserts:** Accumulate N completed tasks, insert in single transaction

**Current architecture uses #2** (ConnectionPool has mutex). For higher worker counts, consider #3 (batch inserts) to reduce transaction overhead.

**Warning signs:** Import progress stalls, many workers idle, high `SQLITE_BUSY` count in logs.

**Source:** [SQLite concurrency challenges](https://www.drmhse.com/posts/battling-with-sqlite-in-a-concurrent-environment/)

### Pitfall 2: Main Thread Starvation During Import

**What goes wrong:** Import workers flood MainThreadQueue with thumbnail tasks faster than main thread can process, UI freezes or becomes unresponsive.

**Why it happens:** MainThreadQueue is bounded (default 1000), but if main thread frame time is high (e.g., user resizing window, complex rendering), queue fills and blocks workers OR queue drains slowly making import appear stalled.

**How to avoid:**
1. **Rate limit thumbnail generation:** Don't generate thumbnails for every imported file in real-time. Batch them or defer until user views library.
2. **Increase queue size:** For high worker counts (e.g., 12 threads on 16-core machine).
3. **Priority ordering:** Process thumbnail tasks at lower priority than UI interactions.

**Recommendation:** For this phase, keep thumbnail generation per-file (existing behavior) but increase MainThreadQueue default size based on thread count (e.g., `maxSize = 1000 + workerCount * 100`).

**Warning signs:** Import progress shows "Generating thumbnail" for extended periods, UI input lag during import.

**Source:** Current implementation at `src/core/threading/main_thread_queue.h` line 18 (default 1000).

### Pitfall 3: std::hardware_concurrency() Returns 0 or Incorrect Count

**What goes wrong:** On some systems (embedded, virtual machines, older compilers), `hardware_concurrency()` returns 0, causing thread pool initialization to fail or create zero workers.

**Why it happens:** Function is implementation-defined and can return 0 if unable to detect hardware parallelism. Also doesn't respect process affinity masks set by OS or container runtime.

**How to avoid:**
1. **Fallback value:** If 0, default to 4 threads (safe for most systems).
2. **User override:** Allow explicit thread count in Settings for systems where detection fails.
3. **Bounds checking:** Clamp to reasonable range (1-64) to prevent accidental configuration errors.

**Example:**
```cpp
unsigned int hwCores = std::thread::hardware_concurrency();
if (hwCores == 0) {
    log::warning("Import", "Could not detect CPU cores, defaulting to 4");
    hwCores = 4;
}
hwCores = std::clamp(hwCores, 1u, 64u);
```

**Warning signs:** Import uses only 1 thread on multi-core system, log shows "hardware_concurrency returned 0".

**Source:** [cppreference hardware_concurrency](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency), [NUMA detection issues](https://github.com/llvm/llvm-project/issues/72267)

### Pitfall 4: Cross-Filesystem Move Fails Silently

**What goes wrong:** User selects "move to library" mode, files are on different filesystem (e.g., network drive, different partition). `std::filesystem::rename` fails, import appears successful but files not moved.

**Why it happens:** POSIX `rename()` and Windows `MoveFile()` are atomic only within same filesystem. Cross-filesystem requires copy + delete.

**How to avoid:** Catch `std::filesystem::filesystem_error` on rename, fallback to copy + verify + delete:
```cpp
try {
    fs::rename(source, dest);
} catch (const fs::filesystem_error& e) {
    // Cross-filesystem, fallback to copy + delete
    fs::copy(source, dest, fs::copy_options::overwrite_existing);
    if (fs::exists(dest) && fs::file_size(dest) == fs::file_size(source)) {
        fs::remove(source);
    }
}
```

**Warning signs:** Files remain in original location after import with "move" mode, no error reported.

**Source:** [std::filesystem::rename](https://en.cppreference.com/w/cpp/filesystem/rename), [atomic file operations discussion](https://github.com/bobvanderlinden/sharpfilesystem/issues/8)

### Pitfall 5: G-code Parser Assumes Absolute Positioning

**What goes wrong:** Parser accumulates positions without tracking G90/G91 modal state, resulting in incorrect toolpath visualization for G-code using relative moves.

**Why it happens:** G-code is modal—G90 (absolute) and G91 (relative) affect interpretation of all subsequent position commands until changed. Parser must maintain state machine.

**How to avoid:** Track positioning mode and apply to all coordinate parameters:
```cpp
// Pseudo-code
Vec3 currentPos{0, 0, 0};
PositioningMode mode = PositioningMode::Absolute;

for (const Command& cmd : commands) {
    if (cmd.type == CommandType::G90) mode = PositioningMode::Absolute;
    if (cmd.type == CommandType::G91) mode = PositioningMode::Relative;

    if (cmd.isMotion()) {
        Vec3 targetPos = currentPos;
        if (cmd.hasX()) targetPos.x = mode == Absolute ? cmd.x : currentPos.x + cmd.x;
        if (cmd.hasY()) targetPos.y = mode == Absolute ? cmd.y : currentPos.y + cmd.y;
        if (cmd.hasZ()) targetPos.z = mode == Absolute ? cmd.z : currentPos.z + cmd.z;
        // Add segment from currentPos to targetPos
        currentPos = targetPos;
    }
}
```

**Warning signs:** Toolpath visualization jumps to wrong positions, doesn't match expected shape.

**Source:** Existing implementation at `src/core/gcode/gcode_types.h` includes `PositioningMode` enum (line 44), parser must use it.

### Pitfall 6: Toast Notifications Accumulate During Large Batch

**What goes wrong:** Importing 1000 files with errors generates 1000 toast notifications, overwhelming UI and causing performance issues.

**Why it happens:** Individual error notifications are useful for small batches but excessive for large batches.

**How to avoid:**
1. **Rate limiting:** Show toast for first N errors (e.g., 10), then silence and only update summary count.
2. **Batched notifications:** Replace individual toasts with single "X errors occurred" toast that updates count.
3. **User setting:** Honor "show error toasts" setting from Config (already in user decisions).

**Recommendation:** Show toasts if enabled AND batch size < 50 files. For larger batches, only show summary at end.

**Warning signs:** UI slows during large import, notification area filled with dozens of toasts.

**Source:** [Progress notifications best practices](https://notifee.app/react-native/docs/android/progress-indicators/)

## Code Examples

### Thread Pool Enqueue and Worker Pattern

```cpp
// Simplified thread pool for import pipeline
// Source: Adapted from https://github.com/progschj/ThreadPool pattern

class ImportThreadPool {
    std::vector<std::thread> m_workers;
    std::queue<ImportTask> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_shutdown{false};
    ConnectionPool& m_pool;

public:
    ImportThreadPool(ConnectionPool& pool, size_t numThreads)
        : m_pool(pool) {
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back(&ImportThreadPool::workerLoop, this);
        }
    }

    void enqueue(ImportTask task) {
        {
            std::lock_guard lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

private:
    void workerLoop() {
        while (!m_shutdown) {
            ImportTask task;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return !m_tasks.empty() || m_shutdown;
                });
                if (m_shutdown && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }

            // Process task with pooled connection
            ScopedConnection conn(m_pool);
            processTask(task, *conn);

            // Post completed task to main thread
            if (task.stage == ImportStage::WaitingForThumbnail) {
                // ... post to MainThreadQueue
            }
        }
    }
};
```

### G-code Import as MeshLoader

```cpp
// New file: src/core/loaders/gcode_loader.h
// Integrate G-code into existing LoaderFactory pattern

class GCodeLoader : public MeshLoader {
public:
    LoadResult load(const Path& path) override {
        gcode::Parser parser;
        auto program = parser.parseFile(path);

        if (program.commands.empty()) {
            return LoadResult::error("Empty or invalid G-code");
        }

        // Convert toolpath to mesh representation (lines)
        auto mesh = std::make_shared<Mesh>();
        for (const auto& segment : program.path) {
            // Add line segment as degenerate triangles or use custom geometry
            // ... (details depend on rendering approach)
        }

        LoadResult result;
        result.mesh = mesh;
        result.format = "gcode";
        return result;
    }

    LoadResult loadFromBuffer(const ByteBuffer& data) override {
        std::string content(data.begin(), data.end());
        gcode::Parser parser;
        auto program = parser.parse(content);
        // ... same as load()
    }
};

// Register in LoaderFactory::getLoaderByExtension():
if (ext == "gcode" || ext == "nc" || ext == "ngc") {
    return std::make_unique<GCodeLoader>();
}
```

### File Handling Mode Implementation

```cpp
// New: src/core/import/file_handler.h
// Handles copy/move/leave-in-place modes

enum class FileHandlingMode { LeaveInPlace, CopyToLibrary, MoveToLibrary };

class FileHandler {
public:
    static Path handleImportedFile(
        const Path& source,
        FileHandlingMode mode,
        const Path& libraryRoot) {

        if (mode == FileHandlingMode::LeaveInPlace) {
            return source; // No-op, return original path
        }

        // Generate destination path in library
        Path dest = libraryRoot / source.filename();

        // Ensure unique filename if collision
        int counter = 1;
        while (fs::exists(dest)) {
            auto stem = source.stem();
            auto ext = source.extension();
            dest = libraryRoot / (stem.string() + "_" +
                   std::to_string(counter++) + ext.string());
        }

        if (mode == FileHandlingMode::CopyToLibrary) {
            fs::copy(source, dest, fs::copy_options::overwrite_existing);
            return dest;
        }

        if (mode == FileHandlingMode::MoveToLibrary) {
            try {
                fs::rename(source, dest);
            } catch (const fs::filesystem_error&) {
                // Cross-filesystem, fallback to copy + delete
                fs::copy(source, dest, fs::copy_options::overwrite_existing);
                if (fs::exists(dest) &&
                    fs::file_size(dest) == fs::file_size(source)) {
                    fs::remove(source);
                }
            }
            return dest;
        }

        return source; // Fallback
    }
};
```

### Toast Notification Integration

```cpp
// Using ImGui-notify pattern (header-only)
// Source: https://github.com/TyomaVader/ImGuiNotify

// In UIManager or notification helper:
void showImportErrorToast(const std::string& filename, const std::string& error) {
    if (!Config::instance().getShowImportErrorToasts()) return;

    ImGui::InsertNotification({
        ImGuiToastType::Error,
        3000, // 3 seconds
        "Import Failed: %s",
        filename.c_str()
    });
}

// In main render loop:
void UIManager::renderToasts() {
    ImGui::RenderNotifications();
}
```

### Database Schema Extension for G-code

```cpp
// Extend src/core/database/schema.cpp createTables()

// G-code files table
db.execute(R"(
    CREATE TABLE IF NOT EXISTS gcode_files (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        hash TEXT UNIQUE NOT NULL,
        name TEXT NOT NULL,
        file_path TEXT NOT NULL,
        file_size INTEGER DEFAULT 0,
        bounds_min_x REAL DEFAULT 0,
        bounds_min_y REAL DEFAULT 0,
        bounds_min_z REAL DEFAULT 0,
        bounds_max_x REAL DEFAULT 0,
        bounds_max_y REAL DEFAULT 0,
        bounds_max_z REAL DEFAULT 0,
        total_distance REAL DEFAULT 0,
        cutting_distance REAL DEFAULT 0,
        rapid_distance REAL DEFAULT 0,
        estimated_time REAL DEFAULT 0,
        tool_numbers TEXT DEFAULT '[]',
        feed_rates TEXT DEFAULT '[]',
        imported_at TEXT DEFAULT CURRENT_TIMESTAMP,
        model_id INTEGER,
        operation_group TEXT,
        sort_order INTEGER DEFAULT 0,
        FOREIGN KEY (model_id) REFERENCES models(id) ON DELETE SET NULL
    )
)");

// Operation groups (optional, for templates)
db.execute(R"(
    CREATE TABLE IF NOT EXISTS gcode_templates (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        groups TEXT NOT NULL
    )
)");

// Insert CNC Router Basic template
db.execute(R"(
    INSERT OR IGNORE INTO gcode_templates (name, groups)
    VALUES ('CNC Router Basic', '["Roughing","Finishing","Profiling","Drilling"]')
)");
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single worker thread | Multi-worker thread pool with configurable parallelism | 2020s (thread pool pattern stable since C++11) | 4-12x speedup on multi-core systems for I/O bound workloads |
| MD5/SHA256 for deduplication | xxHash64 or similar non-crypto hash | ~2015-2020 (xxHash released 2012) | 50x performance improvement on large files, no security requirement for local deduplication |
| Modal progress dialog | Background status bar with notifications | ~2018+ (modern UX pattern) | Non-blocking, users can continue working during import |
| SQLite default journal | WAL (Write-Ahead Logging) mode | SQLite 3.7.0+ (2010) | Concurrent reads don't block each other or writer, critical for multi-threaded access |

**Deprecated/outdated:**
- **boost::thread and boost::filesystem:** Superseded by C++11 `<thread>` and C++17 `<filesystem>`. No reason to use Boost for these anymore unless targeting pre-C++17 compilers.
- **Hand-rolled CRC32 for file integrity:** Modern non-cryptographic hashes (xxHash, MurmurHash) are faster and higher quality. CRC32 is for error detection, not deduplication.
- **Polling file existence for progress:** Modern approach uses atomic counters and lock-free reads (as seen in current `ImportProgress` struct).

## Open Questions

1. **Should G-code toolpath rendering use custom geometry or convert to mesh?**
   - What we know: Existing renderer handles triangle meshes, not line primitives. G-code toolpaths are inherently lines (start/end points).
   - What's unclear: Performance tradeoff of converting lines to thin triangulated tubes vs adding line rendering shader path.
   - Recommendation: For Phase 2, convert to mesh (degenerate triangles or extruded lines). Defer optimized line rendering to future rendering overhaul. Keeps changes localized to import pipeline.

2. **How to handle G-code with missing modal state (e.g., no G90/G91 specified)?**
   - What we know: Grbl defaults to absolute positioning (G90) on startup.
   - What's unclear: Whether to enforce strict parsing (reject files without explicit mode) or assume defaults.
   - Recommendation: Follow Grbl convention—assume G90 (absolute), G21 (millimeters) as initial state. Log warning if file doesn't specify. Matches user expectations from CNC tools.

3. **Batch insert optimization: worth the complexity?**
   - What we know: Each worker currently inserts one record per file. For 100 files with 8 workers, ~100 transactions. SQLite transaction overhead is non-trivial.
   - What's unclear: Whether batching (e.g., accumulate 10 records, insert in single transaction) provides measurable speedup vs added coordination complexity.
   - Recommendation: Start with per-file inserts (current pattern). Profile on real workloads. If DB insert is >10% of total time, revisit batching.

4. **Managed library directory structure: flat vs hierarchical?**
   - What we know: User constraint says files go to "managed library directory" but doesn't specify structure.
   - What's unclear: Flat (all files in one dir) vs hierarchical (subdirs by date, type, first letter).
   - Recommendation: Start flat for simplicity. Single directory until user has >1000 files. If performance degrades (slow filesystem listing), add date-based subdirs (YYYY/MM/filename). Defer to actual usage data.

5. **Thread pool creation timing: startup vs first import?**
   - What we know: Thread creation is expensive (~1ms per thread). Creating 12 threads at app startup adds ~12ms to startup time.
   - What's unclear: Whether to pre-create pool or lazy-initialize on first import.
   - Recommendation: Lazy initialization. Most users won't import files every session. Create pool on first enqueue, keep alive until app shutdown.

## Sources

### Primary (HIGH confidence)

- C++ Standard Library documentation:
  - [std::thread](https://en.cppreference.com/w/cpp/thread/thread)
  - [std::hardware_concurrency](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency)
  - [std::filesystem](https://en.cppreference.com/w/cpp/filesystem)
  - [std::filesystem::recursive_directory_iterator](https://en.cppreference.com/w/cpp/filesystem/recursive_directory_iterator)

- SQLite official documentation:
  - [Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html)

- Existing codebase files (verified via Read tool):
  - `/data/DW/src/core/import/import_queue.h` - Current single-worker implementation
  - `/data/DW/src/core/import/import_task.h` - Pipeline stages and progress tracking
  - `/data/DW/src/core/database/schema.cpp` - Current database schema
  - `/data/DW/src/core/database/connection_pool.h` - Thread-safe connection pooling
  - `/data/DW/src/core/threading/main_thread_queue.h` - Main thread task posting
  - `/data/DW/src/core/gcode/gcode_parser.h` - Existing G-code parser
  - `/data/DW/src/core/gcode/gcode_types.h` - G-code data structures
  - `/data/DW/src/core/mesh/hash.h` - Current FNV-1a hashing implementation
  - `/data/DW/src/core/loaders/loader_factory.h` - Format loader registration

### Secondary (MEDIUM confidence)

- [GitHub - progschj/ThreadPool](https://github.com/progschj/ThreadPool) - Reference C++11 thread pool implementation (verified widespread use)
- [GitHub - bshoshany/thread-pool](https://github.com/bshoshany/thread-pool) - Modern C++17 thread pool library
- [xxHash benchmark](https://create.stephan-brumme.com/xxhash/) - Performance comparison vs SHA256 (50x faster)
- [SQLite concurrent access patterns](https://www.slingacademy.com/article/managing-concurrent-access-in-sqlite-databases/) - WAL mode usage
- [Grbl compatibility - FluidNC Wiki](http://wiki.fluidnc.com/en/features/grbl_compatibility) - G-code dialect compatibility
- [grblHAL core repository](https://github.com/grblHAL/core) - Grbl fork specifications
- [ImGuiNotify](https://github.com/TyomaVader/ImGuiNotify) - Toast notification implementation for ImGui
- [AtomicWrite](https://github.com/jeffreyrogers/AtomicWrite) - Cross-platform atomic file operations

### Tertiary (LOW confidence - require validation)

- [Thread pool best practices - GeeksforGeeks](https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/) - General overview, not specific implementation guidance
- [C++ OpenGL line rendering techniques](https://github.com/mhalber/Lines) - 6 approaches for wide line rendering (relevant if custom line rendering chosen)
- [Batch import error patterns - various sources](https://docs.spring.io/spring-batch/docs/current/reference/html/common-patterns.html) - Enterprise patterns, not directly applicable to desktop app

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - C++ standard library features well-documented, existing codebase already uses threading primitives
- Architecture: MEDIUM-HIGH - Patterns verified from multiple sources and existing code, but thread pool adaptation to import pipeline needs validation
- Pitfalls: MEDIUM - SQLite concurrency issues well-documented, others based on common C++ threading pitfalls but not specifically tested in this codebase
- G-code parsing: MEDIUM - Parser exists, Grbl compatibility documented, but metadata extraction approach (for import) needs implementation validation

**Research date:** 2026-02-09
**Valid until:** 60 days (stable technologies—C++17, SQLite, Grbl specification)

**Areas requiring validation during implementation:**
1. Optimal ConnectionPool size for N workers (likely 2*N for read/write overlap)
2. MainThreadQueue size scaling with thread count (current default 1000 may be insufficient)
3. G-code rendering approach (mesh conversion vs line rendering)
4. Actual performance gain from parallel workers vs single worker (I/O vs CPU bottleneck)
5. Managed library directory structure performance at scale (>1000 files)
