#pragma once

#include "../types.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dw {

// Axis-aligned bounding box
struct AABB {
    Vec3 min{std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max(),
             std::numeric_limits<f32>::max()};
    Vec3 max{std::numeric_limits<f32>::lowest(), std::numeric_limits<f32>::lowest(),
             std::numeric_limits<f32>::lowest()};

    AABB() = default;
    AABB(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}

    // Expand to include a point
    void expand(const Vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    // Expand to include another AABB
    void expand(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }

    // Get dimensions
    Vec3 size() const { return max - min; }

    f32 width() const { return max.x - min.x; }
    f32 height() const { return max.y - min.y; }
    f32 depth() const { return max.z - min.z; }

    // Get center point
    Vec3 center() const {
        return Vec3{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f,
                    (min.z + max.z) * 0.5f};
    }

    // Check if valid (min <= max)
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Check if contains a point
    bool contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y &&
               point.y <= max.y && point.z >= min.z && point.z <= max.z;
    }

    // Check if intersects another AABB
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    // Get longest axis dimension
    f32 maxExtent() const {
        return std::max({width(), height(), depth()});
    }

    // Get diagonal length
    f32 diagonal() const {
        Vec3 d = size();
        return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    }

    // Reset to invalid state
    void reset() {
        min = Vec3{std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max(),
                   std::numeric_limits<f32>::max()};
        max = Vec3{std::numeric_limits<f32>::lowest(), std::numeric_limits<f32>::lowest(),
                   std::numeric_limits<f32>::lowest()};
    }
};

}  // namespace dw
