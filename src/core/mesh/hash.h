#pragma once

#include "../types.h"

#include <string>

namespace dw {

// Forward declaration
class Mesh;

// Simple hash functions for content deduplication
namespace hash {

// Compute hash of raw bytes (using FNV-1a for simplicity)
u64 computeBytes(const void* data, usize size);

// Compute hash of a file
std::string computeFile(const Path& path);

// Compute hash of a byte buffer (returns hex string)
std::string computeBuffer(const ByteBuffer& buffer);

// Compute hash of mesh geometry
std::string computeMesh(const Mesh& mesh);

// Convert hash to hex string
std::string toHex(u64 hash);

// Convert hex string to hash
u64 fromHex(const std::string& hex);

}  // namespace hash
}  // namespace dw
