#include "board_foot.h"
#include "unit_conversion.h"

#include <cstdio>

namespace dw {

f64 calcBoardFeet(f64 thicknessMm, f64 widthMm, f64 heightMm) {
    if (thicknessMm <= 0.0 || widthMm <= 0.0 || heightMm <= 0.0) {
        return 0.0;
    }

    f64 tIn = mmToInches(thicknessMm);
    f64 wIn = mmToInches(widthMm);
    f64 lIn = mmToInches(heightMm);

    return (tIn * wIn * lIn) / 144.0;
}

f64 calcCostPerBoardFoot(f64 pricePerUnit, const std::string& unitLabel,
                          f64 thicknessMm, f64 widthMm, f64 heightMm) {
    if (pricePerUnit <= 0.0) return 0.0;

    if (unitLabel == "board foot") {
        // Already priced per board foot
        return pricePerUnit;
    }

    if (unitLabel == "linear foot") {
        // BF per linear foot = (T_in * W_in) / 12
        f64 tIn = mmToInches(thicknessMm);
        f64 wIn = mmToInches(widthMm);
        if (tIn <= 0.0 || wIn <= 0.0) return 0.0;

        f64 bfPerLF = (tIn * wIn) / 12.0;
        if (bfPerLF <= 0.0) return 0.0;

        return pricePerUnit / bfPerLF;
    }

    // "sheet" or "each": calculate total BF, divide price by it
    f64 bf = calcBoardFeet(thicknessMm, widthMm, heightMm);
    if (bf <= 0.0) return 0.0;

    return pricePerUnit / bf;
}

std::string formatBoardFeet(f64 bf) {
    if (bf <= 0.0) return "-- BF";

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f BF", bf);
    return std::string(buf);
}

std::string formatCostPerBoardFoot(f64 costPerBF) {
    if (costPerBF <= 0.0) return "--";

    char buf[32];
    std::snprintf(buf, sizeof(buf), "$%.2f/BF", costPerBF);
    return std::string(buf);
}

} // namespace dw
