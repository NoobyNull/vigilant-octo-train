#pragma once

#include "../types.h"

#include <cstdint>
#include <functional>

namespace dw {

// Watches a file for modification and fires a callback when it changes.
// Uses mtime polling — call poll() from the main loop.
class ConfigWatcher {
public:
    using Callback = std::function<void()>;

    // Start watching the given file path
    void watch(const Path& path, uint32_t pollIntervalMs = 500);

    // Stop watching
    void stop();

    // Set callback for when file changes
    void setOnChanged(Callback cb) { m_callback = std::move(cb); }

    // Call from main loop. Checks mtime at the configured interval.
    void poll(uint64_t nowMs);

private:
    Path m_path;
    Callback m_callback;
    int64_t m_lastMtime = 0;
    uint64_t m_lastPollMs = 0;
    uint32_t m_intervalMs = 500;
    bool m_watching = false;
};

}  // namespace dw
