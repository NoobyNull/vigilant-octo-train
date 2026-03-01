#pragma once

#include "../mesh/vertex.h"
#include "../types.h"

#include <functional>
#include <string>
#include <vector>

namespace dw {
namespace carve {

struct HeightmapConfig {
    f32 resolutionMm = 0.1f;  // Grid spacing in mm
    f32 defaultZ = 0.0f;      // Z value for cells with no intersection
};

class Heightmap {
  public:
    Heightmap() = default;

    /// Build from mesh vertex/index data (caller manages threading).
    /// @param vertices  Mesh vertex array
    /// @param indices   Triangle indices (groups of 3)
    /// @param boundsMin Minimum corner of mesh AABB
    /// @param boundsMax Maximum corner of mesh AABB
    /// @param config    Grid resolution and default Z
    /// @param progress  Optional callback receiving [0.0, 1.0]
    void build(const std::vector<Vertex>& vertices,
               const std::vector<u32>& indices,
               const Vec3& boundsMin, const Vec3& boundsMax,
               const HeightmapConfig& config,
               std::function<void(f32)> progress = nullptr);

    // Grid accessors
    f32 at(int col, int row) const;
    f32 atMm(f32 x, f32 y) const;  // Bilinear interpolation at world XY
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }
    f32 resolution() const { return m_resolution; }
    Vec3 boundsMin() const { return m_boundsMin; }
    Vec3 boundsMax() const { return m_boundsMax; }
    bool empty() const { return m_grid.empty(); }

    // Statistics
    f32 minZ() const { return m_minZ; }
    f32 maxZ() const { return m_maxZ; }

    // Persistence — binary .dwhm format (Digital Workshop HeightMap)
    // Header: magic(4) + version(4) + cols(4) + rows(4) + resolution(4)
    //         + boundsMin(12) + boundsMax(12) + minZ(4) + maxZ(4) = 52 bytes
    // Body:   cols * rows * sizeof(f32) raw grid data
    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // Export as 16-bit grayscale PNG for visualization / external use
    bool exportPng(const std::string& path) const;

  private:
    // Internal triangle representation with pre-resolved positions
    struct TriPos {
        Vec3 a, b, c;
        f32 minX, maxX, minY, maxY;  // XY bounding box
    };

    // Spatial acceleration: coarse 2D grid of triangle bucket indices
    struct SpatialBins {
        std::vector<std::vector<int>> bins;  // bin index -> list of TriPos indices
        int binCols = 0;
        int binRows = 0;
        f32 binSize = 1.0f;
    };

    void buildGrid(const std::vector<TriPos>& tris,
                   const SpatialBins& bins,
                   std::function<void(f32)> progress);
    static SpatialBins binTriangles(const std::vector<TriPos>& tris,
                                    const Vec3& boundsMin,
                                    const Vec3& boundsMax);
    static f32 castRay(f32 rayX, f32 rayY,
                       const std::vector<TriPos>& tris,
                       const std::vector<int>& bucket,
                       f32 defaultZ);

    std::vector<f32> m_grid;
    int m_cols = 0;
    int m_rows = 0;
    f32 m_resolution = 0.1f;
    Vec3 m_boundsMin{0.0f};
    Vec3 m_boundsMax{0.0f};
    f32 m_minZ = 0.0f;
    f32 m_maxZ = 0.0f;
    f32 m_defaultZ = 0.0f;
};

} // namespace carve
} // namespace dw
