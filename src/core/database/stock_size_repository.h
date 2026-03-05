#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"
#include "database.h"

namespace dw {

// Stock size entry: a specific purchasable dimension of a material
struct StockSize {
    i64 id = 0;
    i64 materialId = 0;          // FK to materials table
    std::string name;             // Optional custom name (auto-generated if empty)
    f64 widthMm = 0.0;           // Nullable in DB (0.0 means not specified)
    f64 heightMm = 0.0;          // Nullable in DB (0.0 means not specified)
    f64 thicknessMm = 0.0;       // Always required
    f64 pricePerUnit = 0.0;
    std::string unitLabel;        // "sheet", "board foot", "linear foot", "each"
};

// Repository for stock size CRUD operations
class StockSizeRepository {
  public:
    explicit StockSizeRepository(Database& db);

    // Create
    std::optional<i64> insert(const StockSize& size);

    // Read
    std::optional<StockSize> findById(i64 id);
    std::vector<StockSize> findByMaterial(i64 materialId);
    std::vector<StockSize> findAll();

    // Update
    bool update(const StockSize& size);

    // Delete
    bool remove(i64 id);
    bool removeByMaterial(i64 materialId);

    // Utility
    i64 count();
    i64 countByMaterial(i64 materialId);

  private:
    StockSize rowToStockSize(Statement& stmt);
    Database& m_db;
};

} // namespace dw
