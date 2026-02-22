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

// --- Node operations ---

bool GraphManager::addModelNode(i64 id, const std::string& name, const std::string& hash) {
    if (!m_available) return false;

    std::string cypher = "MERGE (m:Model {id: " + std::to_string(id) + "}) SET m.name = '" +
                         name + "', m.hash = '" + hash + "'";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addModelNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeModelNode(i64 id) {
    if (!m_available) return false;

    std::string cypher =
        "MATCH (m:Model {id: " + std::to_string(id) + "}) DETACH DELETE m";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "removeModelNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::addCategoryNode(i64 id, const std::string& name) {
    if (!m_available) return false;

    std::string cypher =
        "MERGE (c:Category {id: " + std::to_string(id) + "}) SET c.name = '" + name + "'";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addCategoryNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeCategoryNode(i64 id) {
    if (!m_available) return false;

    std::string cypher =
        "MATCH (c:Category {id: " + std::to_string(id) + "}) DETACH DELETE c";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "removeCategoryNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::addProjectNode(i64 id, const std::string& name) {
    if (!m_available) return false;

    std::string cypher =
        "MERGE (p:Project {id: " + std::to_string(id) + "}) SET p.name = '" + name + "'";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addProjectNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeProjectNode(i64 id) {
    if (!m_available) return false;

    std::string cypher =
        "MATCH (p:Project {id: " + std::to_string(id) + "}) DETACH DELETE p";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "removeProjectNode failed for id %lld: %s",
                      static_cast<long long>(id), error.c_str());
        return false;
    }
    return true;
}

// --- Edge operations ---

bool GraphManager::addBelongsToEdge(i64 modelId, i64 categoryId) {
    if (!m_available) return false;

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(modelId) +
                         "}), (c:Category {id: " + std::to_string(categoryId) +
                         "}) MERGE (m)-[:BELONGS_TO]->(c)";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addBelongsToEdge failed (%lld->%lld): %s",
                      static_cast<long long>(modelId), static_cast<long long>(categoryId),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeBelongsToEdge(i64 modelId, i64 categoryId) {
    if (!m_available) return false;

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(modelId) +
                         "})-[r:BELONGS_TO]->(c:Category {id: " + std::to_string(categoryId) +
                         "}) DELETE r";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "removeBelongsToEdge failed (%lld->%lld): %s",
                      static_cast<long long>(modelId), static_cast<long long>(categoryId),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::addContainsEdge(i64 projectId, i64 modelId) {
    if (!m_available) return false;

    std::string cypher = "MATCH (p:Project {id: " + std::to_string(projectId) +
                         "}), (m:Model {id: " + std::to_string(modelId) +
                         "}) MERGE (p)-[:CONTAINS]->(m)";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addContainsEdge failed (%lld->%lld): %s",
                      static_cast<long long>(projectId), static_cast<long long>(modelId),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeContainsEdge(i64 projectId, i64 modelId) {
    if (!m_available) return false;

    std::string cypher = "MATCH (p:Project {id: " + std::to_string(projectId) +
                         "})-[r:CONTAINS]->(m:Model {id: " + std::to_string(modelId) +
                         "}) DELETE r";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "removeContainsEdge failed (%lld->%lld): %s",
                      static_cast<long long>(projectId), static_cast<long long>(modelId),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::addRelatedToEdge(i64 modelId1, i64 modelId2) {
    if (!m_available) return false;

    std::string cypher = "MATCH (m1:Model {id: " + std::to_string(modelId1) +
                         "}), (m2:Model {id: " + std::to_string(modelId2) +
                         "}) MERGE (m1)-[:RELATED_TO]->(m2)";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager", "addRelatedToEdge failed (%lld->%lld): %s",
                      static_cast<long long>(modelId1), static_cast<long long>(modelId2),
                      error.c_str());
        return false;
    }
    return true;
}

// --- Relationship queries ---

static std::vector<i64> extractIds(const std::optional<GraphManager::QueryResult>& result) {
    std::vector<i64> ids;
    if (!result) return ids;
    for (const auto& row : result->rows) {
        if (!row.empty()) {
            try {
                ids.push_back(std::stoll(row[0]));
            } catch (...) {
                // Skip malformed rows
            }
        }
    }
    return ids;
}

std::vector<i64> GraphManager::queryModelsInSameCategory(i64 modelId) {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (m:Model {id: " + std::to_string(modelId) +
        "})-[:BELONGS_TO]->(c:Category)<-[:BELONGS_TO]-(other:Model) RETURN other.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryModelsInProject(i64 projectId) {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (p:Project {id: " + std::to_string(projectId) +
        "})-[:CONTAINS]->(m:Model) RETURN m.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryRelatedModels(i64 modelId) {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (m:Model {id: " + std::to_string(modelId) +
        "})-[:RELATED_TO]-(other:Model) RETURN other.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryModelCategories(i64 modelId) {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (m:Model {id: " + std::to_string(modelId) +
        "})-[:BELONGS_TO]->(c:Category) RETURN c.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryModelProjects(i64 modelId) {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (m:Model {id: " + std::to_string(modelId) +
        "})<-[:CONTAINS]-(p:Project) RETURN p.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryOrphanModels() {
    if (!m_available) return {};

    std::string cypher =
        "MATCH (m:Model) WHERE NOT EXISTS { MATCH (m)<-[:CONTAINS]-(:Project) } RETURN m.id";
    return extractIds(queryCypher(cypher));
}

} // namespace dw
