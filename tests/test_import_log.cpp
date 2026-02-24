// Digital Workshop - Import Log Tests

#include <filesystem>

#include <gtest/gtest.h>

#include "core/import/import_log.h"

namespace {

class ImportLogTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_logPath = std::filesystem::temp_directory_path() / "dw_test_import_log";
        std::filesystem::remove(m_logPath);
    }

    void TearDown() override { std::filesystem::remove(m_logPath); }

    std::filesystem::path m_logPath;
};

} // namespace

TEST_F(ImportLogTest, Empty_NoFile) {
    dw::ImportLog log(m_logPath);
    EXPECT_FALSE(log.exists());
    EXPECT_TRUE(log.buildSkipSet().empty());
    EXPECT_TRUE(log.readAll().empty());
}

TEST_F(ImportLogTest, AppendDone_CreatesFile) {
    dw::ImportLog log(m_logPath);
    log.appendDone("/home/user/model.stl", "abc123");
    EXPECT_TRUE(log.exists());
}

TEST_F(ImportLogTest, AppendDone_ReadBack) {
    dw::ImportLog log(m_logPath);
    log.appendDone("/home/user/model.stl", "abc123");

    auto records = log.readAll();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].status, "DONE");
    EXPECT_EQ(records[0].sourcePath, "/home/user/model.stl");
    EXPECT_EQ(records[0].hash, "abc123");
    EXPECT_FALSE(records[0].timestamp.empty());
}

TEST_F(ImportLogTest, AppendDup_ReadBack) {
    dw::ImportLog log(m_logPath);
    log.appendDup("/home/user/copy.stl", "def456");

    auto records = log.readAll();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].status, "DUP");
    EXPECT_EQ(records[0].sourcePath, "/home/user/copy.stl");
    EXPECT_EQ(records[0].hash, "def456");
}

TEST_F(ImportLogTest, MultipleEntries) {
    dw::ImportLog log(m_logPath);
    log.appendDone("/a/b.stl", "hash1");
    log.appendDup("/a/c.stl", "hash2");
    log.appendDone("/a/d.stl", "hash3");

    auto records = log.readAll();
    ASSERT_EQ(records.size(), 3u);
    EXPECT_EQ(records[0].status, "DONE");
    EXPECT_EQ(records[1].status, "DUP");
    EXPECT_EQ(records[2].status, "DONE");
}

TEST_F(ImportLogTest, BuildSkipSet) {
    dw::ImportLog log(m_logPath);
    log.appendDone("/a/b.stl", "hash1");
    log.appendDup("/a/c.stl", "hash2");
    log.appendDone("/a/d.stl", "hash3");

    auto skipSet = log.buildSkipSet();
    ASSERT_EQ(skipSet.size(), 3u);
    EXPECT_TRUE(skipSet.count("/a/b.stl"));
    EXPECT_TRUE(skipSet.count("/a/c.stl"));
    EXPECT_TRUE(skipSet.count("/a/d.stl"));
}

TEST_F(ImportLogTest, Remove) {
    dw::ImportLog log(m_logPath);
    log.appendDone("/a/b.stl", "hash1");
    EXPECT_TRUE(log.exists());
    log.remove();
    EXPECT_FALSE(log.exists());
}
