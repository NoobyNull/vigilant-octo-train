#include "island_detector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace dw {
namespace carve {

namespace {

// Compute burial mask: 1 = buried (tool can't reach), 0 = accessible
std::vector<u8> computeBurialMask(const Heightmap& hm, f32 toolAngleDeg) {
    const int cols = hm.cols();
    const int rows = hm.rows();
    const f32 res = hm.resolution();

    // Half-angle taper slope: how much Z the taper covers per unit XY distance
    const f32 halfAngleRad = (toolAngleDeg * 0.5f) * (3.14159265f / 180.0f);
    const f32 taperSlope = std::tan(halfAngleRad);

    std::vector<u8> mask(static_cast<usize>(cols * rows), 1);  // Start all buried

    // A cell is accessible if from at least one cardinal neighbor,
    // the height difference is within what the taper can reach
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const f32 z = hm.at(col, row);
            // Border cells are always accessible (open edge)
            if (col == 0 || col == cols - 1 || row == 0 || row == rows - 1) {
                mask[static_cast<usize>(row * cols + col)] = 0;
                continue;
            }
            for (int d = 0; d < 4; ++d) {
                const int nc = col + dx[d];
                const int nr = row + dy[d];
                const f32 nz = hm.at(nc, nr);
                // The taper can access this cell if neighbor is higher by
                // at most taperSlope * distance
                if (nz - z <= res * taperSlope) {
                    mask[static_cast<usize>(row * cols + col)] = 0;
                    break;
                }
            }
        }
    }

    // Propagate accessibility: BFS from all accessible cells
    // A buried cell becomes accessible if an accessible neighbor can reach it
    std::queue<std::pair<int, int>> queue;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (mask[static_cast<usize>(row * cols + col)] == 0) {
                queue.push({col, row});
            }
        }
    }

    while (!queue.empty()) {
        auto [c, r] = queue.front();
        queue.pop();
        const f32 z = hm.at(c, r);
        for (int d = 0; d < 4; ++d) {
            const int nc = c + dx[d];
            const int nr = r + dy[d];
            if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) {
                continue;
            }
            if (mask[static_cast<usize>(nr * cols + nc)] == 0) {
                continue;  // Already accessible
            }
            const f32 nz = hm.at(nc, nr);
            // Check if accessible cell at z can reach buried cell at nz
            if (nz - z <= res * taperSlope) {
                mask[static_cast<usize>(nr * cols + nc)] = 0;
                queue.push({nc, nr});
            }
        }
    }

    return mask;
}

// Flood-fill connected buried regions into islands
std::vector<std::vector<std::pair<int, int>>> floodFillIslands(
    const std::vector<u8>& burialMask, int cols, int rows) {
    std::vector<std::vector<std::pair<int, int>>> groups;
    std::vector<bool> visited(static_cast<usize>(cols * rows), false);

    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const auto idx = static_cast<usize>(row * cols + col);
            if (burialMask[idx] == 0 || visited[idx]) {
                continue;
            }
            // BFS flood-fill
            std::vector<std::pair<int, int>> group;
            std::queue<std::pair<int, int>> queue;
            queue.push({col, row});
            visited[idx] = true;

            while (!queue.empty()) {
                auto [c, r] = queue.front();
                queue.pop();
                group.push_back({c, r});
                for (int d = 0; d < 4; ++d) {
                    const int nc = c + dx[d];
                    const int nr = r + dy[d];
                    if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) {
                        continue;
                    }
                    const auto nidx = static_cast<usize>(nr * cols + nc);
                    if (burialMask[nidx] == 0 || visited[nidx]) {
                        continue;
                    }
                    visited[nidx] = true;
                    queue.push({nc, nr});
                }
            }
            groups.push_back(std::move(group));
        }
    }
    return groups;
}

// BFS from island boundary inward to find maximum interior distance
f32 maxDistanceFromRim(const std::vector<std::pair<int, int>>& cells,
                       const std::vector<u8>& burialMask, int cols, int rows, f32 res) {
    // Build a set for fast lookup
    std::vector<int> dist(static_cast<usize>(cols * rows), -1);
    std::queue<std::pair<int, int>> queue;

    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};

    // Find boundary cells (island cells adjacent to non-island)
    for (const auto& [c, r] : cells) {
        bool isBoundary = false;
        for (int d = 0; d < 4; ++d) {
            const int nc = c + dx[d];
            const int nr = r + dy[d];
            if (nc < 0 || nc >= cols || nr < 0 || nr >= rows ||
                burialMask[static_cast<usize>(nr * cols + nc)] == 0) {
                isBoundary = true;
                break;
            }
        }
        if (isBoundary) {
            dist[static_cast<usize>(r * cols + c)] = 0;
            queue.push({c, r});
        }
    }

    int maxDist = 0;
    while (!queue.empty()) {
        auto [c, r] = queue.front();
        queue.pop();
        const int curDist = dist[static_cast<usize>(r * cols + c)];
        for (int d = 0; d < 4; ++d) {
            const int nc = c + dx[d];
            const int nr = r + dy[d];
            if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) {
                continue;
            }
            const auto nidx = static_cast<usize>(nr * cols + nc);
            if (burialMask[nidx] == 0 || dist[nidx] >= 0) {
                continue;
            }
            dist[nidx] = curDist + 1;
            maxDist = std::max(maxDist, curDist + 1);
            queue.push({nc, nr});
        }
    }

    return static_cast<f32>(maxDist) * res;
}

} // namespace

IslandResult detectIslands(const Heightmap& heightmap,
                           f32 toolAngleDeg,
                           f32 minIslandAreaMm2) {
    IslandResult result;

    if (heightmap.empty()) {
        return result;
    }

    const int cols = heightmap.cols();
    const int rows = heightmap.rows();
    const f32 res = heightmap.resolution();
    const f32 cellArea = res * res;

    // Step 1: Compute burial mask
    auto burialMask = computeBurialMask(heightmap, toolAngleDeg);

    // Step 2: Flood-fill islands
    auto groups = floodFillIslands(burialMask, cols, rows);

    // Step 3: Classify and filter islands
    result.islandMask.assign(static_cast<usize>(cols * rows), -1);
    result.maskCols = cols;
    result.maskRows = rows;

    int islandId = 0;
    const Vec3 boundsMin = heightmap.boundsMin();

    for (auto& group : groups) {
        const f32 area = static_cast<f32>(group.size()) * cellArea;
        if (area < minIslandAreaMm2) {
            continue;
        }

        Island island;
        island.id = islandId;
        island.cells = std::move(group);
        island.areaMm2 = area;

        // Compute depth, centroid, bounds
        f32 minZ = std::numeric_limits<f32>::max();
        f32 maxZ = std::numeric_limits<f32>::lowest();
        f32 sumX = 0.0f;
        f32 sumY = 0.0f;
        f32 bMinX = std::numeric_limits<f32>::max();
        f32 bMinY = std::numeric_limits<f32>::max();
        f32 bMaxX = std::numeric_limits<f32>::lowest();
        f32 bMaxY = std::numeric_limits<f32>::lowest();

        for (const auto& [c, r] : island.cells) {
            const f32 z = heightmap.at(c, r);
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);

            const f32 wx = boundsMin.x + static_cast<f32>(c) * res;
            const f32 wy = boundsMin.y + static_cast<f32>(r) * res;
            sumX += wx;
            sumY += wy;
            bMinX = std::min(bMinX, wx);
            bMinY = std::min(bMinY, wy);
            bMaxX = std::max(bMaxX, wx);
            bMaxY = std::max(bMaxY, wy);

            result.islandMask[static_cast<usize>(r * cols + c)] = islandId;
        }

        island.minZ = minZ;
        island.maxZ = maxZ;
        island.depth = maxZ - minZ;

        const auto cellCount = static_cast<f32>(island.cells.size());
        island.centroid = Vec2(sumX / cellCount, sumY / cellCount);
        island.boundsMin = Vec2(bMinX, bMinY);
        island.boundsMax = Vec2(bMaxX, bMaxY);

        // Minimum clearing tool diameter
        const f32 maxDist = maxDistanceFromRim(
            island.cells, burialMask, cols, rows, res);
        island.minClearDiameter = 2.0f * maxDist;

        result.islands.push_back(std::move(island));
        ++islandId;
    }

    return result;
}

} // namespace carve
} // namespace dw
