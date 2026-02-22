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

    // Execute a Cypher query via GraphQLite's cypher() SQL function
    // Returns true on success, error string on failure
    bool executeCypher(const std::string& cypher, std::string& error);

    // Query that returns results as vector of string vectors (rows x cols)
    struct QueryResult {
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> rows;
    };
    std::optional<QueryResult> queryCypher(const std::string& cypher);

    // Graph schema initialization (creates node/edge types)
    bool initializeSchema();

    // --- Node operations (non-fatal -- all return bool, log on failure) ---
    bool addModelNode(i64 id, const std::string& name, const std::string& hash);
    bool removeModelNode(i64 id);
    bool addCategoryNode(i64 id, const std::string& name);
    bool removeCategoryNode(i64 id);
    bool addProjectNode(i64 id, const std::string& name);
    bool removeProjectNode(i64 id);

    // --- Edge operations ---
    bool addBelongsToEdge(i64 modelId, i64 categoryId);
    bool removeBelongsToEdge(i64 modelId, i64 categoryId);
    bool addContainsEdge(i64 projectId, i64 modelId);
    bool removeContainsEdge(i64 projectId, i64 modelId);
    bool addRelatedToEdge(i64 modelId1, i64 modelId2);

    // --- Relationship queries ---
    // Returns model IDs in the same category as the given model
    std::vector<i64> queryModelsInSameCategory(i64 modelId);
    // Returns model IDs in a given project
    std::vector<i64> queryModelsInProject(i64 projectId);
    // Returns model IDs related to a given model (via RELATED_TO edges)
    std::vector<i64> queryRelatedModels(i64 modelId);
    // Returns category IDs a model belongs to
    std::vector<i64> queryModelCategories(i64 modelId);
    // Returns project IDs containing a model
    std::vector<i64> queryModelProjects(i64 modelId);
    // Returns models not in any project (orphans)
    std::vector<i64> queryOrphanModels();

  private:
    Database& m_db;
    bool m_available = false;
};

} // namespace dw
