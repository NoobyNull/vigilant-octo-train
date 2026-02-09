#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "../types.h"

namespace dw {

// Forward declaration
class Database;

// ConnectionPool manages a fixed-size pool of Database connections for thread-safe access
class ConnectionPool {
  public:
    // Create a pool with poolSize connections to the given database path
    ConnectionPool(const Path& dbPath, size_t poolSize = 2);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    // Acquire a connection from the pool (throws std::runtime_error if exhausted)
    Database* acquire();

    // Release a connection back to the pool (ignores nullptr)
    void release(Database* conn);

    // Query pool state (thread-safe)
    size_t availableCount() const;
    size_t inUseCount() const;
    size_t totalSize() const { return m_poolSize; }

  private:
    Path m_dbPath;
    std::deque<std::unique_ptr<Database>> m_available;
    std::vector<Database*> m_inUse;
    mutable std::mutex m_mutex;
    size_t m_poolSize;
};

// ScopedConnection provides RAII-based connection acquisition and release
class ScopedConnection {
  public:
    explicit ScopedConnection(ConnectionPool& pool);
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    // Move semantics
    ScopedConnection(ScopedConnection&& other) noexcept;
    ScopedConnection& operator=(ScopedConnection&& other) noexcept;

    // Access the connection
    Database* get() const { return m_conn; }
    Database* operator->() const { return m_conn; }
    Database& operator*() const { return *m_conn; }
    explicit operator bool() const { return m_conn != nullptr; }

  private:
    ConnectionPool* m_pool = nullptr;
    Database* m_conn = nullptr;
};

} // namespace dw
