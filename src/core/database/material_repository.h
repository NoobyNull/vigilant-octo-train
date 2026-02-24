#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../materials/material.h"
#include "../types.h"
#include "database.h"

namespace dw {

// Repository for material CRUD operations
class MaterialRepository {
  public:
    explicit MaterialRepository(Database& db);

    // Create
    std::optional<i64> insert(const MaterialRecord& material);

    // Read
    std::optional<MaterialRecord> findById(i64 id);
    std::vector<MaterialRecord> findAll();
    std::vector<MaterialRecord> findByCategory(MaterialCategory category);
    std::vector<MaterialRecord> findByName(std::string_view searchTerm);
    std::optional<MaterialRecord> findByExactName(const std::string& name);

    // Update
    bool update(const MaterialRecord& material);

    // Delete
    bool remove(i64 id);

    // Utility
    i64 count();

  private:
    static MaterialRecord rowToMaterial(Statement& stmt);

    Database& m_db;
};

} // namespace dw
