#pragma once

#include "../types.h"
#include "heightmap.h"

#include <utility>
#include <vector>

namespace dw {
namespace carve {

struct Island {
    int id = 0;
    std::vector<std::pair<int, int>> cells;  // (col, row) grid positions
    f32 minZ = 0.0f;           // Deepest point in island
    f32 maxZ = 0.0f;           // Shallowest point (entry rim)
    f32 depth = 0.0f;          // maxZ - minZ
    f32 areaMm2 = 0.0f;        // Physical area in mm^2
    f32 minClearDiameter = 0.0f; // Min clearing tool diameter (mm)
    Vec2 centroid;              // Center position in world coords
    Vec2 boundsMin, boundsMax;  // Bounding box in world coords
};

struct IslandResult {
    std::vector<Island> islands;
    // Grid mask: -1 = not island, >= 0 = island ID
    std::vector<int> islandMask;
    int maskCols = 0;
    int maskRows = 0;
};

// Detect islands in heightmap.
// toolAngleDeg: included angle of finishing tool (V-bit).
//   A cell is "buried" if surrounding height exceeds what
//   the taper can reach at that XY distance.
// minIslandAreaMm2: ignore islands smaller than this (mm^2)
IslandResult detectIslands(const Heightmap& heightmap,
                           f32 toolAngleDeg,
                           f32 minIslandAreaMm2 = 1.0f);

} // namespace carve
} // namespace dw
