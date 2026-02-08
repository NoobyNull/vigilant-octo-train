#include "config_watcher.h"

#include <system_error>

#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

void ConfigWatcher::watch(const Path& path, uint32_t pollIntervalMs) {
    m_path = path;
    m_intervalMs = pollIntervalMs;
    m_watching = true;
    m_lastPollMs = 0;

    // Capture initial mtime
    std::error_code ec;
    auto ftime = fs::last_write_time(m_path, ec);
    if (!ec) {
        m_lastMtime = ftime.time_since_epoch().count();
    }

    log::infof("Config", "Watching %s (poll %ums)", m_path.string().c_str(), m_intervalMs);
}

void ConfigWatcher::stop() {
    m_watching = false;
}

void ConfigWatcher::poll(uint64_t nowMs) {
    if (!m_watching)
        return;

    // Throttle by interval
    if (nowMs - m_lastPollMs < m_intervalMs)
        return;
    m_lastPollMs = nowMs;

    std::error_code ec;
    auto ftime = fs::last_write_time(m_path, ec);
    if (ec)
        return;

    auto mtime = ftime.time_since_epoch().count();
    if (mtime != m_lastMtime) {
        m_lastMtime = mtime;
        log::debug("Config", "Config file changed on disk");
        if (m_callback) {
            m_callback();
        }
    }
}

} // namespace dw
