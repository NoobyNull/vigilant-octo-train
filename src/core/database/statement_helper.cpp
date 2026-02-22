#include "statement_helper.h"

namespace dw {

StatementHelper::StatementHelper(Database& db) : m_db(db) {}

i64 StatementHelper::count(const std::string& query) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

std::optional<std::string> StatementHelper::queryText(const std::string& query,
                                                      int paramIndex,
                                                      const std::string& value) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(paramIndex, value)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return stmt.getText(0);
    }

    return std::nullopt;
}

std::optional<i64> StatementHelper::queryInt(const std::string& query, int paramIndex, i64 value) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(paramIndex, value)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return std::nullopt;
}

std::optional<f64> StatementHelper::queryDouble(const std::string& query,
                                                int paramIndex,
                                                f64 value) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindDouble(paramIndex, value)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return stmt.getDouble(0);
    }

    return std::nullopt;
}

bool StatementHelper::exists(const std::string& query, int paramIndex, const std::string& value) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(paramIndex, value)) {
        return false;
    }

    return stmt.step();
}

bool StatementHelper::exists(const std::string& query, int paramIndex, i64 value) {
    auto stmt = m_db.prepare(query);
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(paramIndex, value)) {
        return false;
    }

    return stmt.step();
}

} // namespace dw
