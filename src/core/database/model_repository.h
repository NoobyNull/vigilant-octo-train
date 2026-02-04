#pragma once

#include "../types.h"
#include "database.h"

#include <optional>
#include <string>
#include <vector>

namespace dw {

// Model data structure
struct ModelRecord {
    i64 id = 0;
    std::string hash;
    std::string name;
    Path filePath;
    std::string fileFormat;
    u64 fileSize = 0;
    u32 vertexCount = 0;
    u32 triangleCount = 0;
    Vec3 boundsMin;
    Vec3 boundsMax;
    Path thumbnailPath;
    std::string importedAt;
    std::vector<std::string> tags;
};

// Repository for model CRUD operations
class ModelRepository {
public:
    explicit ModelRepository(Database& db);

    // Create
    std::optional<i64> insert(const ModelRecord& model);

    // Read
    std::optional<ModelRecord> findById(i64 id);
    std::optional<ModelRecord> findByHash(const std::string& hash);
    std::vector<ModelRecord> findAll();
    std::vector<ModelRecord> findByName(const std::string& searchTerm);
    std::vector<ModelRecord> findByFormat(const std::string& format);
    std::vector<ModelRecord> findByTag(const std::string& tag);

    // Update
    bool update(const ModelRecord& model);
    bool updateThumbnail(i64 id, const Path& thumbnailPath);
    bool updateTags(i64 id, const std::vector<std::string>& tags);

    // Delete
    bool remove(i64 id);
    bool removeByHash(const std::string& hash);

    // Utility
    bool exists(const std::string& hash);
    i64 count();

private:
    ModelRecord rowToModel(Statement& stmt);
    std::string tagsToJson(const std::vector<std::string>& tags);
    std::vector<std::string> jsonToTags(const std::string& json);

    Database& m_db;
};

}  // namespace dw
