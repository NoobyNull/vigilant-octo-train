#pragma once

#include <string>
#include <vector>

#include "../types.h"

namespace dw {
namespace optimizer {

// A rectangular part to be cut
struct Part {
    i64 id = 0; // Reference to model/source
    std::string name;
    f32 width = 0.0f;  // X dimension
    f32 height = 0.0f; // Y dimension
    int quantity = 1;  // How many needed

    Part() = default;
    Part(f32 w, f32 h, int qty = 1) : width(w), height(h), quantity(qty) {}
    Part(i64 id_, const std::string& name_, f32 w, f32 h, int qty = 1)
        : id(id_), name(name_), width(w), height(h), quantity(qty) {}

    f32 area() const { return width * height; }
};

// A sheet of material
struct Sheet {
    f32 width = 0.0f;
    f32 height = 0.0f;
    f32 cost = 0.0f;  // Optional cost per sheet
    int quantity = 0; // Available quantity (0 = unlimited)
    std::string name; // e.g., "4x8 Plywood"

    Sheet() = default;
    Sheet(f32 w, f32 h) : width(w), height(h) {}
    Sheet(f32 w, f32 h, f32 c) : width(w), height(h), cost(c) {}

    f32 area() const { return width * height; }
};

// Placement of a part on a sheet
struct Placement {
    const Part* part = nullptr;
    int partIndex = 0;     // Index in original parts list
    int instanceIndex = 0; // Which instance (for quantity > 1)
    f32 x = 0.0f;          // Position on sheet
    f32 y = 0.0f;
    bool rotated = false; // 90 degree rotation

    f32 getWidth() const { return rotated ? part->height : part->width; }
    f32 getHeight() const { return rotated ? part->width : part->height; }
};

// Result for a single sheet
struct SheetResult {
    int sheetIndex = 0;
    std::vector<Placement> placements;
    f32 usedArea = 0.0f;
    f32 wasteArea = 0.0f;

    f32 efficiency() const {
        f32 total = usedArea + wasteArea;
        return total > 0.0f ? usedArea / total : 0.0f;
    }
};

// Complete cut plan result
struct CutPlan {
    std::vector<SheetResult> sheets;
    std::vector<Part> unplacedParts; // Parts that couldn't fit

    f32 totalUsedArea = 0.0f;
    f32 totalWasteArea = 0.0f;
    f32 totalCost = 0.0f;
    int sheetsUsed = 0;

    f32 overallEfficiency() const {
        f32 total = totalUsedArea + totalWasteArea;
        return total > 0.0f ? totalUsedArea / total : 0.0f;
    }

    bool isComplete() const { return unplacedParts.empty(); }
};

} // namespace optimizer
} // namespace dw
