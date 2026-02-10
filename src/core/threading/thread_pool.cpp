#include "thread_pool.h"

#include <algorithm>

namespace dw {

size_t calculateThreadCount(ParallelismTier tier) {
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) {
        cores = 4; // Fallback if detection fails
    }

    size_t threadCount = 0;
    switch (tier) {
    case ParallelismTier::Auto:
        threadCount = static_cast<size_t>(cores * 0.6);
        break;
    case ParallelismTier::Fixed:
        threadCount = static_cast<size_t>(cores * 0.9);
        break;
    case ParallelismTier::Expert:
        threadCount = cores;
        break;
    }

    // Clamp to range [1, 64]
    threadCount = std::max(size_t(1), std::min(size_t(64), threadCount));
    return threadCount;
}

ThreadPool::ThreadPool(size_t numThreads) {
    // Create worker threads immediately
    m_workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::enqueue(std::function<void()> task) {
    if (m_shutdown.load()) {
        return; // Don't accept tasks after shutdown
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_taskQueue.push(std::move(task));
    }
    m_condition.notify_one();
}

void ThreadPool::shutdown() {
    if (m_shutdown.exchange(true)) {
        return; // Already shut down
    }

    // Wake all workers so they can exit
    m_condition.notify_all();

    // Join all worker threads
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

bool ThreadPool::isIdle() const {
    return m_taskQueue.empty() && m_activeCount.load() == 0;
}

size_t ThreadPool::pendingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_taskQueue.size();
}

size_t ThreadPool::activeCount() const {
    return m_activeCount.load();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Wait for task or shutdown signal
            m_condition.wait(lock, [this] { return m_shutdown.load() || !m_taskQueue.empty(); });

            // Exit if shutdown and queue is empty
            if (m_shutdown.load() && m_taskQueue.empty()) {
                return;
            }

            // Dequeue task if available
            if (!m_taskQueue.empty()) {
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
        }

        // Execute task outside lock
        if (task) {
            m_activeCount.fetch_add(1);
            task();
            m_activeCount.fetch_sub(1);
        }
    }
}

} // namespace dw
