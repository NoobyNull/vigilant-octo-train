#pragma once

#include <string>
#include <vector>

#include "../types.h"

namespace dw {

// Snapshot of a DB record at point of capture (fallback if DB regenerated)
struct CostingSnapshot {
    i64 dbId = 0;                    // Original DB record ID for live lookups
    std::string name;                // Snapshotted name
    std::string dimensions;          // Snapshotted "1220x2440x19mm" style string
    f64 priceAtCapture = 0.0;       // Price when added to project
};

// Cost category string constants
namespace CostCategory {
    constexpr const char* Material    = "material";
    constexpr const char* Tooling     = "tooling";
    constexpr const char* Consumable  = "consumable";
    constexpr const char* Labor       = "labor";
    constexpr const char* Overhead    = "overhead";
}

// Single cost line item in a project
struct CostingEntry {
    std::string id;                  // UUID string for stable identity
    std::string name;
    std::string category;            // "material", "tooling", "consumable", "labor", "overhead"
    f64 quantity = 0.0;
    f64 unitRate = 0.0;             // Per-unit price
    f64 estimatedTotal = 0.0;       // quantity * unitRate
    f64 actualTotal = 0.0;          // Manual entry from user
    std::string unit;                // "sheet", "board foot", "hour", "each", etc.
    std::string notes;
    CostingSnapshot snapshot;        // For material category: DB reference + snapshot
};

// Working estimate (editable, live prices for non-material)
struct CostingEstimate {
    std::string name;
    std::vector<CostingEntry> entries;
    f64 salePrice = 0.0;
    std::string notes;
    std::string createdAt;
    std::string modifiedAt;
};

// Finalized order (frozen values, locked)
struct CostingOrder {
    std::string name;
    std::vector<CostingEntry> entries;
    f64 salePrice = 0.0;
    std::string notes;
    std::string createdAt;
    std::string finalizedAt;
    std::string sourceEstimateName;  // Which estimate this was converted from
};

// I/O for project-level costing JSON files
class ProjectCostingIO {
  public:
    // Save/load estimates
    static bool saveEstimates(const Path& costingDir,
                              const std::vector<CostingEstimate>& estimates);
    static std::vector<CostingEstimate> loadEstimates(const Path& costingDir);

    // Save/load orders
    static bool saveOrders(const Path& costingDir, const std::vector<CostingOrder>& orders);
    static std::vector<CostingOrder> loadOrders(const Path& costingDir);

    // Convert estimate to order (freezes all values)
    static CostingOrder convertToOrder(const CostingEstimate& estimate);

    // Schema version
    static constexpr int SCHEMA_VERSION = 1;
};

} // namespace dw
