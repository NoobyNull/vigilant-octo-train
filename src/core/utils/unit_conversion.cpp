#include "unit_conversion.h"
#include "string_utils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace dw {

f64 mmToInches(f64 mm) { return mm / MM_PER_INCH; }
f64 inchesToMm(f64 inches) { return inches * MM_PER_INCH; }
f64 feetToMm(f64 feet) { return feet * MM_PER_FOOT; }
f64 cmToMm(f64 cm) { return cm * MM_PER_CM; }
f64 mToMm(f64 m) { return m * MM_PER_M; }

// --- parseDimension ---

std::optional<f64> parseDimension(const std::string& input, bool isMetric) {
    std::string s = str::trim(input);
    if (s.empty()) return std::nullopt;

    // Detect and strip unit suffix (case-insensitive)
    // Check multi-char suffixes first, then single-char
    enum class Unit { None, Mm, Cm, M, Inch, Foot };
    Unit unit = Unit::None;

    // Build lowercase copy for suffix matching
    std::string lower = str::toLower(s);

    if (lower.size() >= 2 && lower.substr(lower.size() - 2) == "mm") {
        unit = Unit::Mm;
        s = s.substr(0, s.size() - 2);
    } else if (lower.size() >= 2 && lower.substr(lower.size() - 2) == "cm") {
        unit = Unit::Cm;
        s = s.substr(0, s.size() - 2);
    } else if (s.back() == '"') {
        unit = Unit::Inch;
        s = s.substr(0, s.size() - 1);
    } else if (s.back() == '\'') {
        unit = Unit::Foot;
        s = s.substr(0, s.size() - 1);
    } else if (lower.back() == 'm') {
        unit = Unit::M;
        s = s.substr(0, s.size() - 1);
    }

    // Trim remaining whitespace between number and suffix
    s = str::trim(s);
    if (s.empty()) return std::nullopt;

    // Parse the numeric part
    f64 value = 0.0;
    try {
        std::size_t pos = 0;
        value = std::stod(s, &pos);
        // Ensure entire remaining string was consumed
        if (pos != s.size()) return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }

    // Convert to mm based on unit
    switch (unit) {
    case Unit::Mm:   return value;
    case Unit::Cm:   return cmToMm(value);
    case Unit::M:    return mToMm(value);
    case Unit::Inch: return inchesToMm(value);
    case Unit::Foot: return feetToMm(value);
    case Unit::None:
        // Bare number: interpret based on project units
        if (isMetric) return value;           // mm
        return inchesToMm(value);             // inches -> mm
    }

    return std::nullopt; // unreachable
}

// --- formatDimension ---

// Helper: format a double, stripping trailing zeros but keeping at least one decimal if fractional
static std::string formatNum(f64 val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.4f", val);
    std::string s(buf);

    // Strip trailing zeros after decimal point
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        if (last == dot) {
            // All decimals are zero -- show as integer
            s = s.substr(0, dot);
        } else {
            s = s.substr(0, last + 1);
        }
    }
    return s;
}

std::string formatDimension(f64 valueMm, bool isMetric) {
    if (isMetric) {
        return formatNum(valueMm) + " mm";
    }

    // SAE: convert to inches
    f64 totalInches = mmToInches(valueMm);

    if (totalInches >= 12.0) {
        auto feet = static_cast<int>(totalInches / 12.0);
        f64 remainInches = totalInches - static_cast<f64>(feet) * 12.0;

        // Round to avoid floating point artifacts
        remainInches = std::round(remainInches * 10000.0) / 10000.0;

        if (remainInches < 0.0001) {
            // Exact feet
            return std::to_string(feet) + "'";
        }
        return std::to_string(feet) + "'" + formatNum(remainInches) + "\"";
    }

    // Under 12 inches
    return formatNum(totalInches) + "\"";
}

// --- formatDimensionCompact ---

std::string formatDimensionCompact(f64 valueMm, bool isMetric) {
    if (isMetric) {
        return formatNum(valueMm);
    }

    // SAE: same as formatDimension but no "mm" suffix needed — use feet-inches
    f64 totalInches = mmToInches(valueMm);

    if (totalInches >= 12.0) {
        auto feet = static_cast<int>(totalInches / 12.0);
        f64 remainInches = totalInches - static_cast<f64>(feet) * 12.0;
        remainInches = std::round(remainInches * 10000.0) / 10000.0;

        if (remainInches < 0.0001) {
            return std::to_string(feet) + "'";
        }
        return std::to_string(feet) + "'" + formatNum(remainInches) + "\"";
    }

    return formatNum(totalInches) + "\"";
}

} // namespace dw
