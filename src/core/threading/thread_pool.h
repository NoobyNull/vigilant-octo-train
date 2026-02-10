#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace dw {

// Parallelism tier for automatic thread count calculation
enum class ParallelismTier {
    Auto = 0,   // 60% of cores (balanced, leaves headroom for UI/system)
    Fixed = 1,  // 90% of cores (high throughput, minimal system reserve)
    Expert = 2, // 100% of cores (maximum parallelism)
};

// Calculate optimal thread count based on parallelism tier
// Returns clamped value in range [1, 64]
size_t calculateThreadCount(ParallelismTier tier);

// Generic bounded thread pool for parallel task execution
// Workers dequeue and execute tasks concurrently until shutdown
class ThreadPool {
  public:
    // Create thread pool with specified number of worker threads
    // Workers start immediately and wait for tasks
    explicit ThreadPool(size_t numThreads);

    // Shutdown pool and join all threads (executes remaining tasks)
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueue a task for execution by any available worker
    // Thread-safe, may block briefly if queue lock is contended
    void enqueue(std::function<void()> task);

    // Signal shutdown and wait for all workers to finish
    // Remaining queued tasks are executed before threads exit
    void shutdown();

    // Check if pool is idle (no pending or active tasks)
    bool isIdle() const;

    // Get count of pending tasks in queue
    size_t pendingCount() const;

    // Get count of currently executing tasks
    size_t activeCount() const;

  private:
    void workerLoop();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex m_mutex;
    std::condition_variable m_condition;

    std::atomic<size_t> m_activeCount{0};
    std::atomic<bool> m_shutdown{false};
};

} // namespace dw
