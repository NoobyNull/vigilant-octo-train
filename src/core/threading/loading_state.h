#pragma once

// Digital Workshop - Loading State
// Thread-safe state shared between the model-loading worker thread and the UI.

#include <atomic>
#include <mutex>
#include <string>

namespace dw {

struct LoadingState {
    std::atomic<bool> active{false};
    std::atomic<uint64_t> generation{0}; // invalidates stale loads

    void set(const std::string& name) {
        std::lock_guard lock(m_nameMutex);
        m_modelName = name;
        active.store(true);
    }

    std::string getName() const {
        std::lock_guard lock(m_nameMutex);
        return m_modelName;
    }

    void reset() {
        active.store(false);
        std::lock_guard lock(m_nameMutex);
        m_modelName.clear();
    }

  private:
    mutable std::mutex m_nameMutex;
    std::string m_modelName;
};

} // namespace dw
