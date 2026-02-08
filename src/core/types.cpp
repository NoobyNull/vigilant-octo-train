#include "types.h"

#include <algorithm>
#include <cmath>

namespace dw {

Vec2 toSpherical(const Vec3& dir) {
    f32 len = glm::length(dir);
    if (len < 0.001f)
        len = 1.0f;
    f32 azimuth = std::atan2(dir.x, dir.z);
    f32 elevation = std::asin(std::clamp(-dir.y / len, -1.0f, 1.0f));
    return {azimuth, elevation};
}

Vec3 fromSpherical(f32 azimuth, f32 elevation) {
    return {std::sin(azimuth) * std::cos(elevation), -std::sin(elevation),
            std::cos(azimuth) * std::cos(elevation)};
}

} // namespace dw
