// Digital Workshop - Filesystem Detector Tests

#include <gtest/gtest.h>

#include "core/import/filesystem_detector.h"

using namespace dw;

TEST(FilesystemDetector, RootIsLocal) {
    auto info = detectFilesystem(Path("/"));
    EXPECT_EQ(info.location, StorageLocation::Local);
    EXPECT_FALSE(info.fsTypeName.empty());
}

TEST(FilesystemDetector, TmpIsLocal) {
    auto info = detectFilesystem(Path("/tmp"));
    EXPECT_EQ(info.location, StorageLocation::Local);
}

TEST(FilesystemDetector, NonexistentPathDoesNotCrash) {
    auto info = detectFilesystem(Path("/nonexistent/path/that/doesnt/exist"));
    // Should walk up to "/" which is local
    EXPECT_NE(info.location, StorageLocation::Unknown);
}

TEST(FilesystemDetector, EmptyPathReturnsUnknown) {
    auto info = detectFilesystem(Path(""));
    EXPECT_EQ(info.location, StorageLocation::Unknown);
}

TEST(FilesystemDetector, HomeDirIsLocal) {
    // Home directory should always be on a local filesystem
    const char* home = std::getenv("HOME");
    if (home) {
        auto info = detectFilesystem(Path(home));
        EXPECT_EQ(info.location, StorageLocation::Local);
    }
}
