#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../optimizer/sheet.h"
#include "../types.h"

namespace dw {
class Database;
class Statement;

struct CutPlanRecord {
    i64 id = 0;
    std::optional<i64> projectId;
    std::string name;
    std::string algorithm; // "guillotine" or "first_fit_decreasing"
    // Serialized JSON
    std::string sheetConfigJson;
    std::string partsJson;
    std::string resultJson;
    bool allowRotation = true;
    f32 kerf = 0.0f;
    f32 margin = 0.0f;
    int sheetsUsed = 0;
    f32 efficiency = 0.0f;
    std::string createdAt;
    std::string modifiedAt;
};

class CutPlanRepository {
  public:
    explicit CutPlanRepository(Database& db);

    std::optional<i64> insert(const CutPlanRecord& record);
    std::optional<CutPlanRecord> findById(i64 id);
    std::vector<CutPlanRecord> findAll();
    std::vector<CutPlanRecord> findByProject(i64 projectId);
    bool update(const CutPlanRecord& record);
    bool remove(i64 id);
    i64 count();

    // Serialization helpers
    static std::string sheetToJson(const optimizer::Sheet& sheet);
    static optimizer::Sheet jsonToSheet(const std::string& json);
    static std::string partsToJson(const std::vector<optimizer::Part>& parts);
    static std::vector<optimizer::Part> jsonToParts(const std::string& json);
    static std::string cutPlanToJson(const optimizer::CutPlan& plan);
    static optimizer::CutPlan jsonToCutPlan(const std::string& json);

  private:
    CutPlanRecord rowToRecord(Statement& stmt);
    Database& m_db;
};
} // namespace dw
