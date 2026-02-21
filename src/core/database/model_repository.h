#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../types.h"
#include "database.h"

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

    // Orientation data (NULL = not yet computed)
    std::optional<f32> orientYaw;
    std::optional<Mat4> orientMatrix;
};

// Repository for model CRUD operations
class ModelRepository {
  public:
    explicit ModelRepository(Database& db);

    // Create
    std::optional<i64> insert(const ModelRecord& model);

    // Read
    std::optional<ModelRecord> findById(i64 id);
    std::optional<ModelRecord> findByHash(std::string_view hash);
    std::vector<ModelRecord> findAll();
    std::vector<ModelRecord> findByName(std::string_view searchTerm);
    std::vector<ModelRecord> findByFormat(std::string_view format);
    std::vector<ModelRecord> findByTag(std::string_view tag);

    // Update
    bool update(const ModelRecord& model);
    bool updateThumbnail(i64 id, const Path& thumbnailPath);
    bool updateTags(i64 id, const std::vector<std::string>& tags);
    bool updateOrient(i64 id, f32 yaw, const Mat4& matrix);

    // Delete
    bool remove(i64 id);
    bool removeByHash(std::string_view hash);

    // Utility
    bool exists(std::string_view hash);
    i64 count();

  private:
    ModelRecord rowToModel(Statement& stmt);
    std::string tagsToJson(const std::vector<std::string>& tags);
    std::vector<std::string> jsonToTags(const std::string& json);
    static std::string mat4ToJson(const Mat4& m);
    static std::optional<Mat4> jsonToMat4(const std::string& json);

    Database& m_db;
};

} // namespace dw
