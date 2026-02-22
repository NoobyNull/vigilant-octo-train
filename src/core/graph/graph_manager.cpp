#include "graph_manager.h"

#include <sqlite3.h>

#include "../utils/log.h"

namespace dw {

GraphManager::GraphManager(Database& db) : m_db(db) {}

bool GraphManager::initialize(const Path& extensionDir) {
    // Enable extension loading
    if (!m_db.enableExtensionLoading()) {
        log::error("GraphManager", "Failed to enable extension loading");
        m_available = false;
        return false;
    }

    // Construct extension path (without file extension -- sqlite3_load_extension
    // auto-appends .so/.dll/.dylib based on platform)
    Path extPath = extensionDir / "graphqlite";
    std::string error;

    if (!m_db.loadExtension(extPath.string(), error)) {
        log::warningf("GraphManager",
                      "GraphQLite extension not found at %s -- graph queries disabled (%s)",
                      extPath.string().c_str(), error.c_str());
        m_available = false;

        // Disable extension loading for security even on failure
        m_db.disableExtensionLoading();
        return false;
    }

    log::info("GraphManager", "GraphQLite extension loaded successfully");

    // Disable extension loading for security now that we're done
    m_db.disableExtensionLoading();

    m_available = true;

    // Initialize graph schema
    if (!initializeSchema()) {
        log::warning("GraphManager", "Failed to initialize graph schema");
        // Extension is loaded but schema failed -- still mark as available
        // so caller can decide what to do
    }

    return true;
}

bool GraphManager::initializeSchema() {
    if (!m_available) {
        return false;
    }

    std::string error;

    // Create node tables for core entities
    // Note: The exact Cypher DDL depends on GraphQLite's implementation.
    // These use standard Cypher CREATE syntax; adapt if GraphQLite differs.
    const char* schemaDDL[] = {
        "CREATE NODE TABLE IF NOT EXISTS Model(id INT64, name STRING, hash STRING, PRIMARY KEY(id))",
        "CREATE NODE TABLE IF NOT EXISTS Category(id INT64, name STRING, PRIMARY KEY(id))",
        "CREATE NODE TABLE IF NOT EXISTS Project(id INT64, name STRING, PRIMARY KEY(id))",
        "CREATE REL TABLE IF NOT EXISTS BELONGS_TO(FROM Model TO Category)",
        "CREATE REL TABLE IF NOT EXISTS CONTAINS(FROM Project TO Model)",
        "CREATE REL TABLE IF NOT EXISTS RELATED_TO(FROM Model TO Model)",
    };

    for (const auto& ddl : schemaDDL) {
        if (!executeCypher(ddl, error)) {
            log::warningf("GraphManager", "Schema DDL failed: %s -- %s", ddl, error.c_str());
            return false;
        }
    }

    log::info("GraphManager", "Graph schema initialized");
    return true;
}

bool GraphManager::executeCypher(const std::string& cypher, std::string& error) {
    if (!m_available) {
        error = "GraphQLite not available";
        return false;
    }

    // GraphQLite exposes a cypher() SQL function.
    // Escape single quotes in the Cypher string for SQL embedding.
    std::string escaped;
    escaped.reserve(cypher.size());
    for (char c : cypher) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }

    std::string sql = "SELECT cypher('" + escaped + "')";
    if (!m_db.execute(sql)) {
        error = m_db.lastError();
        return false;
    }

    return true;
}

std::optional<GraphManager::QueryResult> GraphManager::queryCypher(const std::string& cypher) {
    if (!m_available) {
        return std::nullopt;
    }

    // Escape single quotes
    std::string escaped;
    escaped.reserve(cypher.size());
    for (char c : cypher) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }

    std::string sql = "SELECT * FROM cypher('" + escaped + "')";
    auto stmt = m_db.prepare(sql);
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    QueryResult result;

    // Get column info from first step
    bool firstRow = true;
    while (stmt.step()) {
        if (firstRow) {
            int colCount = stmt.columnCount();
            for (int i = 0; i < colCount; ++i) {
                result.columns.push_back(stmt.columnName(i));
            }
            firstRow = false;
        }

        std::vector<std::string> row;
        for (int i = 0; i < stmt.columnCount(); ++i) {
            row.push_back(stmt.getText(i));
        }
        result.rows.push_back(std::move(row));
    }

    // If no rows were returned, still populate columns if possible
    if (firstRow) {
        int colCount = stmt.columnCount();
        for (int i = 0; i < colCount; ++i) {
            result.columns.push_back(stmt.columnName(i));
        }
    }

    return result;
}

} // namespace dw
