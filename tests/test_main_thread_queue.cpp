// Test suite for MainThreadQueue and thread_utils
// Following TDD RED-GREEN pattern: write tests first (expect to fail)

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "core/threading/main_thread_queue.h"
#include "core/utils/thread_utils.h"

using namespace dw;

// Test fixture with thread initialization
class MainThreadQueueTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize main thread for thread_utils assertions
        threading::initMainThread();
    }
};

// Test 1: Enqueue single task, processAll executes it
TEST_F(MainThreadQueueTest, EnqueueSingleTask) {
    MainThreadQueue queue;
    std::atomic<int> counter{0};

    queue.enqueue([&counter]() { counter++; });

    EXPECT_EQ(counter.load(), 0); // Not executed yet
    queue.processAll();
    EXPECT_EQ(counter.load(), 1); // Executed after processAll
}

// Test 2: Enqueue multiple tasks, processAll executes all in FIFO order
TEST_F(MainThreadQueueTest, FIFOOrder) {
    MainThreadQueue queue;
    std::vector<int> order;

    queue.enqueue([&order]() { order.push_back(1); });
    queue.enqueue([&order]() { order.push_back(2); });
    queue.enqueue([&order]() { order.push_back(3); });

    queue.processAll();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// Test 3: processAll on empty queue does nothing (no crash)
TEST_F(MainThreadQueueTest, ProcessEmptyQueue) {
    MainThreadQueue queue;

    // Should not crash
    EXPECT_NO_THROW(queue.processAll());
    EXPECT_NO_THROW(queue.processAll());
}

// Test 4: size() returns correct count before and after processAll
TEST_F(MainThreadQueueTest, SizeTracking) {
    MainThreadQueue queue;

    EXPECT_EQ(queue.size(), 0);

    queue.enqueue([]() {});
    queue.enqueue([]() {});
    queue.enqueue([]() {});

    EXPECT_EQ(queue.size(), 3);

    queue.processAll();
    EXPECT_EQ(queue.size(), 0);
}

// Test 5: Cross-thread enqueue - spawn thread that enqueues 100 tasks
TEST_F(MainThreadQueueTest, CrossThreadEnqueue) {
    MainThreadQueue queue;
    std::atomic<int> counter{0};

    // Spawn worker thread to enqueue tasks
    std::thread worker([&queue, &counter]() {
        for (int i = 0; i < 100; ++i) {
            queue.enqueue([&counter]() { counter++; });
        }
    });

    worker.join();

    // Verify all tasks enqueued
    EXPECT_EQ(queue.size(), 100);

    // Process on main thread
    queue.processAll();

    // Verify all executed
    EXPECT_EQ(counter.load(), 100);
    EXPECT_EQ(queue.size(), 0);
}

// Test 6: shutdown() causes blocked enqueue to return without adding
TEST_F(MainThreadQueueTest, ShutdownUnblocksEnqueue) {
    MainThreadQueue queue(2); // Small queue for easy blocking

    // Fill the queue
    queue.enqueue([]() {});
    queue.enqueue([]() {});

    EXPECT_EQ(queue.size(), 2);

    // Spawn thread that will block on enqueue
    std::atomic<bool> enqueueReturned{false};
    std::thread worker([&queue, &enqueueReturned]() {
        queue.enqueue([]() {}); // This should block
        enqueueReturned = true;
    });

    // Give worker time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Shutdown should unblock the worker
    queue.shutdown();

    worker.join();

    // Worker should have returned without adding task
    EXPECT_TRUE(enqueueReturned.load());
    EXPECT_EQ(queue.size(), 2); // Still 2, not 3
}

// Test 7: After shutdown, enqueue is a no-op
TEST_F(MainThreadQueueTest, EnqueueAfterShutdown) {
    MainThreadQueue queue;

    queue.shutdown();

    queue.enqueue([]() {});
    queue.enqueue([]() {});

    EXPECT_EQ(queue.size(), 0); // No tasks added
}

// Test 8: Multiple processAll calls - second call processes new tasks
TEST_F(MainThreadQueueTest, MultipleProcessCalls) {
    MainThreadQueue queue;
    std::atomic<int> counter{0};

    // First batch
    queue.enqueue([&counter]() { counter++; });
    queue.enqueue([&counter]() { counter++; });

    queue.processAll();
    EXPECT_EQ(counter.load(), 2);

    // Second batch
    queue.enqueue([&counter]() { counter++; });
    queue.enqueue([&counter]() { counter++; });

    queue.processAll();
    EXPECT_EQ(counter.load(), 4);
}

// Test 9: isMainThread() returns true on thread that called initMainThread()
TEST_F(MainThreadQueueTest, IsMainThreadTrue) {
    // SetUp already called initMainThread()
    EXPECT_TRUE(threading::isMainThread());
}

// Test 10: isMainThread() returns false on a different thread
TEST_F(MainThreadQueueTest, IsMainThreadFalse) {
    std::atomic<bool> result{true};

    std::thread worker([&result]() { result = threading::isMainThread(); });

    worker.join();

    EXPECT_FALSE(result.load());
}
