#include "bin_packer.h"

#include <algorithm>
#include <limits>

#include "optimizer_utils.h"

namespace dw {
namespace optimizer {

namespace {
constexpr f32 PLACEMENT_EPSILON = 0.001f;
} // namespace

CutPlan BinPacker::optimize(const std::vector<Part>& parts, const std::vector<Sheet>& sheets) {
    CutPlan plan;

    if (parts.empty() || sheets.empty()) {
        return plan;
    }

    // Expand parts by quantity and sort by area (decreasing)
    auto expandedParts = expandParts(parts);

    // Track which parts have been placed
    std::vector<bool> placed(expandedParts.size(), false);

    // Process sheets
    for (int sheetIdx = 0; sheetIdx < static_cast<int>(sheets.size()); ++sheetIdx) {
        const Sheet& sheet = sheets[sheetIdx];

        // Effective sheet size after margins
        f32 effectiveWidth = sheet.width - 2 * m_margin;
        f32 effectiveHeight = sheet.height - 2 * m_margin;

        if (effectiveWidth <= 0 || effectiveHeight <= 0) {
            continue;
        }

        // Start with one free rectangle covering the whole sheet
        std::vector<FreeRect> freeRects;
        freeRects.push_back({m_margin, m_margin, effectiveWidth, effectiveHeight});

        SheetResult sheetResult;
        sheetResult.sheetIndex = sheetIdx;

        // Try to place each unplaced part
        for (usize i = 0; i < expandedParts.size(); ++i) {
            if (placed[i]) {
                continue;
            }

            const auto& ep = expandedParts[i];
            Placement placement;
            placement.part = ep.part;
            placement.partIndex = ep.partIndex;
            placement.instanceIndex = ep.instanceIndex;

            if (tryPlace(*ep.part, ep.partIndex, ep.instanceIndex, freeRects, placement)) {
                sheetResult.placements.push_back(placement);
                sheetResult.usedArea += ep.part->area();
                placed[i] = true;
            }
        }

        // Only add sheet if it has placements
        if (!sheetResult.placements.empty()) {
            sheetResult.wasteArea = sheet.area() - sheetResult.usedArea;
            plan.sheets.push_back(sheetResult);
            plan.totalUsedArea += sheetResult.usedArea;
            plan.totalWasteArea += sheetResult.wasteArea;
            plan.totalCost += sheet.cost;
            plan.sheetsUsed++;
        }

        // Check if all parts are placed
        bool allPlaced = true;
        for (bool p : placed) {
            if (!p) {
                allPlaced = false;
                break;
            }
        }

        if (allPlaced) {
            break;
        }
    }

    // Collect unplaced parts
    for (usize i = 0; i < expandedParts.size(); ++i) {
        if (!placed[i]) {
            const auto& ep = expandedParts[i];
            Part unplaced = *ep.part;
            unplaced.quantity = 1; // Each instance is separate
            plan.unplacedParts.push_back(unplaced);
        }
    }

    return plan;
}

bool BinPacker::tryPlace(const Part& part, int partIndex, int instanceIndex,
                         std::vector<FreeRect>& freeRects, Placement& outPlacement) {
    f32 partWidth = part.width + m_kerf;
    f32 partHeight = part.height + m_kerf;

    // Find best-fitting free rectangle
    usize bestRect = std::numeric_limits<usize>::max();
    bool bestRotated = false;
    f32 bestScore = std::numeric_limits<f32>::max();

    for (usize i = 0; i < freeRects.size(); ++i) {
        const FreeRect& rect = freeRects[i];

        // Try normal orientation (with epsilon tolerance for float rounding)
        if (partWidth <= rect.width + PLACEMENT_EPSILON &&
            partHeight <= rect.height + PLACEMENT_EPSILON) {
            f32 score = rect.width * rect.height - partWidth * partHeight;
            if (score < bestScore) {
                bestScore = score;
                bestRect = i;
                bestRotated = false;
            }
        }

        // Try rotated orientation
        if (m_allowRotation && partHeight <= rect.width + PLACEMENT_EPSILON &&
            partWidth <= rect.height + PLACEMENT_EPSILON) {
            f32 score = rect.width * rect.height - partHeight * partWidth;
            if (score < bestScore) {
                bestScore = score;
                bestRect = i;
                bestRotated = true;
            }
        }
    }

    if (bestRect == std::numeric_limits<usize>::max()) {
        return false;
    }

    // Place the part
    outPlacement.partIndex = partIndex;
    outPlacement.instanceIndex = instanceIndex;
    outPlacement.x = freeRects[bestRect].x;
    outPlacement.y = freeRects[bestRect].y;
    outPlacement.rotated = bestRotated;

    // Split the free rectangle
    splitFreeRect(freeRects, bestRect, outPlacement);

    return true;
}

void BinPacker::splitFreeRect(std::vector<FreeRect>& freeRects, usize rectIndex,
                              const Placement& placement) {
    FreeRect rect = freeRects[rectIndex];

    f32 placedWidth = placement.getWidth() + m_kerf;
    f32 placedHeight = placement.getHeight() + m_kerf;

    // Remove the used rectangle
    freeRects.erase(freeRects.begin() + static_cast<long>(rectIndex));

    // Create new free rectangles from remaining space
    // Right remainder
    if (rect.width - placedWidth > 0) {
        FreeRect right;
        right.x = rect.x + placedWidth;
        right.y = rect.y;
        right.width = rect.width - placedWidth;
        right.height = rect.height;
        freeRects.push_back(right);
    }

    // Top remainder
    if (rect.height - placedHeight > 0) {
        FreeRect top;
        top.x = rect.x;
        top.y = rect.y + placedHeight;
        top.width = placedWidth;
        top.height = rect.height - placedHeight;
        freeRects.push_back(top);
    }
}

} // namespace optimizer
} // namespace dw
