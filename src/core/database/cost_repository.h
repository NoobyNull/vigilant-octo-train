#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"
#include "database.h"

namespace dw {

// Cost item categories
enum class CostCategory { Material, Labor, Tool, Other };

// Single cost line item
struct CostItem {
    i64 id = 0;
    std::string name;
    CostCategory category = CostCategory::Material;
    f64 quantity = 1.0;
    f64 rate = 0.0;  // Unit price
    f64 total = 0.0; // quantity * rate
    std::string notes;
};

// Cost estimate document
struct CostEstimate {
    i64 id = 0;
    std::string name;
    i64 projectId = 0; // Optional link to project
    std::vector<CostItem> items;
    f64 subtotal = 0.0;
    f64 taxRate = 0.0; // Percentage (e.g., 8.0 for 8%)
    f64 taxAmount = 0.0;
    f64 discountRate = 0.0;
    f64 discountAmount = 0.0;
    f64 total = 0.0;
    std::string notes;
    std::string createdAt;
    std::string modifiedAt;

    // Recalculate totals from items
    void recalculate();
};

// Repository for cost estimate CRUD operations
class CostRepository {
  public:
    explicit CostRepository(Database& db);

    // Create
    std::optional<i64> insert(const CostEstimate& estimate);

    // Read
    std::optional<CostEstimate> findById(i64 id);
    std::vector<CostEstimate> findAll();
    std::vector<CostEstimate> findByProject(i64 projectId);

    // Update
    bool update(const CostEstimate& estimate);

    // Delete
    bool remove(i64 id);

    // Utility
    i64 count();

  private:
    CostEstimate rowToEstimate(Statement& stmt);
    std::string itemsToJson(const std::vector<CostItem>& items);
    std::vector<CostItem> jsonToItems(const std::string& json);
    std::string categoryToString(CostCategory cat);
    CostCategory stringToCategory(const std::string& str);

    Database& m_db;
};

} // namespace dw
