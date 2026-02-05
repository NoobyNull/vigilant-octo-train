// Digital Workshop - 3MF Loader Tests

#include <gtest/gtest.h>

#include "core/loaders/threemf_loader.h"

TEST(ThreeMFLoader, SupportsExtension) {
    dw::ThreeMFLoader loader;
    EXPECT_TRUE(loader.supports("3mf"));
    EXPECT_TRUE(loader.supports("3MF"));
    EXPECT_FALSE(loader.supports("stl"));
    EXPECT_FALSE(loader.supports(""));
}

TEST(ThreeMFLoader, Extensions_Contains3MF) {
    dw::ThreeMFLoader loader;
    auto exts = loader.extensions();
    EXPECT_FALSE(exts.empty());
    bool found = false;
    for (const auto& e : exts) {
        if (e == "3mf") found = true;
    }
    EXPECT_TRUE(found);
}

TEST(ThreeMFLoader, LoadFromBuffer_EmptyData) {
    dw::ThreeMFLoader loader;
    dw::ByteBuffer empty;
    auto result = loader.loadFromBuffer(empty);
    EXPECT_FALSE(result.success());
}

TEST(ThreeMFLoader, LoadFromBuffer_GarbageData) {
    dw::ThreeMFLoader loader;
    dw::ByteBuffer garbage = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02};
    auto result = loader.loadFromBuffer(garbage);
    EXPECT_FALSE(result.success());
}

TEST(ThreeMFLoader, LoadFromBuffer_TooSmall) {
    dw::ThreeMFLoader loader;
    dw::ByteBuffer tiny = {0x50, 0x4B};  // Just "PK" (ZIP magic, incomplete)
    auto result = loader.loadFromBuffer(tiny);
    EXPECT_FALSE(result.success());
}

TEST(ThreeMFLoader, Load_NonExistentFile) {
    dw::ThreeMFLoader loader;
    auto result = loader.load("/nonexistent/model.3mf");
    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.error.empty());
}
