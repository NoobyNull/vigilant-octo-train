#pragma once

#include <optional>
#include <string>
#include "../types.h"

namespace dw {

// Conversion constants
constexpr f64 MM_PER_INCH = 25.4;
constexpr f64 MM_PER_FOOT = 304.8;
constexpr f64 MM_PER_CM = 10.0;
constexpr f64 MM_PER_M = 1000.0;

f64 mmToInches(f64 mm);
f64 inchesToMm(f64 inches);
f64 feetToMm(f64 feet);
f64 cmToMm(f64 cm);
f64 mToMm(f64 m);

// Parse user input string to millimeters. isMetric = true means bare numbers are mm.
std::optional<f64> parseDimension(const std::string& input, bool isMetric);

// Format mm value to display string with units. Uses feet-inches for SAE.
std::string formatDimension(f64 valueMm, bool isMetric);

// Compact format for table cells (no unit suffix in metric, shorter in SAE).
std::string formatDimensionCompact(f64 valueMm, bool isMetric);

} // namespace dw
