#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../types.h"
#include "project_costing_io.h"

namespace dw {

struct RateCategory; // Forward declare -- no include needed for engine interface

// Aggregated cost totals for a category
struct CategoryTotal {
    std::string category;
    f64 estimatedTotal = 0.0;
    f64 actualTotal = 0.0;
    int entryCount = 0;
};

// Computes and manages project cost entries
class CostingEngine {
  public:
    CostingEngine() = default;

    // --- Entry management ---
    void setEntries(std::vector<CostingEntry> entries);
    const std::vector<CostingEntry>& entries() const;
    void addEntry(CostingEntry entry);
    void removeEntry(const std::string& entryId);
    CostingEntry* findEntry(const std::string& entryId);

    // --- Material snapshot ---
    // Create a material entry with price snapshot from a StockSize
    static CostingEntry createMaterialEntry(
        const std::string& name,
        i64 dbId,
        const std::string& dimensions,
        f64 priceAtCapture,
        f64 quantity,
        const std::string& unit);

    // --- Labor entry helper ---
    static CostingEntry createLaborEntry(
        const std::string& name,
        f64 hourlyRate,
        f64 estimatedHours);

    // --- Overhead entry helper ---
    static CostingEntry createOverheadEntry(
        const std::string& name,
        f64 amount,
        const std::string& notes = "");

    // --- Auto-populate from rate categories ---
    // Generates consumable/tooling entries from rate categories and project volume.
    // Replaces any existing auto-generated entries for the same rate category name.
    void applyRateCategories(const std::vector<RateCategory>& rates,
                             f64 projectVolumeCuUnits);

    // --- Totals ---
    void recalculate();  // Recompute estimatedTotal = quantity * unitRate for all entries
    f64 totalEstimated() const;
    f64 totalActual() const;
    f64 variance() const;  // totalActual - totalEstimated

    // Per-category subtotals
    std::vector<CategoryTotal> categoryTotals() const;

  private:
    std::vector<CostingEntry> m_entries;
    std::string generateId() const;  // Simple UUID-like string
};

} // namespace dw
