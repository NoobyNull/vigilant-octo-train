#include "unit_conversion.h"

namespace dw {

f64 mmToInches(f64 mm) { (void)mm; return 0.0; }
f64 inchesToMm(f64 inches) { (void)inches; return 0.0; }
f64 feetToMm(f64 feet) { (void)feet; return 0.0; }
f64 cmToMm(f64 cm) { (void)cm; return 0.0; }
f64 mToMm(f64 m) { (void)m; return 0.0; }

std::optional<f64> parseDimension(const std::string& input, bool isMetric) {
    (void)input; (void)isMetric;
    return std::nullopt;
}

std::string formatDimension(f64 valueMm, bool isMetric) {
    (void)valueMm; (void)isMetric;
    return "";
}

std::string formatDimensionCompact(f64 valueMm, bool isMetric) {
    (void)valueMm; (void)isMetric;
    return "";
}

} // namespace dw
