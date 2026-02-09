#include "connection_pool.h"

#include <algorithm>

#include <sqlite3.h>

#include "database.h"
#include "../utils/log.h"

namespace dw {

// ConnectionPool implementation

ConnectionPool::ConnectionPool(const Path& dbPath, size_t poolSize)
    : m_dbPath(dbPath), m_poolSize(poolSize) {

    if (poolSize == 0) {
        throw std::runtime_error("ConnectionPool size must be at least 1");
    }

    // Create all connections upfront
    for (size_t i = 0; i < poolSize; ++i) {
        auto db = std::make_unique<Database>();
        if (!db->openWithFlags(m_dbPath, SQLITE_OPEN_NOMUTEX)) {
            throw std::runtime_error("Failed to open database connection for pool");
        }
        m_available.push_back(std::move(db));
    }

    log::infof("ConnectionPool", "Created pool with %zu connections to %s", poolSize,
               dbPath.string().c_str());
}

ConnectionPool::~ConnectionPool() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_available.clear(); // unique_ptr destructors close databases
    m_inUse.clear();
    log::debug("ConnectionPool", "Destroyed");
}

Database* ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_available.empty()) {
        throw std::runtime_error("ConnectionPool exhausted - all connections are in use");
    }

    // Get connection from front
    std::unique_ptr<Database> db = std::move(m_available.front());
    m_available.pop_front();

    // Track as in-use
    Database* raw = db.release();
    m_inUse.push_back(raw);

    return raw;
}

void ConnectionPool::release(Database* conn) {
    if (conn == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Remove from in-use tracking
    auto it = std::find(m_inUse.begin(), m_inUse.end(), conn);
    if (it != m_inUse.end()) {
        m_inUse.erase(it);
    }

    // Return to available pool
    m_available.push_back(std::unique_ptr<Database>(conn));
}

size_t ConnectionPool::availableCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_available.size();
}

size_t ConnectionPool::inUseCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_inUse.size();
}

// ScopedConnection implementation

ScopedConnection::ScopedConnection(ConnectionPool& pool) : m_pool(&pool), m_conn(pool.acquire()) {}

ScopedConnection::~ScopedConnection() {
    if (m_pool && m_conn) {
        m_pool->release(m_conn);
    }
}

ScopedConnection::ScopedConnection(ScopedConnection&& other) noexcept
    : m_pool(other.m_pool), m_conn(other.m_conn) {
    other.m_pool = nullptr;
    other.m_conn = nullptr;
}

ScopedConnection& ScopedConnection::operator=(ScopedConnection&& other) noexcept {
    if (this != &other) {
        // Release current connection if we have one
        if (m_pool && m_conn) {
            m_pool->release(m_conn);
        }

        // Take ownership from other
        m_pool = other.m_pool;
        m_conn = other.m_conn;

        // Nullify other
        other.m_pool = nullptr;
        other.m_conn = nullptr;
    }
    return *this;
}

} // namespace dw
