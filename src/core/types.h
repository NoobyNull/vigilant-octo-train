#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

// Basic 3D math types (simple structs, no GLM dependency yet)
struct Vec2 {
    f32 x{0.0f};
    f32 y{0.0f};

    Vec2() = default;
    Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(f32 scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(f32 scalar) const { return {x / scalar, y / scalar}; }
};

struct Vec3 {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};

    Vec3() = default;
    Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    Vec3 operator*(f32 scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vec3 operator/(f32 scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    f32 dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    Vec3 cross(const Vec3& other) const {
        return {y * other.z - z * other.y, z * other.x - x * other.z,
                x * other.y - y * other.x};
    }
    f32 length() const;
    Vec3 normalized() const;
};

struct Vec4 {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};
    f32 w{0.0f};

    Vec4() = default;
    Vec4(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(const Vec3& v, f32 w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
};

// 4x4 matrix (column-major like OpenGL)
struct Mat4 {
    f32 data[16]{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    Mat4() = default;

    f32& operator()(int row, int col) { return data[col * 4 + row]; }
    f32 operator()(int row, int col) const { return data[col * 4 + row]; }

    static Mat4 identity();
    static Mat4 translate(const Vec3& v);
    static Mat4 scale(const Vec3& v);
    static Mat4 rotateX(f32 radians);
    static Mat4 rotateY(f32 radians);
    static Mat4 rotateZ(f32 radians);
    static Mat4 perspective(f32 fovY, f32 aspect, f32 nearPlane, f32 farPlane);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 ortho(f32 left, f32 right, f32 bottom, f32 top, f32 nearPlane,
                      f32 farPlane);

    Mat4 operator*(const Mat4& other) const;
    Vec4 operator*(const Vec4& v) const;

    const f32* ptr() const { return data; }
};

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
template <typename T>
using Result = std::optional<T>;

// Byte buffer
using ByteBuffer = std::vector<u8>;

}  // namespace dw
