#include "hash.h"

#include <cstring>
#include <iomanip>
#include <sstream>

#include "../utils/file_utils.h"
#include "mesh.h"

namespace dw {
namespace hash {

// FNV-1a constants
constexpr u64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr u64 FNV_PRIME = 1099511628211ULL;

u64 computeBytes(const void* data, usize size) {
    u64 hash = FNV_OFFSET_BASIS;
    const u8* bytes = static_cast<const u8*>(data);

    for (usize i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

std::string computeFile(const Path& path) {
    auto data = file::readBinary(path);
    if (!data) {
        return "";
    }

    u64 hash = computeBytes(data->data(), data->size());
    return toHex(hash);
}

std::string computeBuffer(const ByteBuffer& buffer) {
    if (buffer.empty()) {
        return "";
    }
    u64 h = computeBytes(buffer.data(), buffer.size());
    return toHex(h);
}

std::string computeMesh(const Mesh& mesh) {
    // Hash vertex data
    u64 hash = FNV_OFFSET_BASIS;

    // Include vertex count in hash
    u32 vertexCount = mesh.vertexCount();
    hash = computeBytes(&vertexCount, sizeof(vertexCount));

    // Hash vertex positions (ignore normals/texcoords for geometry identity)
    for (const auto& vertex : mesh.vertices()) {
        const f32* pos = &vertex.position.x;
        for (int i = 0; i < 3; ++i) {
            u32 bits = 0;
            std::memcpy(&bits, &pos[i], sizeof(bits));
            hash ^= bits;
            hash *= FNV_PRIME;
        }
    }

    // Hash indices
    for (u32 index : mesh.indices()) {
        hash ^= index;
        hash *= FNV_PRIME;
    }

    return toHex(hash);
}

std::string toHex(u64 hash) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

u64 fromHex(const std::string& hex) {
    u64 hash = 0;
    std::istringstream ss(hex);
    ss >> std::hex >> hash;
    return hash;
}

} // namespace hash
} // namespace dw
