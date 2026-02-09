#include "camera.h"

#include <algorithm>
#include <cmath>

namespace dw {

namespace {
constexpr f32 PI = 3.14159265359f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
} // namespace

Camera::Camera() {
    updateVectors();
}

Mat4 Camera::viewMatrix() const {
    Vec3 pos = position();
    return glm::lookAt(pos, m_target, Vec3{0.0f, 1.0f, 0.0f});
}

Mat4 Camera::projectionMatrix() const {
    return glm::perspective(m_fov * DEG_TO_RAD, aspectRatio(), m_nearPlane, m_farPlane);
}

Mat4 Camera::viewProjectionMatrix() const {
    return projectionMatrix() * viewMatrix();
}

void Camera::orbit(f32 deltaX, f32 deltaY) {
    // For screen-space rotation (dragging always rotates in the direction of mouse movement),
    // we need to rotate around view-relative axes, not fixed world axes.
    //
    // Standard orbit camera issue: rotation around world Y-axis creates opposite visual
    // effects when viewing from front vs back of the object.
    //
    // Solution: rotate around camera's local right vector for vertical (pitch) motion,
    // and around world up (Y) for horizontal (yaw) motion, but with sign adjustment
    // based on camera orientation.
    //
    // For yaw: when camera is on the "back side" of the object (yaw between 90° and 270°),
    // horizontal mouse movement should be inverted to maintain screen-space consistency.
    // Actually, simpler: use camera's up vector instead of world up.
    //
    // Even simpler: just apply the rotation with proper axis orientation.
    // The key insight is that for orbit camera, we want turntable-style visual feedback:
    // drag right = model visually rotates right, regardless of view angle.
    //
    // Implementation: Since we're constrained to yaw/pitch representation, we adjust
    // the delta signs based on viewing angle.

    // For true screen-space behavior, yaw change should flip when viewing from back.
    // Simple test: if yaw is in range (90, 270), we're viewing from the "back hemisphere"
    // and should invert horizontal rotation.

    // Normalize yaw to [0, 360) first
    f32 normalizedYaw = std::fmod(m_yaw, 360.0f);
    if (normalizedYaw < 0.0f)
        normalizedYaw += 360.0f;

    // If viewing from back hemisphere (yaw between 90 and 270), invert horizontal delta
    f32 yawDelta = deltaX;
    if (normalizedYaw > 90.0f && normalizedYaw < 270.0f) {
        yawDelta = -deltaX;
    }

    m_yaw += yawDelta * m_orbitSensitivity;
    m_pitch += deltaY * m_orbitSensitivity;

    // Clamp pitch
    m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);

    // Wrap yaw to [0, 360)
    m_yaw = std::fmod(m_yaw, 360.0f);
    if (m_yaw < 0.0f)
        m_yaw += 360.0f;

    updateVectors();
}

void Camera::pan(f32 deltaX, f32 deltaY) {
    f32 yawRad = m_yaw * DEG_TO_RAD;
    f32 pitchRad = m_pitch * DEG_TO_RAD;

    // Calculate right and up vectors in world space
    Vec3 forward{std::sin(yawRad) * std::cos(pitchRad), std::sin(pitchRad),
                 std::cos(yawRad) * std::cos(pitchRad)};
    Vec3 right = glm::normalize(glm::cross(Vec3{0.0f, 1.0f, 0.0f}, forward));
    Vec3 up = glm::normalize(glm::cross(forward, right));

    f32 panScale = m_distance * m_panSensitivity;
    m_target = m_target + right * (-deltaX * panScale) + up * (deltaY * panScale);
}

void Camera::zoom(f32 delta) {
    m_distance *= (1.0f - delta * m_zoomSensitivity);
    m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);
}

void Camera::reset() {
    // Restore default orientation
    m_pitch = 30.0f;
    m_yaw = 45.0f;

    // If we have stored bounds from a previous fitToBounds, re-fit
    // Otherwise reset to default view
    if (m_hasBounds) {
        m_target = m_lastBoundsCenter;
        m_distance = m_lastBoundsExtent * 2.0f;
        m_nearPlane = m_distance * 0.01f;
        m_farPlane = m_distance * 100.0f;
    } else {
        m_target = Vec3{0.0f, 0.0f, 0.0f};
        m_distance = 5.0f;
    }
    updateVectors();
}

void Camera::fitToBounds(const Vec3& min, const Vec3& max) {
    Vec3 center{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f};

    Vec3 size = max - min;
    f32 maxExtent = std::max({size.x, size.y, size.z});

    // Store bounds for reset
    m_lastBoundsCenter = center;
    m_lastBoundsExtent = maxExtent;
    m_hasBounds = true;

    // Pan to center and zoom to fit — preserve current orientation
    m_target = center;
    m_distance = maxExtent * 2.0f;

    // Ensure reasonable clip planes
    m_nearPlane = m_distance * 0.01f;
    m_farPlane = m_distance * 100.0f;

    updateVectors();
}

void Camera::setViewport(int width, int height) {
    m_viewportWidth = std::max(1, width);
    m_viewportHeight = std::max(1, height);
}

f32 Camera::aspectRatio() const {
    return static_cast<f32>(m_viewportWidth) / static_cast<f32>(m_viewportHeight);
}

void Camera::setDistance(f32 distance) {
    m_distance = std::clamp(distance, m_minDistance, m_maxDistance);
}

void Camera::setClipPlanes(f32 nearPlane, f32 farPlane) {
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

Vec3 Camera::position() const {
    f32 yawRad = m_yaw * DEG_TO_RAD;
    f32 pitchRad = m_pitch * DEG_TO_RAD;

    Vec3 offset{std::sin(yawRad) * std::cos(pitchRad) * m_distance, std::sin(pitchRad) * m_distance,
                std::cos(yawRad) * std::cos(pitchRad) * m_distance};

    return m_target + offset;
}

void Camera::updateVectors() {
    // Currently not caching derived values, but could be added for performance
}

} // namespace dw
