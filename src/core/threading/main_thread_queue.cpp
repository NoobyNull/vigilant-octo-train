#include "main_thread_queue.h"

namespace dw {

MainThreadQueue::MainThreadQueue(size_t maxSize) : m_maxSize(maxSize) {}

void MainThreadQueue::enqueue(std::function<void()> task) {
    std::unique_lock<std::mutex> lock(m_mutex);

    // Wait if queue is full (blocks producer until space available or shutdown)
    m_cvFull.wait(lock, [this] { return m_size.load() < m_maxSize || m_shutdown.load(); });

    // If shutdown, return immediately without enqueuing
    if (m_shutdown.load()) {
        return;
    }

    // Add task to queue
    m_queue.push(std::move(task));
    m_size.fetch_add(1);
}

void MainThreadQueue::processAll() {
    // Assert we're on the main thread (debug only)
    ASSERT_MAIN_THREAD();

    // Drain queue into local vector (minimize lock time)
    std::vector<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            tasks.push_back(std::move(m_queue.front()));
            m_queue.pop();
            m_size.fetch_sub(1);
        }
        // Notify blocked producers that space is available
        m_cvFull.notify_all();
    }

    // Execute all callbacks outside the lock (prevents deadlocks)
    for (auto& task : tasks) {
        task();
    }
}

size_t MainThreadQueue::size() const {
    return m_size.load();
}

void MainThreadQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    // Wake all waiting producers
    m_cvFull.notify_all();
}

} // namespace dw
