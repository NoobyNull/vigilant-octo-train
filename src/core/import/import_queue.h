#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "../database/connection_pool.h"
#include "../threading/thread_pool.h"
#include "import_task.h"

namespace dw {

// Forward declarations
class LibraryManager;
class StorageManager;

class ImportQueue {
  public:
    explicit ImportQueue(ConnectionPool& pool, LibraryManager* libraryManager = nullptr,
                         StorageManager* storageManager = nullptr);
    ~ImportQueue();

    // Enqueue files for import (called from main thread)
    void enqueue(const std::vector<Path>& paths);
    void enqueue(const Path& path);

    // Re-enqueue selected duplicates (skips duplicate check)
    void enqueueForReimport(const std::vector<DuplicateRecord>& duplicates);

    // Cancel all pending imports
    void cancel();

    // Check if queue is actively processing
    bool isActive() const;

    // Get progress (thread-safe, lock-free reads)
    const ImportProgress& progress() const;

    // Poll for completed tasks that need main-thread work (thumbnail gen)
    // Returns tasks ready for GL operations. Caller takes ownership.
    std::vector<ImportTask> pollCompleted();

    // Mark a task as fully done after thumbnail generation
    void taskFinished(i64 modelId);

    // Set callback for when a task finishes completely (after thumbnail)
    using ImportCallback = std::function<void(const ImportTask& task)>;
    void setOnComplete(ImportCallback callback);

    // Get batch summary (thread-safe read)
    const ImportBatchSummary& batchSummary() const;

    // Set callback for when batch completes
    using SummaryCallback = std::function<void(const ImportBatchSummary&)>;
    void setOnBatchComplete(SummaryCallback callback);

  private:
    void processTask(ImportTask task); // Note: takes by value for move into lambda

    ConnectionPool& m_pool;
    LibraryManager* m_libraryManager;   // Optional, for auto-detect
    StorageManager* m_storageManager;    // Optional, for CAS blob storage

    // Thread management
    std::unique_ptr<ThreadPool> m_threadPool; // Lazily created
    std::mutex m_mutex;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_cancelRequested{false};

    // Task queues (protected by m_mutex)
    std::vector<ImportTask> m_completed; // Ready for main-thread thumbnail

    // Batch tracking
    mutable std::mutex m_summaryMutex;
    ImportBatchSummary m_batchSummary;
    std::atomic<int> m_remainingTasks{0};

    ImportProgress m_progress;
    ImportCallback m_onComplete;
    SummaryCallback m_onBatchComplete;
};

} // namespace dw
