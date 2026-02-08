#pragma once

#include <algorithm>
#include <vector>

#include "sheet.h"

namespace dw {
namespace optimizer {

// A single instance of a part, expanded from quantity.
// Used by optimizer algorithms to track individual placements.
struct ExpandedPart {
    const Part* part;
    int partIndex;
    int instanceIndex;
    f32 area;
};

// Expand parts by quantity into individual instances and sort by area (largest first).
// Each Part with quantity N produces N ExpandedPart entries.
inline std::vector<ExpandedPart> expandParts(const std::vector<Part>& parts) {
    std::vector<ExpandedPart> expanded;
    for (int i = 0; i < static_cast<int>(parts.size()); ++i) {
        const Part& part = parts[i];
        for (int j = 0; j < part.quantity; ++j) {
            expanded.push_back({&part, i, j, part.area()});
        }
    }

    // Sort by area (largest first) for best-fit-decreasing heuristic
    std::sort(expanded.begin(), expanded.end(),
              [](const ExpandedPart& a, const ExpandedPart& b) { return a.area > b.area; });

    return expanded;
}

} // namespace optimizer
} // namespace dw
