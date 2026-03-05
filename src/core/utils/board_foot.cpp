#include "board_foot.h"

namespace dw {

f64 calcBoardFeet(f64 thicknessMm, f64 widthMm, f64 heightMm) {
    (void)thicknessMm; (void)widthMm; (void)heightMm;
    return 0.0;
}

f64 calcCostPerBoardFoot(f64 pricePerUnit, const std::string& unitLabel,
                          f64 thicknessMm, f64 widthMm, f64 heightMm) {
    (void)pricePerUnit; (void)unitLabel; (void)thicknessMm; (void)widthMm; (void)heightMm;
    return 0.0;
}

std::string formatBoardFeet(f64 bf) {
    (void)bf;
    return "";
}

std::string formatCostPerBoardFoot(f64 costPerBF) {
    (void)costPerBF;
    return "";
}

} // namespace dw
