#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../database/database.h"
#include "../database/gcode_repository.h"
#include "../database/model_repository.h"
#include "../mesh/mesh.h"
#include "../types.h"

namespace dw {

// Forward declarations
class ThumbnailGenerator;
class Texture;
class GraphManager;

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

    // Set graph manager for dual-write (optional, owned externally)
    void setGraphManager(GraphManager* gm) { m_graphManager = gm; }

    // Generate thumbnail and update DB record
    bool generateThumbnail(i64 modelId, const Mesh& mesh, const Texture* materialTexture = nullptr,
                           float cameraPitch = 30.0f, float cameraYaw = 45.0f);

    // G-code operations
    std::vector<GCodeRecord> getAllGCodeFiles();
    std::vector<GCodeRecord> searchGCodeFiles(const std::string& query);
    std::optional<GCodeRecord> getGCodeFile(i64 id);
    bool deleteGCodeFile(i64 id);

    // Hierarchy operations
    std::optional<i64> createOperationGroup(i64 modelId, const std::string& name,
                                            int sortOrder = 0);
    std::vector<OperationGroup> getOperationGroups(i64 modelId);
    bool addGCodeToGroup(i64 groupId, i64 gcodeId, int sortOrder = 0);
    bool removeGCodeFromGroup(i64 groupId, i64 gcodeId);
    std::vector<GCodeRecord> getGroupGCodeFiles(i64 groupId);
    bool deleteOperationGroup(i64 groupId);

    // Template operations
    std::vector<GCodeTemplate> getTemplates();
    bool applyTemplate(i64 modelId, const std::string& templateName);

    // Auto-detect: try to match G-code filename to model name
    std::optional<i64> autoDetectModelMatch(const std::string& gcodeFilename);

    // --- Category management (delegates to ModelRepository + graph dual-write) ---
    bool assignCategory(i64 modelId, i64 categoryId);
    bool removeModelCategory(i64 modelId, i64 categoryId);
    std::optional<i64> createCategory(const std::string& name,
                                      std::optional<i64> parentId = std::nullopt);
    bool deleteCategory(i64 categoryId);
    std::vector<CategoryRecord> getAllCategories();
    std::vector<CategoryRecord> getRootCategories();
    std::vector<CategoryRecord> getChildCategories(i64 parentId);
    std::vector<ModelRecord> filterByCategory(i64 categoryId);

    // FTS5 search (preferred over LIKE-based searchModels for text queries)
    std::vector<ModelRecord> searchModelsFTS(const std::string& query);

    // Graph queries (delegated to GraphManager)
    std::vector<i64> getRelatedModelIds(i64 modelId);
    std::vector<i64> getModelsInProject(i64 projectId);
    bool isGraphAvailable() const;

  private:
    std::string computeFileHash(const Path& path);

    Database& m_db;
    ModelRepository m_modelRepo;
    GCodeRepository m_gcodeRepo;
    DuplicateHandler m_duplicateHandler;
    ThumbnailGenerator* m_thumbnailGen = nullptr;
    GraphManager* m_graphManager = nullptr;
};

} // namespace dw
