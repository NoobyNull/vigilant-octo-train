#include "cut_plan_repository.h"

#include <sstream>

#include "../utils/log.h"
#include "../utils/string_utils.h"
#include "database.h"

namespace dw {

CutPlanRepository::CutPlanRepository(Database& db) : m_db(db) {}

std::optional<i64> CutPlanRepository::insert(const CutPlanRecord& record) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO cut_plans (
            project_id, name, algorithm, sheet_config, parts, result,
            allow_rotation, kerf, margin, sheets_used, efficiency
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    bool bindOk = true;
    if (record.projectId.has_value()) {
        bindOk = stmt.bindInt(1, record.projectId.value());
    } else {
        bindOk = stmt.bindNull(1);
    }
    bindOk = bindOk && stmt.bindText(2, record.name) && stmt.bindText(3, record.algorithm) &&
             stmt.bindText(4, record.sheetConfigJson) && stmt.bindText(5, record.partsJson) &&
             stmt.bindText(6, record.resultJson) &&
             stmt.bindInt(7, record.allowRotation ? 1 : 0) &&
             stmt.bindDouble(8, static_cast<f64>(record.kerf)) &&
             stmt.bindDouble(9, static_cast<f64>(record.margin)) &&
             stmt.bindInt(10, record.sheetsUsed) &&
             stmt.bindDouble(11, static_cast<f64>(record.efficiency));

    if (!bindOk) {
        log::error("CutPlanRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("CutPlanRepo", "Failed to insert cut plan: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<CutPlanRecord> CutPlanRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM cut_plans WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToRecord(stmt);
    }

    return std::nullopt;
}

std::vector<CutPlanRecord> CutPlanRepository::findAll() {
    std::vector<CutPlanRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM cut_plans ORDER BY modified_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToRecord(stmt));
    }

    return results;
}

std::vector<CutPlanRecord> CutPlanRepository::findByProject(i64 projectId) {
    std::vector<CutPlanRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM cut_plans WHERE project_id = ? ORDER BY created_at");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, projectId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToRecord(stmt));
    }

    return results;
}

bool CutPlanRepository::update(const CutPlanRecord& record) {
    auto stmt = m_db.prepare(R"(
        UPDATE cut_plans SET
            project_id = ?,
            name = ?,
            algorithm = ?,
            sheet_config = ?,
            parts = ?,
            result = ?,
            allow_rotation = ?,
            kerf = ?,
            margin = ?,
            sheets_used = ?,
            efficiency = ?,
            modified_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    bool bindOk = true;
    if (record.projectId.has_value()) {
        bindOk = stmt.bindInt(1, record.projectId.value());
    } else {
        bindOk = stmt.bindNull(1);
    }
    bindOk = bindOk && stmt.bindText(2, record.name) && stmt.bindText(3, record.algorithm) &&
             stmt.bindText(4, record.sheetConfigJson) && stmt.bindText(5, record.partsJson) &&
             stmt.bindText(6, record.resultJson) &&
             stmt.bindInt(7, record.allowRotation ? 1 : 0) &&
             stmt.bindDouble(8, static_cast<f64>(record.kerf)) &&
             stmt.bindDouble(9, static_cast<f64>(record.margin)) &&
             stmt.bindInt(10, record.sheetsUsed) &&
             stmt.bindDouble(11, static_cast<f64>(record.efficiency)) &&
             stmt.bindInt(12, record.id);

    if (!bindOk) {
        return false;
    }

    return stmt.execute();
}

bool CutPlanRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM cut_plans WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }
    return stmt.execute();
}

i64 CutPlanRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM cut_plans");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

CutPlanRecord CutPlanRepository::rowToRecord(Statement& stmt) {
    CutPlanRecord record;
    record.id = stmt.getInt(0);
    record.projectId = stmt.isNull(1) ? std::nullopt : std::optional<i64>(stmt.getInt(1));
    record.name = stmt.getText(2);
    record.algorithm = stmt.getText(3);
    record.sheetConfigJson = stmt.getText(4);
    record.partsJson = stmt.getText(5);
    record.resultJson = stmt.getText(6);
    record.allowRotation = stmt.getInt(7) != 0;
    record.kerf = static_cast<f32>(stmt.getDouble(8));
    record.margin = static_cast<f32>(stmt.getDouble(9));
    record.sheetsUsed = static_cast<int>(stmt.getInt(10));
    record.efficiency = static_cast<f32>(stmt.getDouble(11));
    record.createdAt = stmt.getText(12);
    record.modifiedAt = stmt.getText(13);
    return record;
}

// ===== JSON Serialization Helpers =====

std::string CutPlanRepository::sheetToJson(const optimizer::Sheet& sheet) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"width\":" << sheet.width << ",";
    ss << "\"height\":" << sheet.height << ",";
    ss << "\"cost\":" << sheet.cost << ",";
    ss << "\"quantity\":" << sheet.quantity << ",";
    ss << "\"name\":\"" << str::escapeJsonString(sheet.name) << "\"";
    ss << "}";
    return ss.str();
}

optimizer::Sheet CutPlanRepository::jsonToSheet(const std::string& json) {
    optimizer::Sheet sheet;

    if (json.size() < 2 || json[0] != '{') {
        return sheet;
    }

    auto parseDouble = [&json](const std::string& key) -> f64 {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return 0.0;
        size_t valStart = pos + search.length();
        size_t valEnd = json.find_first_of(",}", valStart);
        if (valEnd == std::string::npos) return 0.0;
        try {
            return std::stod(json.substr(valStart, valEnd - valStart));
        } catch (...) {
            return 0.0;
        }
    };

    auto parseString = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        size_t valStart = pos + search.length();
        size_t valEnd = json.find('"', valStart);
        if (valEnd == std::string::npos) return "";
        return json.substr(valStart, valEnd - valStart);
    };

    sheet.width = static_cast<f32>(parseDouble("width"));
    sheet.height = static_cast<f32>(parseDouble("height"));
    sheet.cost = static_cast<f32>(parseDouble("cost"));
    sheet.quantity = static_cast<int>(parseDouble("quantity"));
    sheet.name = parseString("name");

    return sheet;
}

std::string CutPlanRepository::partsToJson(const std::vector<optimizer::Part>& parts) {
    if (parts.empty()) {
        return "[]";
    }

    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        const auto& part = parts[i];
        ss << "{";
        ss << "\"id\":" << part.id << ",";
        ss << "\"name\":\"" << str::escapeJsonString(part.name) << "\",";
        ss << "\"width\":" << part.width << ",";
        ss << "\"height\":" << part.height << ",";
        ss << "\"quantity\":" << part.quantity;
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

std::vector<optimizer::Part> CutPlanRepository::jsonToParts(const std::string& json) {
    std::vector<optimizer::Part> parts;

    if (json.size() < 2 || json[0] != '[') {
        return parts;
    }

    size_t pos = 1;
    while (pos < json.size()) {
        size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos) break;

        // Find matching closing brace
        size_t objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos) break;

        std::string obj = json.substr(objStart + 1, objEnd - objStart - 1);
        optimizer::Part part;

        auto parseDouble = [&obj](const std::string& key) -> f64 {
            std::string search = "\"" + key + "\":";
            size_t keyPos = obj.find(search);
            if (keyPos == std::string::npos) return 0.0;
            size_t valStart = keyPos + search.length();
            size_t valEnd = obj.find_first_of(",}", valStart);
            if (valEnd == std::string::npos) valEnd = obj.size();
            try {
                return std::stod(obj.substr(valStart, valEnd - valStart));
            } catch (...) {
                return 0.0;
            }
        };

        auto parseString = [&obj](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":\"";
            size_t keyPos = obj.find(search);
            if (keyPos == std::string::npos) return "";
            size_t valStart = keyPos + search.length();
            size_t valEnd = obj.find('"', valStart);
            if (valEnd == std::string::npos) return "";
            return obj.substr(valStart, valEnd - valStart);
        };

        part.id = static_cast<i64>(parseDouble("id"));
        part.name = parseString("name");
        part.width = static_cast<f32>(parseDouble("width"));
        part.height = static_cast<f32>(parseDouble("height"));
        part.quantity = static_cast<int>(parseDouble("quantity"));

        parts.push_back(part);
        pos = objEnd + 1;
    }

    return parts;
}

std::string CutPlanRepository::cutPlanToJson(const optimizer::CutPlan& plan) {
    std::ostringstream ss;
    ss << "{";

    // Sheets array with placements
    ss << "\"sheets\":[";
    for (size_t i = 0; i < plan.sheets.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& sr = plan.sheets[i];
        ss << "{\"sheetIndex\":" << sr.sheetIndex;
        ss << ",\"usedArea\":" << sr.usedArea;
        ss << ",\"wasteArea\":" << sr.wasteArea;
        ss << ",\"placements\":[";
        for (size_t j = 0; j < sr.placements.size(); ++j) {
            if (j > 0) ss << ",";
            const auto& p = sr.placements[j];
            ss << "{\"partIndex\":" << p.partIndex;
            ss << ",\"instanceIndex\":" << p.instanceIndex;
            ss << ",\"x\":" << p.x;
            ss << ",\"y\":" << p.y;
            ss << ",\"rotated\":" << (p.rotated ? "true" : "false");
            ss << "}";
        }
        ss << "]}";
    }
    ss << "],";

    // Unplaced parts
    ss << "\"unplacedParts\":[";
    for (size_t i = 0; i < plan.unplacedParts.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& part = plan.unplacedParts[i];
        ss << "{\"id\":" << part.id;
        ss << ",\"name\":\"" << str::escapeJsonString(part.name) << "\"";
        ss << ",\"width\":" << part.width;
        ss << ",\"height\":" << part.height;
        ss << ",\"quantity\":" << part.quantity;
        ss << "}";
    }
    ss << "],";

    ss << "\"totalUsedArea\":" << plan.totalUsedArea << ",";
    ss << "\"totalWasteArea\":" << plan.totalWasteArea << ",";
    ss << "\"totalCost\":" << plan.totalCost << ",";
    ss << "\"sheetsUsed\":" << plan.sheetsUsed;

    ss << "}";
    return ss.str();
}

optimizer::CutPlan CutPlanRepository::jsonToCutPlan(const std::string& json) {
    optimizer::CutPlan plan;

    if (json.size() < 2 || json[0] != '{') {
        return plan;
    }

    // Parse top-level numeric fields
    auto parseTopDouble = [&json](const std::string& key) -> f64 {
        std::string search = "\"" + key + "\":";
        size_t pos = json.rfind(search);
        if (pos == std::string::npos) return 0.0;
        size_t valStart = pos + search.length();
        size_t valEnd = json.find_first_of(",}", valStart);
        if (valEnd == std::string::npos) return 0.0;
        try {
            return std::stod(json.substr(valStart, valEnd - valStart));
        } catch (...) {
            return 0.0;
        }
    };

    plan.totalUsedArea = static_cast<f32>(parseTopDouble("totalUsedArea"));
    plan.totalWasteArea = static_cast<f32>(parseTopDouble("totalWasteArea"));
    plan.totalCost = static_cast<f32>(parseTopDouble("totalCost"));
    plan.sheetsUsed = static_cast<int>(parseTopDouble("sheetsUsed"));

    // Parse unplacedParts array
    std::string upKey = "\"unplacedParts\":[";
    size_t upPos = json.find(upKey);
    if (upPos != std::string::npos) {
        size_t arrStart = upPos + upKey.length();
        // Find matching ]
        int depth = 1;
        size_t arrEnd = arrStart;
        while (arrEnd < json.size() && depth > 0) {
            if (json[arrEnd] == '[') ++depth;
            if (json[arrEnd] == ']') --depth;
            if (depth > 0) ++arrEnd;
        }
        std::string arr = json.substr(arrStart, arrEnd - arrStart);
        plan.unplacedParts = jsonToParts("[" + arr + "]");
    }

    return plan;
}

} // namespace dw
