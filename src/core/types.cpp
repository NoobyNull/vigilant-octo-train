#include "types.h"

#include <cmath>

namespace dw {

f32 Vec3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vec3 Vec3::normalized() const {
    f32 len = length();
    if (len > 0.0f) {
        return *this / len;
    }
    return *this;
}

Mat4 Mat4::identity() {
    return Mat4{};
}

Mat4 Mat4::translate(const Vec3& v) {
    Mat4 m;
    m(0, 3) = v.x;
    m(1, 3) = v.y;
    m(2, 3) = v.z;
    return m;
}

Mat4 Mat4::scale(const Vec3& v) {
    Mat4 m;
    m(0, 0) = v.x;
    m(1, 1) = v.y;
    m(2, 2) = v.z;
    return m;
}

Mat4 Mat4::rotateX(f32 radians) {
    Mat4 m;
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);
    m(1, 1) = c;
    m(1, 2) = -s;
    m(2, 1) = s;
    m(2, 2) = c;
    return m;
}

Mat4 Mat4::rotateY(f32 radians) {
    Mat4 m;
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);
    m(0, 0) = c;
    m(0, 2) = s;
    m(2, 0) = -s;
    m(2, 2) = c;
    return m;
}

Mat4 Mat4::rotateZ(f32 radians) {
    Mat4 m;
    f32 c = std::cos(radians);
    f32 s = std::sin(radians);
    m(0, 0) = c;
    m(0, 1) = -s;
    m(1, 0) = s;
    m(1, 1) = c;
    return m;
}

Mat4 Mat4::perspective(f32 fovY, f32 aspect, f32 nearPlane, f32 farPlane) {
    Mat4 m;
    f32 tanHalfFov = std::tan(fovY / 2.0f);

    m(0, 0) = 1.0f / (aspect * tanHalfFov);
    m(1, 1) = 1.0f / tanHalfFov;
    m(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    m(3, 2) = -1.0f;
    m(3, 3) = 0.0f;

    return m;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalized();
    Vec3 s = f.cross(up).normalized();
    Vec3 u = s.cross(f);

    Mat4 m;
    m(0, 0) = s.x;
    m(0, 1) = s.y;
    m(0, 2) = s.z;
    m(1, 0) = u.x;
    m(1, 1) = u.y;
    m(1, 2) = u.z;
    m(2, 0) = -f.x;
    m(2, 1) = -f.y;
    m(2, 2) = -f.z;
    m(0, 3) = -s.dot(eye);
    m(1, 3) = -u.dot(eye);
    m(2, 3) = f.dot(eye);

    return m;
}

Mat4 Mat4::ortho(f32 left, f32 right, f32 bottom, f32 top, f32 nearPlane,
                 f32 farPlane) {
    Mat4 m;
    m(0, 0) = 2.0f / (right - left);
    m(1, 1) = 2.0f / (top - bottom);
    m(2, 2) = -2.0f / (farPlane - nearPlane);
    m(0, 3) = -(right + left) / (right - left);
    m(1, 3) = -(top + bottom) / (top - bottom);
    m(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    return m;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result(row, col) = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result(row, col) += (*this)(row, k) * other(k, col);
            }
        }
    }
    return result;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    return {data[0] * v.x + data[4] * v.y + data[8] * v.z + data[12] * v.w,
            data[1] * v.x + data[5] * v.y + data[9] * v.z + data[13] * v.w,
            data[2] * v.x + data[6] * v.y + data[10] * v.z + data[14] * v.w,
            data[3] * v.x + data[7] * v.y + data[11] * v.z + data[15] * v.w};
}

}  // namespace dw
