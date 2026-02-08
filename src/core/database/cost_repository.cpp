#include "cost_repository.h"

#include <sstream>

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

void CostEstimate::recalculate() {
    subtotal = 0.0;
    for (auto& item : items) {
        item.total = item.quantity * item.rate;
        subtotal += item.total;
    }
    taxAmount = subtotal * (taxRate / 100.0);
    discountAmount = subtotal * (discountRate / 100.0);
    total = subtotal + taxAmount - discountAmount;
}

CostRepository::CostRepository(Database& db) : m_db(db) {}

std::optional<i64> CostRepository::insert(const CostEstimate& estimate) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO cost_estimates (
            name, project_id, items, subtotal, tax_rate, tax_amount,
            discount_rate, discount_amount, total, notes
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    bool bindOk = stmt.bindText(1, estimate.name);
    if (estimate.projectId > 0) {
        bindOk = bindOk && stmt.bindInt(2, estimate.projectId);
    } else {
        bindOk = bindOk && stmt.bindNull(2);
    }
    bindOk = bindOk && stmt.bindText(3, itemsToJson(estimate.items)) &&
             stmt.bindDouble(4, estimate.subtotal) && stmt.bindDouble(5, estimate.taxRate) &&
             stmt.bindDouble(6, estimate.taxAmount) && stmt.bindDouble(7, estimate.discountRate) &&
             stmt.bindDouble(8, estimate.discountAmount) && stmt.bindDouble(9, estimate.total) &&
             stmt.bindText(10, estimate.notes);

    if (!bindOk) {
        log::error("CostRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("CostRepo", "Failed to insert estimate: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<CostEstimate> CostRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM cost_estimates WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToEstimate(stmt);
    }

    return std::nullopt;
}

std::vector<CostEstimate> CostRepository::findAll() {
    std::vector<CostEstimate> results;

    auto stmt = m_db.prepare("SELECT * FROM cost_estimates ORDER BY modified_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToEstimate(stmt));
    }

    return results;
}

std::vector<CostEstimate> CostRepository::findByProject(i64 projectId) {
    std::vector<CostEstimate> results;

    auto stmt =
        m_db.prepare("SELECT * FROM cost_estimates WHERE project_id = ? ORDER BY modified_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, projectId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToEstimate(stmt));
    }

    return results;
}

bool CostRepository::update(const CostEstimate& estimate) {
    auto stmt = m_db.prepare(R"(
        UPDATE cost_estimates SET
            name = ?,
            project_id = ?,
            items = ?,
            subtotal = ?,
            tax_rate = ?,
            tax_amount = ?,
            discount_rate = ?,
            discount_amount = ?,
            total = ?,
            notes = ?,
            modified_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    bool bindOk = stmt.bindText(1, estimate.name);
    if (estimate.projectId > 0) {
        bindOk = bindOk && stmt.bindInt(2, estimate.projectId);
    } else {
        bindOk = bindOk && stmt.bindNull(2);
    }
    bindOk = bindOk && stmt.bindText(3, itemsToJson(estimate.items)) &&
             stmt.bindDouble(4, estimate.subtotal) && stmt.bindDouble(5, estimate.taxRate) &&
             stmt.bindDouble(6, estimate.taxAmount) && stmt.bindDouble(7, estimate.discountRate) &&
             stmt.bindDouble(8, estimate.discountAmount) && stmt.bindDouble(9, estimate.total) &&
             stmt.bindText(10, estimate.notes) && stmt.bindInt(11, estimate.id);

    if (!bindOk) {
        return false;
    }

    return stmt.execute();
}

bool CostRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM cost_estimates WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }
    return stmt.execute();
}

i64 CostRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM cost_estimates");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

CostEstimate CostRepository::rowToEstimate(Statement& stmt) {
    CostEstimate est;
    est.id = stmt.getInt(0);
    est.name = stmt.getText(1);
    est.projectId = stmt.isNull(2) ? 0 : stmt.getInt(2);
    est.items = jsonToItems(stmt.getText(3));
    est.subtotal = stmt.getDouble(4);
    est.taxRate = stmt.getDouble(5);
    est.taxAmount = stmt.getDouble(6);
    est.discountRate = stmt.getDouble(7);
    est.discountAmount = stmt.getDouble(8);
    est.total = stmt.getDouble(9);
    est.notes = stmt.getText(10);
    est.createdAt = stmt.getText(11);
    est.modifiedAt = stmt.getText(12);
    return est;
}

std::string CostRepository::categoryToString(CostCategory cat) {
    switch (cat) {
    case CostCategory::Material:
        return "material";
    case CostCategory::Labor:
        return "labor";
    case CostCategory::Tool:
        return "tool";
    case CostCategory::Other:
        return "other";
    }
    return "other";
}

CostCategory CostRepository::stringToCategory(const std::string& str) {
    if (str == "material")
        return CostCategory::Material;
    if (str == "labor")
        return CostCategory::Labor;
    if (str == "tool")
        return CostCategory::Tool;
    return CostCategory::Other;
}

std::string CostRepository::itemsToJson(const std::vector<CostItem>& items) {
    if (items.empty()) {
        return "[]";
    }

    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        const auto& item = items[i];
        ss << "{";
        ss << "\"name\":\"" << str::escapeJsonString(item.name) << "\",";
        ss << "\"category\":\"" << categoryToString(item.category) << "\",";
        ss << "\"quantity\":" << item.quantity << ",";
        ss << "\"rate\":" << item.rate << ",";
        ss << "\"total\":" << item.total << ",";
        ss << "\"notes\":\"" << str::escapeJsonString(item.notes) << "\"";
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

std::vector<CostItem> CostRepository::jsonToItems(const std::string& json) {
    std::vector<CostItem> items;

    if (json.size() < 2 || json[0] != '[') {
        return items;
    }

    // Simple JSON parsing for cost items
    size_t pos = 1;
    while (pos < json.size()) {
        size_t objStart = json.find('{', pos);
        if (objStart == std::string::npos)
            break;

        size_t objEnd = json.find('}', objStart);
        if (objEnd == std::string::npos)
            break;

        std::string obj = json.substr(objStart + 1, objEnd - objStart - 1);
        CostItem item;

        // Parse each field
        auto parseString = [&obj](const std::string& key) -> std::string {
            size_t keyPos = obj.find("\"" + key + "\":");
            if (keyPos == std::string::npos)
                return "";
            size_t valStart = obj.find('"', keyPos + key.length() + 3);
            if (valStart == std::string::npos)
                return "";
            size_t valEnd = obj.find('"', valStart + 1);
            if (valEnd == std::string::npos)
                return "";
            return obj.substr(valStart + 1, valEnd - valStart - 1);
        };

        auto parseDouble = [&obj](const std::string& key) -> f64 {
            size_t keyPos = obj.find("\"" + key + "\":");
            if (keyPos == std::string::npos)
                return 0.0;
            size_t valStart = keyPos + key.length() + 3;
            size_t valEnd = obj.find_first_of(",}", valStart);
            if (valEnd == std::string::npos)
                return 0.0;
            try {
                return std::stod(obj.substr(valStart, valEnd - valStart));
            } catch (...) {
                return 0.0;
            }
        };

        item.name = parseString("name");
        item.category = stringToCategory(parseString("category"));
        item.quantity = parseDouble("quantity");
        item.rate = parseDouble("rate");
        item.total = parseDouble("total");
        item.notes = parseString("notes");

        items.push_back(item);
        pos = objEnd + 1;
    }

    return items;
}

} // namespace dw
