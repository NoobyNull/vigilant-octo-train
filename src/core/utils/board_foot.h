#pragma once

#include <string>
#include "../types.h"

namespace dw {

// Calculate board-foot volume from dimensions in mm.
// Returns 0.0 if any required dimension is zero.
f64 calcBoardFeet(f64 thicknessMm, f64 widthMm, f64 heightMm);

// Calculate cost-per-board-foot given pricing unit and dimensions.
// unitLabel: "sheet", "board foot", "linear foot", "each"
// Returns 0.0 if calculation is not possible.
f64 calcCostPerBoardFoot(f64 pricePerUnit, const std::string& unitLabel,
                          f64 thicknessMm, f64 widthMm, f64 heightMm);

// Format board-foot value for display. Returns "-- BF" if bf <= 0.
std::string formatBoardFeet(f64 bf);

// Format cost-per-board-foot for display. Returns "--" if value <= 0.
std::string formatCostPerBoardFoot(f64 costPerBF);

} // namespace dw
