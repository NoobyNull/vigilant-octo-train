#pragma once

#include "../types.h"

namespace dw {

// Vertex with position, normal, and texture coordinates
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;

    Vertex() = default;
    Vertex(const Vec3& pos) : position(pos) {}
    Vertex(const Vec3& pos, const Vec3& norm) : position(pos), normal(norm) {}
    Vertex(const Vec3& pos, const Vec3& norm, const Vec2& tex)
        : position(pos), normal(norm), texCoord(tex) {}

    bool operator==(const Vertex& other) const {
        return position.x == other.position.x && position.y == other.position.y &&
               position.z == other.position.z && normal.x == other.normal.x &&
               normal.y == other.normal.y && normal.z == other.normal.z &&
               texCoord.x == other.texCoord.x && texCoord.y == other.texCoord.y;
    }
};

// Triangle face (indices into vertex array)
struct Triangle {
    u32 v0 = 0;
    u32 v1 = 0;
    u32 v2 = 0;

    Triangle() = default;
    Triangle(u32 a, u32 b, u32 c) : v0(a), v1(b), v2(c) {}
};

}  // namespace dw

// Hash function for Vertex (for use in unordered_map)
namespace std {
template <>
struct hash<dw::Vertex> {
    size_t operator()(const dw::Vertex& v) const {
        // Simple hash combining position components
        size_t h = 0;
        auto hashFloat = [](float f) {
            return std::hash<float>{}(f);
        };
        h ^= hashFloat(v.position.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.position.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.position.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.normal.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.normal.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.normal.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.texCoord.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= hashFloat(v.texCoord.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
}  // namespace std
