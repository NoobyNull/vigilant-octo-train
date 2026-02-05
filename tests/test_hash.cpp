// Digital Workshop - Hash Tests

#include <gtest/gtest.h>

#include "core/mesh/hash.h"
#include "core/types.h"

TEST(Hash, ComputeBuffer_ConsistentResults) {
    dw::ByteBuffer data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"

    std::string hash1 = dw::hash::computeBuffer(data);
    std::string hash2 = dw::hash::computeBuffer(data);

    EXPECT_FALSE(hash1.empty());
    EXPECT_EQ(hash1, hash2);
}

TEST(Hash, ComputeBuffer_DifferentDataDifferentHash) {
    dw::ByteBuffer data1 = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    dw::ByteBuffer data2 = {0x57, 0x6F, 0x72, 0x6C, 0x64};  // "World"

    std::string hash1 = dw::hash::computeBuffer(data1);
    std::string hash2 = dw::hash::computeBuffer(data2);

    EXPECT_FALSE(hash1.empty());
    EXPECT_FALSE(hash2.empty());
    EXPECT_NE(hash1, hash2);
}

TEST(Hash, ComputeBuffer_EmptyBufferReturnsEmpty) {
    dw::ByteBuffer empty;

    std::string hash = dw::hash::computeBuffer(empty);

    EXPECT_TRUE(hash.empty());
}

TEST(Hash, ToHexFromHex_Roundtrip) {
    dw::u64 original = 0xDEADBEEFCAFE1234ULL;

    std::string hex = dw::hash::toHex(original);
    dw::u64 recovered = dw::hash::fromHex(hex);

    EXPECT_EQ(original, recovered);
}

TEST(Hash, ToHex_Format) {
    // toHex should produce a 16-character lowercase hex string
    dw::u64 value = 0x0000000000000001ULL;

    std::string hex = dw::hash::toHex(value);

    EXPECT_EQ(hex.size(), 16u);
    EXPECT_EQ(hex, "0000000000000001");
}

TEST(Hash, ToHex_Zero) {
    std::string hex = dw::hash::toHex(0);

    EXPECT_EQ(hex.size(), 16u);
    EXPECT_EQ(hex, "0000000000000000");
}

TEST(Hash, FromHex_Zero) {
    dw::u64 value = dw::hash::fromHex("0000000000000000");

    EXPECT_EQ(value, 0u);
}

TEST(Hash, ComputeBytes_Deterministic) {
    const char data[] = "test data";
    dw::u64 hash1 = dw::hash::computeBytes(data, sizeof(data) - 1);
    dw::u64 hash2 = dw::hash::computeBytes(data, sizeof(data) - 1);

    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, 0u);
}

TEST(Hash, ComputeBytes_DifferentDataDifferentHash) {
    const char data1[] = "abc";
    const char data2[] = "xyz";
    dw::u64 h1 = dw::hash::computeBytes(data1, 3);
    dw::u64 h2 = dw::hash::computeBytes(data2, 3);

    EXPECT_NE(h1, h2);
}
