#include <gtest/gtest.h>

#include <chrono>
#include <fstream>

#include "core/mesh/hash.h"
#include "core/storage/storage_manager.h"

using namespace dw;

class StorageManagerTest : public ::testing::Test {
  protected:
    Path testRoot;
    std::unique_ptr<StorageManager> mgr;

    void SetUp() override {
        // Create a unique temp directory for each test
        auto now = std::chrono::steady_clock::now()
                       .time_since_epoch()
                       .count();
        testRoot = fs::temp_directory_path() /
                   ("test_cas_" + std::to_string(now));
        fs::create_directories(testRoot);
        mgr = std::make_unique<StorageManager>(testRoot);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(testRoot, ec);
    }

    // Helper: create a test file with given content, return its path
    Path createTestFile(const std::string& name,
                        const std::string& content) {
        Path p = testRoot / name;
        std::ofstream out(p, std::ios::binary);
        out << content;
        out.close();
        return p;
    }
};

TEST_F(StorageManagerTest, BlobPathComputation) {
    Path p = mgr->blobPath("abcdef1234567890", "stl");
    ASSERT_FALSE(p.empty());
    // Should end with ab/cd/abcdef1234567890.stl
    std::string ps = p.string();
    EXPECT_NE(ps.find("ab/cd/abcdef1234567890.stl"), std::string::npos)
        << "Path was: " << ps;

    // Different hash gives different prefix dirs
    Path p2 = mgr->blobPath("1234567890abcdef", "obj");
    std::string ps2 = p2.string();
    EXPECT_NE(ps2.find("12/34/1234567890abcdef.obj"), std::string::npos)
        << "Path was: " << ps2;
}

TEST_F(StorageManagerTest, BlobPathShortHash) {
    Path p = mgr->blobPath("abc", "stl");
    EXPECT_TRUE(p.empty()) << "Hash < 4 chars should return empty path";
}

TEST_F(StorageManagerTest, StoreFileBasic) {
    // Create a test file
    Path source = createTestFile("test_input.stl", "hello blob store");
    std::string fileHash = hash::computeFile(source);
    ASSERT_FALSE(fileHash.empty());

    // Store it
    std::string error;
    Path result = mgr->storeFile(source, fileHash, "stl", error);
    ASSERT_FALSE(result.empty()) << "storeFile failed: " << error;
    EXPECT_TRUE(error.empty());

    // Verify it matches expected blob path
    EXPECT_EQ(result, mgr->blobPath(fileHash, "stl"));

    // Verify file exists at returned path
    EXPECT_TRUE(fs::exists(result));

    // Verify temp dir is empty (temp file was renamed)
    Path tmpDir = testRoot / ".tmp";
    if (fs::exists(tmpDir)) {
        int tmpCount = 0;
        for ([[maybe_unused]] const auto& e :
             fs::directory_iterator(tmpDir)) {
            ++tmpCount;
        }
        EXPECT_EQ(tmpCount, 0) << "Temp dir should be empty after store";
    }
}

TEST_F(StorageManagerTest, StoreFileDedup) {
    Path source = createTestFile("test_dedup.stl", "dedup content");
    std::string fileHash = hash::computeFile(source);

    std::string error1, error2;
    Path result1 = mgr->storeFile(source, fileHash, "stl", error1);
    Path result2 = mgr->storeFile(source, fileHash, "stl", error2);

    ASSERT_FALSE(result1.empty()) << "First store failed: " << error1;
    ASSERT_FALSE(result2.empty()) << "Second store failed: " << error2;
    EXPECT_EQ(result1, result2);
    EXPECT_TRUE(error1.empty());
    EXPECT_TRUE(error2.empty());
    EXPECT_TRUE(fs::exists(result1));
}

TEST_F(StorageManagerTest, StoreFileHashMismatch) {
    Path source = createTestFile("test_mismatch.stl", "mismatch content");
    std::string wrongHash = "deadbeef12345678";

    std::string error;
    Path result = mgr->storeFile(source, wrongHash, "stl", error);

    EXPECT_TRUE(result.empty()) << "Should fail with wrong hash";
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("Hash verification failed"), std::string::npos)
        << "Error was: " << error;

    // No file at wrong hash path
    EXPECT_FALSE(fs::exists(mgr->blobPath(wrongHash, "stl")));

    // Temp file should be cleaned up
    Path tmpDir = testRoot / ".tmp";
    if (fs::exists(tmpDir)) {
        int tmpCount = 0;
        for ([[maybe_unused]] const auto& e :
             fs::directory_iterator(tmpDir)) {
            ++tmpCount;
        }
        EXPECT_EQ(tmpCount, 0) << "Temp file should be cleaned up";
    }
}

TEST_F(StorageManagerTest, MoveFileBasic) {
    Path source = createTestFile("test_move.stl", "move content");
    std::string fileHash = hash::computeFile(source);

    std::string error;
    Path result = mgr->moveFile(source, fileHash, "stl", error);

    ASSERT_FALSE(result.empty()) << "moveFile failed: " << error;
    EXPECT_TRUE(fs::exists(result));
    EXPECT_FALSE(fs::exists(source)) << "Source should be removed after move";
}

TEST_F(StorageManagerTest, ExistsCheck) {
    std::string fileHash = "abcdef1234567890";
    EXPECT_FALSE(mgr->exists(fileHash, "stl"))
        << "Should not exist before store";

    Path source = createTestFile("test_exists.stl", "exists content");
    std::string realHash = hash::computeFile(source);

    std::string error;
    mgr->storeFile(source, realHash, "stl", error);
    EXPECT_TRUE(mgr->exists(realHash, "stl"))
        << "Should exist after store";
}

TEST_F(StorageManagerTest, RemoveBlob) {
    Path source = createTestFile("test_remove.stl", "remove content");
    std::string fileHash = hash::computeFile(source);

    std::string error;
    mgr->storeFile(source, fileHash, "stl", error);
    ASSERT_TRUE(mgr->exists(fileHash, "stl"));

    bool removed = mgr->remove(fileHash, "stl");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(mgr->exists(fileHash, "stl"));
}

TEST_F(StorageManagerTest, CleanupOrphanedTempFiles) {
    // Manually create temp files simulating crash leftovers
    Path tmpDir = testRoot / ".tmp";
    fs::create_directories(tmpDir);

    std::ofstream(tmpDir / "import_dead1.stl") << "orphan1";
    std::ofstream(tmpDir / "import_dead2.obj") << "orphan2";
    std::ofstream(tmpDir / "import_dead3.3mf") << "orphan3";

    int count = mgr->cleanupOrphanedTempFiles();
    EXPECT_EQ(count, 3);

    // Verify temp dir is empty
    int remaining = 0;
    for ([[maybe_unused]] const auto& e : fs::directory_iterator(tmpDir)) {
        ++remaining;
    }
    EXPECT_EQ(remaining, 0);
}
