#pragma once

#include "../types.h"
#include "database.h"

#include <optional>
#include <string>
#include <vector>

namespace dw {

// Project data structure
struct ProjectRecord {
    i64 id = 0;
    std::string name;
    std::string description;
    Path filePath;
    std::string createdAt;
    std::string modifiedAt;
};

// Project-Model link
struct ProjectModelLink {
    i64 projectId = 0;
    i64 modelId = 0;
    int sortOrder = 0;
    std::string addedAt;
};

// Repository for project CRUD operations
class ProjectRepository {
public:
    explicit ProjectRepository(Database& db);

    // Create
    std::optional<i64> insert(const ProjectRecord& project);

    // Read
    std::optional<ProjectRecord> findById(i64 id);
    std::vector<ProjectRecord> findAll();
    std::vector<ProjectRecord> findByName(const std::string& searchTerm);

    // Update
    bool update(const ProjectRecord& project);
    bool updateModifiedTime(i64 id);

    // Delete
    bool remove(i64 id);

    // Project-Model relationships
    bool addModel(i64 projectId, i64 modelId, int sortOrder = 0);
    bool removeModel(i64 projectId, i64 modelId);
    bool updateModelOrder(i64 projectId, i64 modelId, int sortOrder);
    std::vector<i64> getModelIds(i64 projectId);
    std::vector<i64> getProjectsForModel(i64 modelId);
    bool hasModel(i64 projectId, i64 modelId);

    // Utility
    i64 count();

private:
    ProjectRecord rowToProject(Statement& stmt);

    Database& m_db;
};

}  // namespace dw
