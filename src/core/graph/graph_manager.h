#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../database/database.h"
#include "../types.h"

namespace dw {

// GraphManager wraps GraphQLite extension lifecycle.
// GraphQLite adds Cypher query support to SQLite via sqlite3_load_extension().
// All graph data lives in the same SQLite database -- no separate graph DB.
class GraphManager {
  public:
    explicit GraphManager(Database& db);

    // Initialize: load extension + create graph schema
    // Returns false if extension not found (non-fatal, app works without)
    bool initialize(const Path& extensionDir);

    // Is GraphQLite loaded and available?
    bool isAvailable() const { return m_available; }

    // Query result type returned by queryCypher
    struct QueryResult {
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> rows;
    };

    // --- Node operations (non-fatal -- all return bool, log on failure) ---
    bool addModelNode(i64 id, const std::string& name, const std::string& hash);
    bool removeModelNode(i64 id);
    bool addCategoryNode(i64 id, const std::string& name);
    bool removeCategoryNode(i64 id);

    // --- Edge operations ---
    bool addBelongsToEdge(i64 modelId, i64 categoryId);
    bool removeBelongsToEdge(i64 modelId, i64 categoryId);

    // --- Relationship queries ---
    // Returns model IDs in a given project
    std::vector<i64> queryModelsInProject(i64 projectId);
    // Returns model IDs related to a given model (via RELATED_TO edges)
    std::vector<i64> queryRelatedModels(i64 modelId);

  private:
    // Escape single quotes in a Cypher string for safe SQL embedding
    std::string escapeCypher(const std::string& cypher) const;

    // Execute a Cypher query via GraphQLite's cypher() SQL function
    bool executeCypher(const std::string& cypher, std::string& error);

    // Query that returns results as vector of string vectors (rows x cols)
    std::optional<QueryResult> queryCypher(const std::string& cypher);

    // Graph schema initialization (creates node/edge types)
    bool initializeSchema();

    Database& m_db;
    bool m_available = false;
};

} // namespace dw
