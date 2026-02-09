#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "core/utils/thread_utils.h"

namespace dw {

// Thread-safe bounded FIFO queue for posting callables from worker threads to main thread
// Workers enqueue tasks (may block if full), main thread calls processAll() each frame
class MainThreadQueue {
  public:
    explicit MainThreadQueue(size_t maxSize = 1000);

    // Enqueue a task from any thread (blocks if queue full, returns immediately if shutdown)
    void enqueue(std::function<void()> task);

    // Process all pending tasks on main thread (drains queue and executes callbacks)
    void processAll();

    // Query approximate queue size (lock-free)
    size_t size() const;

    // Shutdown the queue (unblocks waiting producers, prevents further enqueues)
    void shutdown();

  private:
    std::queue<std::function<void()>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cvFull; // Notifies producers when space available
    size_t m_maxSize;
    std::atomic<size_t> m_size{0};
    std::atomic<bool> m_shutdown{false};
};

} // namespace dw
