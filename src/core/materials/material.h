#pragma once

#include <string>
#include <string_view>

#include "../types.h"

namespace dw {

// Material category classification
enum class MaterialCategory {
    Hardwood,   // Traditional hardwood species (oak, maple, walnut, cherry, etc.)
    Softwood,   // Coniferous species (pine, cedar, fir, spruce, etc.)
    Domestic,   // Common North American woods not covered by hardwood/softwood categories
    Composite   // MDF, HDF, plywood, non-ferrous metals, plastics, foams
};

// Convert MaterialCategory to string representation
inline std::string materialCategoryToString(MaterialCategory category) {
    switch (category) {
        case MaterialCategory::Hardwood:
            return "hardwood";
        case MaterialCategory::Softwood:
            return "softwood";
        case MaterialCategory::Domestic:
            return "domestic";
        case MaterialCategory::Composite:
            return "composite";
        default:
            return "hardwood";
    }
}

// Convert string to MaterialCategory
inline MaterialCategory stringToMaterialCategory(std::string_view str) {
    if (str == "hardwood") {
        return MaterialCategory::Hardwood;
    }
    if (str == "softwood") {
        return MaterialCategory::Softwood;
    }
    if (str == "domestic") {
        return MaterialCategory::Domestic;
    }
    if (str == "composite") {
        return MaterialCategory::Composite;
    }
    return MaterialCategory::Hardwood; // Default fallback
}

// Material data structure
struct MaterialRecord {
    i64 id = 0;
    std::string name;
    MaterialCategory category = MaterialCategory::Hardwood;
    Path archivePath;
    f32 jankaHardness = 0.0f;         // Janka hardness rating (lbf)
    f32 feedRate = 0.0f;              // Feed rate (inches/min)
    f32 spindleSpeed = 0.0f;          // Spindle speed (RPM)
    f32 depthOfCut = 0.0f;            // Depth of cut (inches)
    f32 costPerBoardFoot = 0.0f;      // Cost per board foot (USD)
    f32 grainDirectionDeg = 0.0f;     // Grain direction (0-360 degrees)
    Path thumbnailPath;
    std::string importedAt;
};

} // namespace dw
