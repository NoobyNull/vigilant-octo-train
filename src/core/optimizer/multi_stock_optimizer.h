#pragma once

#include <string>
#include <vector>

#include "cut_optimizer.h"
#include "sheet.h"
#include "waste_breakdown.h"

namespace dw {
namespace optimizer {

// A group of parts sharing the same materialId, with available stock sizes
struct MaterialGroup {
    i64 materialId = 0;
    std::string materialName;
    std::vector<Part> parts;
    std::vector<Sheet> stockSizes; // Available sheets for this material
};

// Result of multi-stock optimization -- one CutPlan per material group
struct MultiStockResult {
    struct GroupResult {
        i64 materialId = 0;
        std::string materialName;
        CutPlan plan;
        WasteBreakdown waste;
        Sheet usedSheet; // Which stock size was selected
    };
    std::vector<GroupResult> groups;
    f32 totalCost = 0.0f;
    int totalSheetsUsed = 0;
};

// Group parts by materialId, then for each group try all available stock sizes
// and pick the one minimizing total cost (sheets_used * sheet_cost).
// Falls back to the first stock size if costs are equal.
MultiStockResult optimizeMultiStock(
    const std::vector<Part>& parts,
    const std::vector<MaterialGroup>& materials,
    Algorithm algorithm,
    bool allowRotation, f32 kerf, f32 margin);

} // namespace optimizer
} // namespace dw
