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

  private:
    Database& m_db;
    bool m_available = false;
};

} // namespace dw
