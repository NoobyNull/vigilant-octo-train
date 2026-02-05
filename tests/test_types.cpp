// Digital Workshop - Types / Mat4 Math Tests

#include <gtest/gtest.h>

#include "core/types.h"

#include <cmath>

namespace {
constexpr float PI = 3.14159265359f;
constexpr float EPS = 1e-5f;
}  // namespace

// --- Vec3 ---

TEST(Vec3, DefaultZero) {
    dw::Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vec3, Add) {
    dw::Vec3 a(1, 2, 3);
    dw::Vec3 b(4, 5, 6);
    auto c = a + b;
    EXPECT_FLOAT_EQ(c.x, 5.0f);
    EXPECT_FLOAT_EQ(c.y, 7.0f);
    EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(Vec3, Subtract) {
    dw::Vec3 a(5, 5, 5);
    dw::Vec3 b(1, 2, 3);
    auto c = a - b;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 3.0f);
    EXPECT_FLOAT_EQ(c.z, 2.0f);
}

TEST(Vec3, ScalarMultiply) {
    dw::Vec3 v(1, 2, 3);
    auto r = v * 2.0f;
    EXPECT_FLOAT_EQ(r.x, 2.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
    EXPECT_FLOAT_EQ(r.z, 6.0f);
}

TEST(Vec3, Length) {
    dw::Vec3 v(3, 4, 0);
    EXPECT_NEAR(v.length(), 5.0f, EPS);
}

TEST(Vec3, Normalized) {
    dw::Vec3 v(0, 0, 5);
    auto n = v.normalized();
    EXPECT_NEAR(n.x, 0.0f, EPS);
    EXPECT_NEAR(n.y, 0.0f, EPS);
    EXPECT_NEAR(n.z, 1.0f, EPS);
}

TEST(Vec3, Normalized_ZeroVector) {
    dw::Vec3 v(0, 0, 0);
    auto n = v.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
    EXPECT_FLOAT_EQ(n.z, 0.0f);
}

TEST(Vec3, Dot) {
    dw::Vec3 a(1, 0, 0);
    dw::Vec3 b(0, 1, 0);
    EXPECT_NEAR(a.dot(b), 0.0f, EPS);

    dw::Vec3 c(1, 0, 0);
    EXPECT_NEAR(a.dot(c), 1.0f, EPS);
}

TEST(Vec3, Cross) {
    dw::Vec3 x(1, 0, 0);
    dw::Vec3 y(0, 1, 0);
    auto z = x.cross(y);
    EXPECT_NEAR(z.x, 0.0f, EPS);
    EXPECT_NEAR(z.y, 0.0f, EPS);
    EXPECT_NEAR(z.z, 1.0f, EPS);
}

// --- Mat4 Identity ---

TEST(Mat4, Identity) {
    auto m = dw::Mat4::identity();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float expected = (i == j) ? 1.0f : 0.0f;
            EXPECT_FLOAT_EQ(m(i, j), expected);
        }
    }
}

// --- Mat4 Translate ---

TEST(Mat4, Translate) {
    auto m = dw::Mat4::translate({10, 20, 30});
    // Apply to origin point
    dw::Vec4 p(0, 0, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 10.0f, EPS);
    EXPECT_NEAR(r.y, 20.0f, EPS);
    EXPECT_NEAR(r.z, 30.0f, EPS);
    EXPECT_NEAR(r.w, 1.0f, EPS);
}

TEST(Mat4, Translate_Direction_Unaffected) {
    auto m = dw::Mat4::translate({10, 20, 30});
    // Directions (w=0) should not be affected by translation
    dw::Vec4 d(1, 0, 0, 0);
    auto r = m * d;
    EXPECT_NEAR(r.x, 1.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, 0.0f, EPS);
}

// --- Mat4 Scale ---

TEST(Mat4, Scale) {
    auto m = dw::Mat4::scale({2, 3, 4});
    dw::Vec4 p(1, 1, 1, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 2.0f, EPS);
    EXPECT_NEAR(r.y, 3.0f, EPS);
    EXPECT_NEAR(r.z, 4.0f, EPS);
}

TEST(Mat4, Scale_Uniform) {
    auto m = dw::Mat4::scale({5, 5, 5});
    dw::Vec4 p(1, 2, 3, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 5.0f, EPS);
    EXPECT_NEAR(r.y, 10.0f, EPS);
    EXPECT_NEAR(r.z, 15.0f, EPS);
}

// --- Mat4 Rotate ---

TEST(Mat4, RotateZ_90Degrees) {
    auto m = dw::Mat4::rotateZ(PI / 2.0f);
    dw::Vec4 p(1, 0, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 0.0f, EPS);
    EXPECT_NEAR(r.y, 1.0f, EPS);
    EXPECT_NEAR(r.z, 0.0f, EPS);
}

TEST(Mat4, RotateX_90Degrees) {
    auto m = dw::Mat4::rotateX(PI / 2.0f);
    dw::Vec4 p(0, 1, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 0.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, 1.0f, EPS);
}

TEST(Mat4, RotateY_90Degrees) {
    auto m = dw::Mat4::rotateY(PI / 2.0f);
    dw::Vec4 p(1, 0, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 0.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, -1.0f, EPS);
}

TEST(Mat4, Rotate_360_Identity) {
    auto m = dw::Mat4::rotateZ(2.0f * PI);
    dw::Vec4 p(1, 0, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 1.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, 0.0f, EPS);
}

// --- Mat4 Multiply ---

TEST(Mat4, Multiply_Identity) {
    auto a = dw::Mat4::translate({1, 2, 3});
    auto id = dw::Mat4::identity();
    auto r = a * id;
    // Should equal a
    for (int i = 0; i < 16; ++i) {
        EXPECT_NEAR(r.data[i], a.data[i], EPS);
    }
}

TEST(Mat4, Multiply_TranslateScale) {
    // Scale then translate: point (1,1,1) → scale 2x → (2,2,2) → translate (10,0,0) → (12,2,2)
    auto t = dw::Mat4::translate({10, 0, 0});
    auto s = dw::Mat4::scale({2, 2, 2});
    auto m = t * s;
    dw::Vec4 p(1, 1, 1, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 12.0f, EPS);
    EXPECT_NEAR(r.y, 2.0f, EPS);
    EXPECT_NEAR(r.z, 2.0f, EPS);
}

// --- Mat4 Perspective ---

TEST(Mat4, Perspective_NonZero) {
    auto m = dw::Mat4::perspective(PI / 4.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    // Key properties: (0,0) > 0, (1,1) > 0, (3,2) = -1
    EXPECT_GT(m(0, 0), 0.0f);
    EXPECT_GT(m(1, 1), 0.0f);
    EXPECT_FLOAT_EQ(m(3, 2), -1.0f);
    EXPECT_FLOAT_EQ(m(3, 3), 0.0f);
}

// --- Mat4 LookAt ---

TEST(Mat4, LookAt_ForwardIsNegZ) {
    // Camera at origin looking down -Z
    auto m = dw::Mat4::lookAt({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
    // The view matrix should be close to identity for this case
    // A point at (0,0,-1) in world should map to (0,0,-1) in view
    dw::Vec4 p(0, 0, -1, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 0.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, -1.0f, EPS);
}

// --- Mat4 Ortho ---

TEST(Mat4, Ortho_CenterMapsToOrigin) {
    auto m = dw::Mat4::ortho(-10, 10, -10, 10, -1, 1);
    // Center of ortho box (0,0,0) should map to (0,0,0) in NDC
    dw::Vec4 p(0, 0, 0, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 0.0f, EPS);
    EXPECT_NEAR(r.y, 0.0f, EPS);
    EXPECT_NEAR(r.z, 0.0f, EPS);
}

TEST(Mat4, Ortho_CornerMapsToNDCCorner) {
    auto m = dw::Mat4::ortho(-10, 10, -10, 10, -1, 1);
    // Right edge maps to x=1
    dw::Vec4 p(10, 10, -1, 1);
    auto r = m * p;
    EXPECT_NEAR(r.x, 1.0f, EPS);
    EXPECT_NEAR(r.y, 1.0f, EPS);
}

// --- Color ---

TEST(Color, FromRGB) {
    auto c = dw::Color::fromRGB(255, 0, 128);
    EXPECT_NEAR(c.r, 1.0f, 0.01f);
    EXPECT_NEAR(c.g, 0.0f, 0.01f);
    EXPECT_NEAR(c.b, 0.502f, 0.01f);
    EXPECT_NEAR(c.a, 1.0f, 0.01f);
}

TEST(Color, FromHex) {
    auto c = dw::Color::fromHex(0xFF0000);
    EXPECT_NEAR(c.r, 1.0f, 0.01f);
    EXPECT_NEAR(c.g, 0.0f, 0.01f);
    EXPECT_NEAR(c.b, 0.0f, 0.01f);
}
