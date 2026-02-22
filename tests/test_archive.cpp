// Digital Workshop - Archive Tests

#include <gtest/gtest.h>

#include "core/archive/archive.h"
#include "core/utils/file_utils.h"

#include <filesystem>

namespace {

class ArchiveTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_baseDir = std::filesystem::temp_directory_path() / "dw_test_archive";
        m_srcDir = (m_baseDir / "source").string();
        m_outDir = (m_baseDir / "output").string();
        m_archivePath = (m_baseDir / "test.dwp").string();

        std::filesystem::create_directories(m_srcDir);
        std::filesystem::create_directories(m_outDir);
    }

    void TearDown() override { std::filesystem::remove_all(m_baseDir); }

    void createTestFile(const std::string& relativePath, const std::string& content) {
        auto path = m_srcDir + "/" + relativePath;
        auto parent = dw::file::getParent(path);
        (void)dw::file::createDirectories(parent);
        ASSERT_TRUE(dw::file::writeText(path, content));
    }

    std::filesystem::path m_baseDir;
    std::string m_srcDir;
    std::string m_outDir;
    std::string m_archivePath;
};

} // namespace

// --- Create + Extract roundtrip ---

TEST_F(ArchiveTest, CreateAndExtract_SingleFile) {
    createTestFile("hello.txt", "Hello World");

    auto createResult = dw::ProjectArchive::create(m_archivePath, m_srcDir);
    ASSERT_TRUE(createResult.success) << createResult.error;
    EXPECT_EQ(createResult.files.size(), 1u);

    auto extractResult = dw::ProjectArchive::extract(m_archivePath, m_outDir);
    ASSERT_TRUE(extractResult.success) << extractResult.error;
    EXPECT_EQ(extractResult.files.size(), 1u);

    // Verify content
    auto content = dw::file::readText(m_outDir + "/hello.txt");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, "Hello World");
}

TEST_F(ArchiveTest, CreateAndExtract_MultipleFiles) {
    createTestFile("a.txt", "Alpha");
    createTestFile("b.txt", "Bravo");
    createTestFile("sub/c.txt", "Charlie");

    auto createResult = dw::ProjectArchive::create(m_archivePath, m_srcDir);
    ASSERT_TRUE(createResult.success) << createResult.error;
    EXPECT_EQ(createResult.files.size(), 3u);

    auto extractResult = dw::ProjectArchive::extract(m_archivePath, m_outDir);
    ASSERT_TRUE(extractResult.success) << extractResult.error;
    EXPECT_EQ(extractResult.files.size(), 3u);

    auto a = dw::file::readText(m_outDir + "/a.txt");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, "Alpha");

    auto c = dw::file::readText(m_outDir + "/sub/c.txt");
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(*c, "Charlie");
}

TEST_F(ArchiveTest, CreateAndExtract_BinaryData) {
    // Write binary content
    dw::ByteBuffer bin = {0x00, 0xFF, 0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_TRUE(dw::file::writeBinary(m_srcDir + "/data.bin", bin));

    auto createResult = dw::ProjectArchive::create(m_archivePath, m_srcDir);
    ASSERT_TRUE(createResult.success) << createResult.error;

    auto extractResult = dw::ProjectArchive::extract(m_archivePath, m_outDir);
    ASSERT_TRUE(extractResult.success) << extractResult.error;

    auto content = dw::file::readBinary(m_outDir + "/data.bin");
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, bin);
}

// --- List ---

TEST_F(ArchiveTest, List_ReturnsEntries) {
    createTestFile("one.txt", "1");
    createTestFile("two.txt", "22");

    dw::ProjectArchive::create(m_archivePath, m_srcDir);

    auto entries = dw::ProjectArchive::list(m_archivePath);
    EXPECT_EQ(entries.size(), 2u);

    for (const auto& entry : entries) {
        EXPECT_FALSE(entry.path.empty());
        EXPECT_GT(entry.uncompressedSize, 0u);
    }
}

TEST_F(ArchiveTest, List_NonExistentFile) {
    auto entries = dw::ProjectArchive::list("/nonexistent/archive.dwp");
    EXPECT_TRUE(entries.empty());
}

// --- isValidArchive ---

TEST_F(ArchiveTest, IsValidArchive_True) {
    createTestFile("file.txt", "content");
    dw::ProjectArchive::create(m_archivePath, m_srcDir);

    EXPECT_TRUE(dw::ProjectArchive::isValidArchive(m_archivePath));
}

TEST_F(ArchiveTest, IsValidArchive_RandomFile) {
    ASSERT_TRUE(dw::file::writeText(m_archivePath, "not an archive"));
    EXPECT_FALSE(dw::ProjectArchive::isValidArchive(m_archivePath));
}

TEST_F(ArchiveTest, IsValidArchive_NonExistent) {
    EXPECT_FALSE(dw::ProjectArchive::isValidArchive("/nonexistent.dwp"));
}

// --- Error cases ---

TEST_F(ArchiveTest, Create_NonExistentDir) {
    auto result = dw::ProjectArchive::create(m_archivePath, "/nonexistent/dir");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST_F(ArchiveTest, Create_EmptyDir) {
    // Source dir exists but is empty
    auto result = dw::ProjectArchive::create(m_archivePath, m_srcDir);
    EXPECT_FALSE(result.success);
}

TEST_F(ArchiveTest, Extract_InvalidArchive) {
    ASSERT_TRUE(dw::file::writeText(m_archivePath, "garbage data"));
    auto result = dw::ProjectArchive::extract(m_archivePath, m_outDir);
    EXPECT_FALSE(result.success);
}

TEST_F(ArchiveTest, Extract_NonExistentArchive) {
    auto result = dw::ProjectArchive::extract("/nonexistent.dwp", m_outDir);
    EXPECT_FALSE(result.success);
}
