// Digital Workshop - Config Watcher Tests

#include <gtest/gtest.h>

#include "core/config/config_watcher.h"
#include "core/utils/file_utils.h"

#include <filesystem>
#include <thread>

namespace {

class ConfigWatcherTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_tmpDir = std::filesystem::temp_directory_path() / "dw_test_watcher";
        std::filesystem::create_directories(m_tmpDir);
        m_filePath = m_tmpDir / "config.json";
        ASSERT_TRUE(dw::file::writeText(m_filePath, "{}"));
    }

    void TearDown() override { std::filesystem::remove_all(m_tmpDir); }

    std::filesystem::path m_tmpDir;
    dw::Path m_filePath;
};

} // namespace

TEST_F(ConfigWatcherTest, CallbackNotFiredBeforeInterval) {
    dw::ConfigWatcher watcher;
    int callCount = 0;
    watcher.setOnChanged([&]() { callCount++; });
    watcher.watch(m_filePath, 1000); // 1 second interval

    // Poll immediately — should not fire (interval not elapsed)
    watcher.poll(0);
    watcher.poll(500);
    EXPECT_EQ(callCount, 0);
}

TEST_F(ConfigWatcherTest, CallbackFiredAfterFileChange) {
    dw::ConfigWatcher watcher;
    int callCount = 0;
    watcher.setOnChanged([&]() { callCount++; });
    watcher.watch(m_filePath, 100);

    // Wait and modify the file
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(dw::file::writeText(m_filePath, "{\"changed\": true}"));

    // Poll after interval
    watcher.poll(200);

    EXPECT_GE(callCount, 1);
}

TEST_F(ConfigWatcherTest, StopSuppressesCallback) {
    dw::ConfigWatcher watcher;
    int callCount = 0;
    watcher.setOnChanged([&]() { callCount++; });
    watcher.watch(m_filePath, 100);

    watcher.stop();

    // Modify file and poll — should not fire
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(dw::file::writeText(m_filePath, "{\"changed\": true}"));
    watcher.poll(300);

    EXPECT_EQ(callCount, 0);
}

TEST_F(ConfigWatcherTest, NoCallbackWithoutChange) {
    dw::ConfigWatcher watcher;
    int callCount = 0;
    watcher.setOnChanged([&]() { callCount++; });
    watcher.watch(m_filePath, 100);

    // Poll multiple times without changing the file
    watcher.poll(200);
    watcher.poll(400);
    watcher.poll(600);

    EXPECT_EQ(callCount, 0);
}
