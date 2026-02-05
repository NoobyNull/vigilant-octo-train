#pragma once

#include "../database/database.h"
#include "../database/model_repository.h"
#include "../mesh/mesh.h"
#include "../types.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dw {

// Forward declaration
class ThumbnailGenerator;

// Import result
struct ImportResult {
    bool success = false;
    i64 modelId = 0;
    std::string error;
    bool isDuplicate = false;
};

// Library manager - handles model library operations
class LibraryManager {
public:
    explicit LibraryManager(Database& db);

    // Import a model file into the library
    ImportResult importModel(const Path& sourcePath);

    // Get all models in library
    std::vector<ModelRecord> getAllModels();

    // Search models by name
    std::vector<ModelRecord> searchModels(const std::string& query);

    // Filter by format
    std::vector<ModelRecord> filterByFormat(const std::string& format);

    // Filter by tag
    std::vector<ModelRecord> filterByTag(const std::string& tag);

    // Get a single model record
    std::optional<ModelRecord> getModel(i64 modelId);

    // Get model by hash
    std::optional<ModelRecord> getModelByHash(const std::string& hash);

    // Load mesh for a model
    MeshPtr loadMesh(i64 modelId);
    MeshPtr loadMesh(const ModelRecord& record);

    // Update model metadata
    bool updateModel(const ModelRecord& record);
    bool updateTags(i64 modelId, const std::vector<std::string>& tags);

    // Remove model from library
    bool removeModel(i64 modelId);

    // Get model count
    i64 modelCount();

    // Check if model exists by hash
    bool modelExists(const std::string& hash);

    // Set callback for duplicate handling
    using DuplicateHandler = std::function<bool(const ModelRecord& existing)>;
    void setDuplicateHandler(DuplicateHandler handler) { m_duplicateHandler = handler; }

    // Set thumbnail generator (optional, owned externally)
    void setThumbnailGenerator(ThumbnailGenerator* generator) { m_thumbnailGen = generator; }

private:
    std::string computeFileHash(const Path& path);
    bool generateThumbnail(i64 modelId, const Mesh& mesh);

    Database& m_db;
    ModelRepository m_modelRepo;
    DuplicateHandler m_duplicateHandler;
    ThumbnailGenerator* m_thumbnailGen = nullptr;
};

}  // namespace dw
