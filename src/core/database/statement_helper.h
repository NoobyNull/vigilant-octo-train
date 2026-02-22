#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "database.h"

namespace dw {

// Helper class for executing SQL statements with named column mappings
// Reduces boilerplate by automating bind/step/get patterns
class StatementHelper {
  public:
    explicit StatementHelper(Database& db);

    // Execute a query and map results to objects
    // Returns list of objects created by mapper function
    template <typename T>
    std::vector<T> findAll(const std::string& query, std::function<T(Statement&)> mapper) {
        std::vector<T> results;
        auto stmt = m_db.prepare(query);
        if (!stmt.isValid()) {
            return results;
        }

        while (stmt.step()) {
            results.push_back(mapper(stmt));
        }
        return results;
    }

    // Execute a query with one parameter
    template <typename T>
    std::optional<T> findOne(const std::string& query,
                             int paramIndex,
                             const std::string& value,
                             std::function<T(Statement&)> mapper) {
        auto stmt = m_db.prepare(query);
        if (!stmt.isValid()) {
            return std::nullopt;
        }

        if (!stmt.bindText(paramIndex, value)) {
            return std::nullopt;
        }

        if (stmt.step()) {
            return mapper(stmt);
        }
        return std::nullopt;
    }

    // Execute a query with one integer parameter
    template <typename T>
    std::optional<T> findOne(const std::string& query,
                             int paramIndex,
                             i64 value,
                             std::function<T(Statement&)> mapper) {
        auto stmt = m_db.prepare(query);
        if (!stmt.isValid()) {
            return std::nullopt;
        }

        if (!stmt.bindInt(paramIndex, value)) {
            return std::nullopt;
        }

        if (stmt.step()) {
            return mapper(stmt);
        }
        return std::nullopt;
    }

    // Execute simple count query
    i64 count(const std::string& query);

    // Execute a query that returns a single text value
    std::optional<std::string> queryText(const std::string& query,
                                         int paramIndex,
                                         const std::string& value);

    // Execute a query that returns a single integer value
    std::optional<i64> queryInt(const std::string& query, int paramIndex, i64 value);

    // Execute a query that returns a single double value
    std::optional<f64> queryDouble(const std::string& query, int paramIndex, f64 value);

    // Check if a row exists
    bool exists(const std::string& query, int paramIndex, const std::string& value);
    bool exists(const std::string& query, int paramIndex, i64 value);

  private:
    Database& m_db;
};

} // namespace dw
