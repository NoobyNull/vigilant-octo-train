#pragma once

#include <random>
#include <string>

namespace dw::uuid {

// Generate a random UUID v4 in canonical format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
inline std::string generate() {
    thread_local std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dis;

    // Generate 16 random bytes
    uint8_t bytes[16];
    uint64_t val1 = dis(gen);
    uint64_t val2 = dis(gen);

    // Copy to bytes array (little-endian on most systems)
    std::memcpy(bytes, &val1, 8);
    std::memcpy(bytes + 8, &val2, 8);

    // Set version 4 (bits 12-15 of time_hi_and_version)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;

    // Set variant 1 (bits 6-7 of clock_seq_hi_and_reserved)
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    // Format as canonical string: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    char buffer[37];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9],
        bytes[10],
        bytes[11],
        bytes[12],
        bytes[13],
        bytes[14],
        bytes[15]);

    return std::string(buffer);
}

} // namespace dw::uuid
