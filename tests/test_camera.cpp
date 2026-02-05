// Digital Workshop - Camera Tests (no GL required)

#include <gtest/gtest.h>

#include "render/camera.h"

#include <cmath>

namespace {
constexpr float EPS = 1e-4f;
}

// --- Default state ---

TEST(Camera, DefaultState) {
    dw::Camera cam;
    EXPECT_FLOAT_EQ(cam.distance(), 5.0f);
    EXPECT_FLOAT_EQ(cam.pitch(), 30.0f);
    EXPECT_FLOAT_EQ(cam.yaw(), 45.0f);
    EXPECT_FLOAT_EQ(cam.fov(), 45.0f);
}

TEST(Camera, DefaultTarget_Origin) {
    dw::Camera cam;
    EXPECT_FLOAT_EQ(cam.target().x, 0.0f);
    EXPECT_FLOAT_EQ(cam.target().y, 0.0f);
    EXPECT_FLOAT_EQ(cam.target().z, 0.0f);
}

// --- Position ---

TEST(Camera, Position_NonZero) {
    dw::Camera cam;
    auto pos = cam.position();
    // Position should be offset from target by distance
    float dist = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    EXPECT_NEAR(dist, 5.0f, 0.1f);
}

TEST(Camera, Position_ChangesWithDistance) {
    dw::Camera cam;
    cam.setDistance(10.0f);
    auto pos = cam.position();
    float dist = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    EXPECT_NEAR(dist, 10.0f, 0.1f);
}

// --- Orbit ---

TEST(Camera, Orbit_ChangesYaw) {
    dw::Camera cam;
    float origYaw = cam.yaw();
    cam.orbit(10.0f, 0.0f);
    EXPECT_NE(cam.yaw(), origYaw);
}

TEST(Camera, Orbit_ChangesPitch) {
    dw::Camera cam;
    float origPitch = cam.pitch();
    cam.orbit(0.0f, 10.0f);
    EXPECT_NE(cam.pitch(), origPitch);
}

TEST(Camera, Orbit_PitchClamped) {
    dw::Camera cam;
    // Push pitch way up — should clamp to max (89)
    cam.orbit(0.0f, 10000.0f);
    EXPECT_LE(cam.pitch(), 89.0f);

    // Push pitch way down — should clamp to min (-89)
    cam.orbit(0.0f, -20000.0f);
    EXPECT_GE(cam.pitch(), -89.0f);
}

TEST(Camera, Orbit_YawWraps) {
    dw::Camera cam;
    // Orbit a full circle
    cam.orbit(720.0f / 0.5f, 0.0f);  // 720 degrees at 0.5 sensitivity
    // Yaw should wrap to [0, 360)
    EXPECT_GE(cam.yaw(), 0.0f);
    EXPECT_LT(cam.yaw(), 360.0f);
}

// --- Zoom ---

TEST(Camera, Zoom_ReducesDistance) {
    dw::Camera cam;
    float origDist = cam.distance();
    cam.zoom(1.0f);  // Zoom in
    EXPECT_LT(cam.distance(), origDist);
}

TEST(Camera, Zoom_IncreasesDistance) {
    dw::Camera cam;
    float origDist = cam.distance();
    cam.zoom(-1.0f);  // Zoom out
    EXPECT_GT(cam.distance(), origDist);
}

TEST(Camera, Zoom_ClampsMinDistance) {
    dw::Camera cam;
    // Zoom in a huge amount
    for (int i = 0; i < 100; ++i) cam.zoom(10.0f);
    EXPECT_GE(cam.distance(), 0.1f);
}

TEST(Camera, Zoom_ClampsMaxDistance) {
    dw::Camera cam;
    // Zoom out a huge amount
    for (int i = 0; i < 100; ++i) cam.zoom(-10.0f);
    EXPECT_LE(cam.distance(), 10000.0f);
}

// --- Pan ---

TEST(Camera, Pan_MovesTarget) {
    dw::Camera cam;
    auto origTarget = cam.target();
    cam.pan(100.0f, 0.0f);
    auto newTarget = cam.target();
    // Target should have moved
    float dx = newTarget.x - origTarget.x;
    float dy = newTarget.y - origTarget.y;
    float dz = newTarget.z - origTarget.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    EXPECT_GT(dist, 0.0f);
}

// --- FitToBounds ---

TEST(Camera, FitToBounds_CentersOnBounds) {
    dw::Camera cam;
    cam.fitToBounds(dw::Vec3(10, 20, 30), dw::Vec3(20, 40, 60));

    // Target should be center of bounds
    EXPECT_NEAR(cam.target().x, 15.0f, EPS);
    EXPECT_NEAR(cam.target().y, 30.0f, EPS);
    EXPECT_NEAR(cam.target().z, 45.0f, EPS);
}

TEST(Camera, FitToBounds_DistanceProportional) {
    dw::Camera cam;
    cam.fitToBounds(dw::Vec3(0, 0, 0), dw::Vec3(100, 100, 100));
    // Distance should be maxExtent * 2 = 200
    EXPECT_NEAR(cam.distance(), 200.0f, EPS);
}

// --- Reset ---

TEST(Camera, Reset_RestoresDefaults) {
    dw::Camera cam;
    cam.orbit(100, 50);
    cam.zoom(5);
    cam.pan(10, 10);

    cam.reset();
    EXPECT_FLOAT_EQ(cam.distance(), 5.0f);
    EXPECT_FLOAT_EQ(cam.pitch(), 30.0f);
    EXPECT_FLOAT_EQ(cam.yaw(), 45.0f);
    EXPECT_FLOAT_EQ(cam.target().x, 0.0f);
}

// --- Viewport ---

TEST(Camera, Viewport_AspectRatio) {
    dw::Camera cam;
    cam.setViewport(1920, 1080);
    EXPECT_NEAR(cam.aspectRatio(), 1920.0f / 1080.0f, 0.001f);
}

TEST(Camera, Viewport_MinClamped) {
    dw::Camera cam;
    cam.setViewport(0, 0);
    // Should clamp to 1x1 to avoid division by zero
    EXPECT_GE(cam.viewportWidth(), 1);
    EXPECT_GE(cam.viewportHeight(), 1);
}

// --- Matrices ---

TEST(Camera, ViewMatrix_NotIdentity) {
    dw::Camera cam;
    auto view = cam.viewMatrix();
    // View matrix should not be identity (camera is offset)
    bool allIdentity = true;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float expected = (i == j) ? 1.0f : 0.0f;
            if (std::abs(view(i, j) - expected) > 0.01f) {
                allIdentity = false;
            }
        }
    }
    EXPECT_FALSE(allIdentity);
}

TEST(Camera, ProjectionMatrix_ValidPerspective) {
    dw::Camera cam;
    cam.setViewport(800, 600);
    auto proj = cam.projectionMatrix();
    // Perspective matrix has (3,2) = -1, (3,3) = 0
    EXPECT_FLOAT_EQ(proj(3, 2), -1.0f);
    EXPECT_FLOAT_EQ(proj(3, 3), 0.0f);
}

// --- Setters ---

TEST(Camera, SetDistance_Clamps) {
    dw::Camera cam;
    cam.setDistance(-5.0f);
    EXPECT_GE(cam.distance(), 0.1f);

    cam.setDistance(999999.0f);
    EXPECT_LE(cam.distance(), 10000.0f);
}

TEST(Camera, SetClipPlanes) {
    dw::Camera cam;
    cam.setClipPlanes(1.0f, 500.0f);
    EXPECT_FLOAT_EQ(cam.nearPlane(), 1.0f);
    EXPECT_FLOAT_EQ(cam.farPlane(), 500.0f);
}
