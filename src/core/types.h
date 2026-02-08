#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace dw {

// Filesystem
namespace fs = std::filesystem;
using Path = fs::path;

// Integer types
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Floating point
using f32 = float;
using f64 = double;

// Size type
using usize = std::size_t;

// 3D math types (GLM aliases)
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

// Spherical coordinate conversions (azimuth/elevation in radians)
Vec2 toSpherical(const Vec3& dir);
Vec3 fromSpherical(f32 azimuth, f32 elevation);

// Color
struct Color {
    f32 r{1.0f};
    f32 g{1.0f};
    f32 b{1.0f};
    f32 a{1.0f};

    Color() = default;
    Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static Color fromRGB(u8 r_, u8 g_, u8 b_, u8 a_ = 255) {
        return {r_ / 255.0f, g_ / 255.0f, b_ / 255.0f, a_ / 255.0f};
    }

    static Color fromHex(u32 hex) {
        return fromRGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
    }
};

// Common result type
template <typename T> using Result = std::optional<T>;

// Byte buffer
using ByteBuffer = std::vector<u8>;

} // namespace dw
