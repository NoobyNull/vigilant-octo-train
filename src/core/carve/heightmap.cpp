#include "heightmap.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace dw {
namespace carve {

// ---- Moller-Trumbore ray-triangle intersection ----
// Ray: origin (rayX, rayY, +INF), direction (0, 0, -1)
// Returns Z of intersection, or -INF if no hit.
static f32 rayTriangleIntersect(f32 rayX, f32 rayY,
                                const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 edge1 = b - a;
    const Vec3 edge2 = c - a;

    // direction = (0, 0, -1)
    // cross(dir, edge2) = (0*e2z - (-1)*e2y, (-1)*e2x - 0*e2z, 0*e2y - 0*e2x)
    //                   = (e2y, -e2x, 0)
    const Vec3 h{edge2.y, -edge2.x, 0.0f};
    const f32 det = glm::dot(edge1, h);

    // Parallel or degenerate
    constexpr f32 kEpsilon = 1e-7f;
    if (det > -kEpsilon && det < kEpsilon)
        return -std::numeric_limits<f32>::infinity();

    const f32 invDet = 1.0f / det;

    // s = origin - a; origin.z is +INF but we only need s.x and s.y
    // for u parameter: u = invDet * dot(s, h) where h.z = 0
    // so only s.x * h.x + s.y * h.y matters
    const f32 sx = rayX - a.x;
    const f32 sy = rayY - a.y;
    const f32 u = invDet * (sx * h.x + sy * h.y);
    if (u < 0.0f || u > 1.0f)
        return -std::numeric_limits<f32>::infinity();

    // q = cross(s, edge1)
    // But s.z is very large (+INF). We need a formulation that avoids INF.
    // Instead, use the barycentric approach directly:
    // For a vertical ray at (rayX, rayY), the barycentric coords are
    // determined purely by XY projection.

    // Full barycentric computation in XY
    const f32 denom = edge1.x * edge2.y - edge1.y * edge2.x;
    if (denom > -kEpsilon && denom < kEpsilon)
        return -std::numeric_limits<f32>::infinity();

    const f32 invDenom = 1.0f / denom;
    const f32 baryU = invDenom * (sx * edge2.y - sy * edge2.x);
    const f32 baryV = invDenom * (edge1.x * sy - edge1.y * sx);

    if (baryU < 0.0f || baryV < 0.0f || (baryU + baryV) > 1.0f)
        return -std::numeric_limits<f32>::infinity();

    // Intersection Z via barycentric interpolation
    return a.z + baryU * edge1.z + baryV * edge2.z;
}

// ---- Spatial binning ----
Heightmap::SpatialBins Heightmap::binTriangles(
    const std::vector<TriPos>& tris,
    const Vec3& boundsMin, const Vec3& boundsMax) {

    SpatialBins bins;
    constexpr int kTargetBins = 64;
    const f32 spanX = boundsMax.x - boundsMin.x;
    const f32 spanY = boundsMax.y - boundsMin.y;
    bins.binSize = std::max(spanX, spanY) / static_cast<f32>(kTargetBins);
    if (bins.binSize < 1e-6f)
        bins.binSize = 1.0f;

    bins.binCols = std::max(1, static_cast<int>(std::ceil(spanX / bins.binSize)));
    bins.binRows = std::max(1, static_cast<int>(std::ceil(spanY / bins.binSize)));
    bins.bins.resize(static_cast<usize>(bins.binCols * bins.binRows));

    for (int i = 0; i < static_cast<int>(tris.size()); ++i) {
        const auto& tri = tris[static_cast<usize>(i)];
        int c0 = std::max(0, static_cast<int>((tri.minX - boundsMin.x) / bins.binSize));
        int c1 = std::min(bins.binCols - 1,
                          static_cast<int>((tri.maxX - boundsMin.x) / bins.binSize));
        int r0 = std::max(0, static_cast<int>((tri.minY - boundsMin.y) / bins.binSize));
        int r1 = std::min(bins.binRows - 1,
                          static_cast<int>((tri.maxY - boundsMin.y) / bins.binSize));
        for (int r = r0; r <= r1; ++r) {
            for (int c = c0; c <= c1; ++c) {
                bins.bins[static_cast<usize>(r * bins.binCols + c)].push_back(i);
            }
        }
    }
    return bins;
}

// ---- Ray casting for a single grid cell ----
f32 Heightmap::castRay(f32 rayX, f32 rayY,
                       const std::vector<TriPos>& tris,
                       const std::vector<int>& bucket,
                       f32 defaultZ) {
    f32 bestZ = -std::numeric_limits<f32>::infinity();
    for (int idx : bucket) {
        const auto& tri = tris[static_cast<usize>(idx)];
        if (rayX < tri.minX || rayX > tri.maxX || rayY < tri.minY || rayY > tri.maxY)
            continue;
        f32 z = rayTriangleIntersect(rayX, rayY, tri.a, tri.b, tri.c);
        if (z > bestZ)
            bestZ = z;
    }
    return (bestZ > -std::numeric_limits<f32>::max()) ? bestZ : defaultZ;
}

// ---- Grid construction (row-by-row) ----
void Heightmap::buildGrid(const std::vector<TriPos>& tris,
                          const SpatialBins& bins,
                          std::function<void(f32)> progress) {
    m_grid.assign(static_cast<usize>(m_cols * m_rows), m_minZ);
    m_minZ = std::numeric_limits<f32>::max();
    m_maxZ = -std::numeric_limits<f32>::max();

    const int progressInterval = std::max(1, m_rows / 100);

    for (int row = 0; row < m_rows; ++row) {
        const f32 worldY = m_boundsMin.y + static_cast<f32>(row) * m_resolution;
        const int binRow = std::min(
            bins.binRows - 1,
            static_cast<int>((worldY - m_boundsMin.y) / bins.binSize));

        for (int col = 0; col < m_cols; ++col) {
            const f32 worldX = m_boundsMin.x + static_cast<f32>(col) * m_resolution;
            const int binCol = std::min(
                bins.binCols - 1,
                static_cast<int>((worldX - m_boundsMin.x) / bins.binSize));

            const auto& bucket =
                bins.bins[static_cast<usize>(binRow * bins.binCols + binCol)];
            const f32 z = castRay(worldX, worldY, tris, bucket, m_defaultZ);

            m_grid[static_cast<usize>(row * m_cols + col)] = z;
            if (z < m_minZ) m_minZ = z;
            if (z > m_maxZ) m_maxZ = z;
        }

        if (progress && (row % progressInterval == 0 || row == m_rows - 1)) {
            progress(static_cast<f32>(row + 1) / static_cast<f32>(m_rows));
        }
    }
}

// ---- Public API ----
void Heightmap::build(const std::vector<Vertex>& vertices,
                      const std::vector<u32>& indices,
                      const Vec3& boundsMin, const Vec3& boundsMax,
                      const HeightmapConfig& config,
                      std::function<void(f32)> progress) {
    m_boundsMin = boundsMin;
    m_boundsMax = boundsMax;
    m_resolution = config.resolutionMm;
    m_defaultZ = config.defaultZ;

    const f32 spanX = boundsMax.x - boundsMin.x;
    const f32 spanY = boundsMax.y - boundsMin.y;

    if (spanX < 1e-6f || spanY < 1e-6f || indices.size() < 3) {
        m_grid.clear();
        m_cols = 0;
        m_rows = 0;
        m_minZ = 0.0f;
        m_maxZ = 0.0f;
        return;
    }

    m_cols = std::max(1, static_cast<int>(std::ceil(spanX / m_resolution)));
    m_rows = std::max(1, static_cast<int>(std::ceil(spanY / m_resolution)));

    // Build TriPos array from indexed vertices
    const usize triCount = indices.size() / 3;
    std::vector<TriPos> tris;
    tris.reserve(triCount);
    for (usize i = 0; i < triCount; ++i) {
        const Vec3& a = vertices[indices[i * 3 + 0]].position;
        const Vec3& b = vertices[indices[i * 3 + 1]].position;
        const Vec3& c = vertices[indices[i * 3 + 2]].position;
        TriPos tp;
        tp.a = a;
        tp.b = b;
        tp.c = c;
        tp.minX = std::min({a.x, b.x, c.x});
        tp.maxX = std::max({a.x, b.x, c.x});
        tp.minY = std::min({a.y, b.y, c.y});
        tp.maxY = std::max({a.y, b.y, c.y});
        tris.push_back(tp);
    }

    auto bins = binTriangles(tris, boundsMin, boundsMax);
    buildGrid(tris, bins, std::move(progress));
}

f32 Heightmap::at(int col, int row) const {
    if (col < 0 || col >= m_cols || row < 0 || row >= m_rows)
        return m_boundsMin.z;
    return m_grid[static_cast<usize>(row * m_cols + col)];
}

f32 Heightmap::atMm(f32 x, f32 y) const {
    if (empty())
        return 0.0f;

    // Single-cell grid: no neighbors for bilinear interpolation
    if (m_cols < 2 || m_rows < 2)
        return m_grid[0];

    // Convert world coords to fractional grid coords
    const f32 fx = (x - m_boundsMin.x) / m_resolution;
    const f32 fy = (y - m_boundsMin.y) / m_resolution;

    // Clamp to valid range
    const f32 maxCol = static_cast<f32>(m_cols - 1);
    const f32 maxRow = static_cast<f32>(m_rows - 1);
    const f32 cx = std::clamp(fx, 0.0f, maxCol);
    const f32 cy = std::clamp(fy, 0.0f, maxRow);

    // Integer and fractional parts
    const int c0 = std::min(static_cast<int>(cx), m_cols - 2);
    const int r0 = std::min(static_cast<int>(cy), m_rows - 2);
    const int c1 = c0 + 1;
    const int r1 = r0 + 1;
    const f32 tx = cx - static_cast<f32>(c0);
    const f32 ty = cy - static_cast<f32>(r0);

    // Bilinear interpolation
    const f32 z00 = m_grid[static_cast<usize>(r0 * m_cols + c0)];
    const f32 z10 = m_grid[static_cast<usize>(r0 * m_cols + c1)];
    const f32 z01 = m_grid[static_cast<usize>(r1 * m_cols + c0)];
    const f32 z11 = m_grid[static_cast<usize>(r1 * m_cols + c1)];

    const f32 top = z00 * (1.0f - tx) + z10 * tx;
    const f32 bot = z01 * (1.0f - tx) + z11 * tx;
    return top * (1.0f - ty) + bot * ty;
}

// ---- Persistence: binary .dwhm format ----

static constexpr u32 DWHM_MAGIC   = 0x4D485744; // "DWHM"
static constexpr u32 DWHM_VERSION = 1;

bool Heightmap::save(const std::string& path) const {
    if (m_grid.empty()) return false;

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    // Header
    f.write(reinterpret_cast<const char*>(&DWHM_MAGIC), 4);
    f.write(reinterpret_cast<const char*>(&DWHM_VERSION), 4);
    f.write(reinterpret_cast<const char*>(&m_cols), 4);
    f.write(reinterpret_cast<const char*>(&m_rows), 4);
    f.write(reinterpret_cast<const char*>(&m_resolution), 4);
    f.write(reinterpret_cast<const char*>(&m_boundsMin), sizeof(Vec3));
    f.write(reinterpret_cast<const char*>(&m_boundsMax), sizeof(Vec3));
    f.write(reinterpret_cast<const char*>(&m_minZ), 4);
    f.write(reinterpret_cast<const char*>(&m_maxZ), 4);

    // Grid data
    f.write(reinterpret_cast<const char*>(m_grid.data()),
            static_cast<std::streamsize>(m_grid.size() * sizeof(f32)));

    return f.good();
}

bool Heightmap::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    u32 magic = 0, version = 0;
    f.read(reinterpret_cast<char*>(&magic), 4);
    f.read(reinterpret_cast<char*>(&version), 4);
    if (magic != DWHM_MAGIC || version != DWHM_VERSION) return false;

    f.read(reinterpret_cast<char*>(&m_cols), 4);
    f.read(reinterpret_cast<char*>(&m_rows), 4);
    f.read(reinterpret_cast<char*>(&m_resolution), 4);
    f.read(reinterpret_cast<char*>(&m_boundsMin), sizeof(Vec3));
    f.read(reinterpret_cast<char*>(&m_boundsMax), sizeof(Vec3));
    f.read(reinterpret_cast<char*>(&m_minZ), 4);
    f.read(reinterpret_cast<char*>(&m_maxZ), 4);

    if (m_cols <= 0 || m_rows <= 0) return false;

    auto count = static_cast<size_t>(m_cols) * static_cast<size_t>(m_rows);
    m_grid.resize(count);
    f.read(reinterpret_cast<char*>(m_grid.data()),
           static_cast<std::streamsize>(count * sizeof(f32)));

    return f.good();
}

bool Heightmap::exportPng(const std::string& path) const {
    if (m_grid.empty()) return false;

    // Export as PGM (Portable GrayMap) — universally readable, no library deps.
    // If path ends in .png, we still write PGM since stb isn't linked here.
    std::string actualPath = path;
    // Ensure .pgm extension for clarity
    auto dotPos = actualPath.rfind('.');
    if (dotPos != std::string::npos)
        actualPath = actualPath.substr(0, dotPos) + ".pgm";
    else
        actualPath += ".pgm";

    std::ofstream f(actualPath, std::ios::binary);
    if (!f.is_open()) return false;

    f32 range = m_maxZ - m_minZ;
    if (range < 1e-6f) range = 1.0f;

    // P5 binary PGM header
    f << "P5\n" << m_cols << " " << m_rows << "\n65535\n";

    // 16-bit big-endian pixel data
    for (int i = 0; i < static_cast<int>(m_grid.size()); ++i) {
        f32 normalized = (m_grid[static_cast<size_t>(i)] - m_minZ) / range;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        u16 val = static_cast<u16>(normalized * 65535.0f);
        // PGM is big-endian
        u8 hi = static_cast<u8>(val >> 8);
        u8 lo = static_cast<u8>(val & 0xFF);
        f.put(static_cast<char>(hi));
        f.put(static_cast<char>(lo));
    }

    return f.good();
}

} // namespace carve
} // namespace dw
