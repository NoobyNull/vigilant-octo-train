#pragma once

#include <vector>
#include <string>

#include "sheet.h"

namespace dw {
namespace optimizer {

// A rectangular scrap piece remaining after cuts
struct ScrapPiece {
    f32 x = 0.0f, y = 0.0f;         // Position on sheet
    f32 width = 0.0f, height = 0.0f; // Dimensions
    int sheetIndex = 0;               // Which result sheet

    f32 area() const { return width * height; }
};

// Complete waste breakdown for an optimization run
struct WasteBreakdown {
    std::vector<ScrapPiece> scrapPieces; // Usable rectangular remainders
    f32 totalScrapArea = 0.0f;           // Sum of scrap piece areas (sq mm)
    f32 totalKerfArea = 0.0f;            // Total kerf loss across entire run (sq mm)
    f32 totalUnusableArea = 0.0f;        // Waste that is neither scrap nor kerf
    f32 scrapValue = 0.0f;              // Dollar value of scrap (scrapArea * rate)
    f32 kerfValue = 0.0f;               // Dollar value of kerf loss
    f32 unusableValue = 0.0f;           // Dollar value of unusable waste
    f32 totalWasteValue = 0.0f;         // Sum of all waste values
};

// Compute waste breakdown from a completed CutPlan
// sheetTemplate: the sheet dimensions/cost used for the run
// kerf: blade width in mm
WasteBreakdown computeWasteBreakdown(const CutPlan& plan,
                                      const Sheet& sheetTemplate,
                                      f32 kerf);

} // namespace optimizer
} // namespace dw
