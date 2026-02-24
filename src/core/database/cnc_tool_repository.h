#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../cnc/cnc_tool.h"
#include "../types.h"
#include "database.h"

namespace dw {

// Repository for CNC tool CRUD operations
class CncToolRepository {
  public:
    explicit CncToolRepository(Database& db);

    // Tool CRUD
    std::optional<i64> insert(const CncToolRecord& tool);
    std::optional<CncToolRecord> findById(i64 id);
    std::vector<CncToolRecord> findAll();
    std::vector<CncToolRecord> findByType(CncToolType type);
    std::vector<CncToolRecord> findByName(std::string_view searchTerm);
    bool update(const CncToolRecord& tool);
    bool remove(i64 id);
    i64 count();

    // Junction CRUD (tool_material_params)
    std::optional<i64> insertParams(const ToolMaterialParams& params);
    std::optional<ToolMaterialParams> findParams(i64 toolId, i64 materialId);
    std::vector<ToolMaterialParams> findParamsForTool(i64 toolId);
    std::vector<ToolMaterialParams> findParamsForMaterial(i64 materialId);
    bool updateParams(const ToolMaterialParams& params);
    bool removeParams(i64 toolId, i64 materialId);

  private:
    static CncToolRecord rowToTool(Statement& stmt);
    static ToolMaterialParams rowToParams(Statement& stmt);

    Database& m_db;
};

} // namespace dw
