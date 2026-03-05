#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"
#include "database.h"

namespace dw {

// Rate category: a volume-scaled cost item (e.g., "Finishing supplies: $3/cu unit")
// projectId=0 means global default; projectId>0 is a per-project override
struct RateCategory {
    i64 id = 0;
    std::string name;              // User-defined name, e.g. "Finishing supplies"
    f64 ratePerCuUnit = 0.0;       // Cost per cubic unit of project bounding volume
    i64 projectId = 0;             // 0 = global default, >0 = project-specific override
    std::string notes;             // Optional user notes

    // Compute cost for a given volume
    f64 computeCost(f64 volumeCuUnits) const { return ratePerCuUnit * volumeCuUnits; }
};

// Repository for rate category CRUD operations
class RateCategoryRepository {
  public:
    explicit RateCategoryRepository(Database& db);

    // Create
    std::optional<i64> insert(const RateCategory& cat);

    // Read
    std::optional<RateCategory> findById(i64 id);
    std::vector<RateCategory> findGlobal();               // where projectId=0
    std::vector<RateCategory> findByProject(i64 projectId); // where projectId=?

    // Effective rates: global defaults UNLESS overridden by project-specific rate
    // with the same name. Project-only rates (no global counterpart) also included.
    std::vector<RateCategory> getEffectiveRates(i64 projectId);

    // Update
    bool update(const RateCategory& cat);

    // Delete
    bool remove(i64 id);
    bool removeByProject(i64 projectId);

    // Utility
    i64 count();

  private:
    RateCategory rowToRateCategory(Statement& stmt);
    Database& m_db;
};

} // namespace dw
