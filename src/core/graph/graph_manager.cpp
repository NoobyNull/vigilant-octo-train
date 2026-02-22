#include "graph_manager.h"

#include <nlohmann/json.hpp>
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
                      extPath.string().c_str(),
                      error.c_str());
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

    // GraphQLite uses implicit schema — labels are created on first use.
    // No DDL required; nodes and edges are created via CREATE/MERGE.
    log::info("GraphManager", "Graph schema initialized (implicit)");
    return true;
}

std::string GraphManager::escapeCypher(const std::string& cypher) const {
    std::string escaped;
    escaped.reserve(cypher.size());
    for (char c : cypher) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

bool GraphManager::executeCypher(const std::string& cypher, std::string& error) {
    if (!m_available) {
        error = "GraphQLite not available";
        return false;
    }

    std::string sql = "SELECT cypher('" + escapeCypher(cypher) + "')";
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

    // cypher() returns JSON; use json_each to iterate result rows
    std::string sql = "SELECT value FROM json_each(cypher('" + escapeCypher(cypher) + "'))";
    auto stmt = m_db.prepare(sql);
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    QueryResult result;
    bool firstRow = true;

    while (stmt.step()) {
        std::string jsonStr = stmt.getText(0);
        try {
            auto obj = nlohmann::json::parse(jsonStr);
            if (firstRow && obj.is_object()) {
                for (auto& [key, _] : obj.items()) {
                    result.columns.push_back(key);
                }
                firstRow = false;
            }
            std::vector<std::string> row;
            row.reserve(result.columns.size());
            for (const auto& col : result.columns) {
                if (obj.contains(col)) {
                    const auto& val = obj[col];
                    if (val.is_string()) {
                        row.push_back(val.get<std::string>());
                    } else {
                        row.push_back(val.dump());
                    }
                } else {
                    row.push_back("");
                }
            }
            result.rows.push_back(std::move(row));
        } catch (const nlohmann::json::exception& e) {
            log::warningf("GraphManager", "JSON parse error in query result: %s", e.what());
        }
    }

    return result;
}

// --- Node operations ---

bool GraphManager::addModelNode(i64 id, const std::string& name, const std::string& hash) {
    if (!m_available)
        return false;

    std::string cypher = "MERGE (m:Model {id: " + std::to_string(id) + "}) SET m.name = '" + name +
                         "', m.hash = '" + hash + "'";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "addModelNode failed for id %lld: %s",
                      static_cast<long long>(id),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeModelNode(i64 id) {
    if (!m_available)
        return false;

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(id) + "}) DETACH DELETE m";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "removeModelNode failed for id %lld: %s",
                      static_cast<long long>(id),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::addCategoryNode(i64 id, const std::string& name) {
    if (!m_available)
        return false;

    std::string cypher = "MERGE (c:Category {id: " + std::to_string(id) + "}) SET c.name = '" +
                         name + "'";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "addCategoryNode failed for id %lld: %s",
                      static_cast<long long>(id),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeCategoryNode(i64 id) {
    if (!m_available)
        return false;

    std::string cypher = "MATCH (c:Category {id: " + std::to_string(id) + "}) DETACH DELETE c";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "removeCategoryNode failed for id %lld: %s",
                      static_cast<long long>(id),
                      error.c_str());
        return false;
    }
    return true;
}

// --- Edge operations ---

bool GraphManager::addBelongsToEdge(i64 modelId, i64 categoryId) {
    if (!m_available)
        return false;

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(modelId) +
                         "}), (c:Category {id: " + std::to_string(categoryId) +
                         "}) MERGE (m)-[:BELONGS_TO]->(c)";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "addBelongsToEdge failed (%lld->%lld): %s",
                      static_cast<long long>(modelId),
                      static_cast<long long>(categoryId),
                      error.c_str());
        return false;
    }
    return true;
}

bool GraphManager::removeBelongsToEdge(i64 modelId, i64 categoryId) {
    if (!m_available)
        return false;

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(modelId) +
                         "})-[r:BELONGS_TO]->(c:Category {id: " + std::to_string(categoryId) +
                         "}) DELETE r";
    std::string error;
    if (!executeCypher(cypher, error)) {
        log::warningf("GraphManager",
                      "removeBelongsToEdge failed (%lld->%lld): %s",
                      static_cast<long long>(modelId),
                      static_cast<long long>(categoryId),
                      error.c_str());
        return false;
    }
    return true;
}

// --- Relationship queries ---

static std::vector<i64> extractIds(const std::optional<GraphManager::QueryResult>& result) {
    std::vector<i64> ids;
    if (!result)
        return ids;
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

std::vector<i64> GraphManager::queryModelsInProject(i64 projectId) {
    if (!m_available)
        return {};

    std::string cypher = "MATCH (p:Project {id: " + std::to_string(projectId) +
                         "})-[:CONTAINS]->(m:Model) RETURN m.id";
    return extractIds(queryCypher(cypher));
}

std::vector<i64> GraphManager::queryRelatedModels(i64 modelId) {
    if (!m_available)
        return {};

    std::string cypher = "MATCH (m:Model {id: " + std::to_string(modelId) +
                         "})-[:RELATED_TO]-(other:Model) RETURN other.id";
    return extractIds(queryCypher(cypher));
}

} // namespace dw
