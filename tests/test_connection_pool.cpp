#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "core/database/connection_pool.h"
#include "core/database/database.h"

using namespace dw;

class ConnectionPoolTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_testDbPath = std::filesystem::temp_directory_path() / "test_pool.db";
        std::filesystem::remove(m_testDbPath);
    }

    void TearDown() override {
        std::filesystem::remove(m_testDbPath);
    }

    Path m_testDbPath;
};

// Test 1: Pool creates connections and reports correct available count
TEST_F(ConnectionPoolTest, ConstructorCreatesConnections) {
    ConnectionPool pool(m_testDbPath, 2);
    EXPECT_EQ(pool.availableCount(), 2);
    EXPECT_EQ(pool.inUseCount(), 0);
    EXPECT_EQ(pool.totalSize(), 2);
}

// Test 2: acquire() returns valid Database pointer and decrements available count
TEST_F(ConnectionPoolTest, AcquireReturnsValidConnection) {
    ConnectionPool pool(m_testDbPath, 2);

    Database* conn = pool.acquire();
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->isOpen());
    EXPECT_EQ(pool.availableCount(), 1);
    EXPECT_EQ(pool.inUseCount(), 1);

    pool.release(conn);
}

// Test 3: release() returns connection to pool
TEST_F(ConnectionPoolTest, ReleaseReturnsConnectionToPool) {
    ConnectionPool pool(m_testDbPath, 2);

    Database* conn = pool.acquire();
    EXPECT_EQ(pool.availableCount(), 1);

    pool.release(conn);
    EXPECT_EQ(pool.availableCount(), 2);
    EXPECT_EQ(pool.inUseCount(), 0);
}

// Test 4: Pool exhaustion throws exception
TEST_F(ConnectionPoolTest, ExhaustionThrowsException) {
    ConnectionPool pool(m_testDbPath, 2);

    Database* conn1 = pool.acquire();
    Database* conn2 = pool.acquire();
    EXPECT_EQ(pool.availableCount(), 0);
    EXPECT_EQ(pool.inUseCount(), 2);

    EXPECT_THROW(pool.acquire(), std::runtime_error);

    pool.release(conn1);
    pool.release(conn2);
}

// Test 5: ScopedConnection acquires and auto-releases
TEST_F(ConnectionPoolTest, ScopedConnectionAutoReleases) {
    ConnectionPool pool(m_testDbPath, 2);
    EXPECT_EQ(pool.availableCount(), 2);

    {
        ScopedConnection scoped(pool);
        EXPECT_EQ(pool.availableCount(), 1);
        EXPECT_NE(scoped.get(), nullptr);
    }

    // Connection should be released after scope exit
    EXPECT_EQ(pool.availableCount(), 2);
}

// Test 6: ScopedConnection provides Database access via operators
TEST_F(ConnectionPoolTest, ScopedConnectionOperators) {
    ConnectionPool pool(m_testDbPath, 2);

    ScopedConnection scoped(pool);
    ASSERT_TRUE(scoped);

    // Test operator->
    EXPECT_TRUE(scoped->isOpen());

    // Test operator*
    Database& db = *scoped;
    EXPECT_TRUE(db.isOpen());

    // Test get()
    Database* dbPtr = scoped.get();
    ASSERT_NE(dbPtr, nullptr);
    EXPECT_TRUE(dbPtr->isOpen());
}

// Test 7: ScopedConnection move constructor transfers ownership
TEST_F(ConnectionPoolTest, ScopedConnectionMoveConstructor) {
    ConnectionPool pool(m_testDbPath, 2);

    ScopedConnection scoped1(pool);
    Database* conn1 = scoped1.get();
    EXPECT_EQ(pool.availableCount(), 1);

    ScopedConnection scoped2(std::move(scoped1));
    EXPECT_EQ(scoped2.get(), conn1);
    EXPECT_FALSE(scoped1);
    EXPECT_EQ(pool.availableCount(), 1); // Still only 1 acquired
}

// Test 8: ScopedConnection move assignment transfers ownership
TEST_F(ConnectionPoolTest, ScopedConnectionMoveAssignment) {
    ConnectionPool pool(m_testDbPath, 2);

    ScopedConnection scoped1(pool);
    Database* conn1 = scoped1.get();

    ScopedConnection scoped2(pool);
    Database* conn2 = scoped2.get();
    EXPECT_EQ(pool.availableCount(), 0);

    scoped2 = std::move(scoped1);
    EXPECT_EQ(scoped2.get(), conn1);
    EXPECT_FALSE(scoped1);
    EXPECT_EQ(pool.availableCount(), 1); // conn2 released, conn1 still held
}

// Test 9: Pooled connections have WAL mode enabled
TEST_F(ConnectionPoolTest, PooledConnectionsHaveWALMode) {
    ConnectionPool pool(m_testDbPath, 2);

    ScopedConnection scoped(pool);
    auto stmt = scoped->prepare("PRAGMA journal_mode");
    ASSERT_TRUE(stmt.isValid());
    ASSERT_TRUE(stmt.step());

    std::string mode = stmt.getText(0);
    EXPECT_EQ(mode, "wal");
}

// Test 10: Pooled connections have synchronous=NORMAL
TEST_F(ConnectionPoolTest, PooledConnectionsHaveSynchronousNormal) {
    ConnectionPool pool(m_testDbPath, 2);

    ScopedConnection scoped(pool);
    auto stmt = scoped->prepare("PRAGMA synchronous");
    ASSERT_TRUE(stmt.isValid());
    ASSERT_TRUE(stmt.step());

    i64 synchronous = stmt.getInt(0);
    EXPECT_EQ(synchronous, 1); // 1 = NORMAL
}

// Test 11: Concurrent acquire/release from multiple threads
TEST_F(ConnectionPoolTest, ConcurrentAccessIsThreadSafe) {
    ConnectionPool pool(m_testDbPath, 4);

    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&pool, &successCount]() {
            try {
                ScopedConnection scoped(pool);
                if (scoped) {
                    // Perform a simple query
                    auto stmt = scoped->prepare("SELECT 1");
                    if (stmt.step()) {
                        successCount++;
                    }
                }
            } catch (const std::runtime_error&) {
                // Pool exhaustion is expected
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GT(successCount.load(), 0);
    EXPECT_EQ(pool.availableCount(), 4); // All connections returned
}

// Test 12: Pool destructor closes all connections cleanly
TEST_F(ConnectionPoolTest, DestructorClosesConnections) {
    {
        ConnectionPool pool(m_testDbPath, 2);
        Database* conn = pool.acquire();
        EXPECT_TRUE(conn->isOpen());
        pool.release(conn);
    }

    // Pool destroyed - verify database file exists but can be opened again
    Database db;
    EXPECT_TRUE(db.open(m_testDbPath));
    db.close();
}

// Test 13: release() handles nullptr gracefully
TEST_F(ConnectionPoolTest, ReleaseHandlesNullptr) {
    ConnectionPool pool(m_testDbPath, 2);

    EXPECT_NO_THROW(pool.release(nullptr));
    EXPECT_EQ(pool.availableCount(), 2);
}
