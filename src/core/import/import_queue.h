#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "../database/connection_pool.h"
#include "import_task.h"

namespace dw {

class ImportQueue {
  public:
    explicit ImportQueue(ConnectionPool& pool);
    ~ImportQueue();

    // Enqueue files for import (called from main thread)
    void enqueue(const std::vector<Path>& paths);
    void enqueue(const Path& path);

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

  private:
    void workerLoop();
    void processTask(ImportTask& task);

    ConnectionPool& m_pool;

    // Thread management
    std::thread m_worker;
    std::mutex m_mutex;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_cancelRequested{false};

    // Task queues (protected by m_mutex)
    std::vector<ImportTask> m_pending;
    std::vector<ImportTask> m_completed; // Ready for main-thread thumbnail

    ImportProgress m_progress;
    ImportCallback m_onComplete;
};

} // namespace dw
