#include "library_manager.h"

#include "../../render/thumbnail_generator.h"
#include "../loaders/loader_factory.h"
#include "../mesh/hash.h"
#include "../paths/app_paths.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

LibraryManager::LibraryManager(Database& db) : m_db(db), m_modelRepo(db), m_gcodeRepo(db) {}

ImportResult LibraryManager::importModel(const Path& sourcePath) {
    ImportResult result;

    // Check file exists
    if (!file::exists(sourcePath)) {
        result.error = "File does not exist: " + sourcePath.string();
        log::error("Library", result.error);
        return result;
    }

    // Compute hash for deduplication
    std::string fileHash = computeFileHash(sourcePath);
    if (fileHash.empty()) {
        result.error = "Failed to compute file hash";
        log::error("Library", result.error);
        return result;
    }

    // Check for duplicate
    if (modelExists(fileHash)) {
        auto existing = getModelByHash(fileHash);
        if (existing) {
            result.isDuplicate = true;

            // Ask user what to do with duplicate
            if (m_duplicateHandler && !m_duplicateHandler(*existing)) {
                result.error = "Import cancelled: duplicate model";
                log::info("Library", "Import cancelled: duplicate model");
                return result;
            }

            // User wants to re-import (or no handler set)
            log::infof("Library", "Re-importing duplicate model: %s", existing->name.c_str());
            m_modelRepo.removeByHash(fileHash);
        }
    }

    // Load the mesh
    auto loadResult = LoaderFactory::load(sourcePath);
    if (!loadResult) {
        result.error = "Failed to load model: " + loadResult.error;
        log::error("Library", result.error);
        return result;
    }

    MeshPtr mesh = loadResult.mesh;

    // Create model record
    ModelRecord record;
    record.hash = fileHash;
    record.name = file::getStem(sourcePath);
    record.filePath = sourcePath;
    record.fileFormat = file::getExtension(sourcePath);

    auto fileSize = file::getFileSize(sourcePath);
    record.fileSize = fileSize ? *fileSize : 0;

    record.vertexCount = mesh->vertexCount();
    record.triangleCount = mesh->triangleCount();
    record.boundsMin = mesh->bounds().min;
    record.boundsMax = mesh->bounds().max;

    // Insert into database
    auto modelId = m_modelRepo.insert(record);
    if (!modelId) {
        result.error = "Failed to save model to database";
        log::error("Library", result.error);
        return result;
    }

    // Generate thumbnail (best effort)
    generateThumbnail(*modelId, *mesh);

    result.success = true;
    result.modelId = *modelId;

    log::infof("Library", "Imported model: %s (ID: %lld, %u triangles)", record.name.c_str(),
               static_cast<long long>(*modelId), record.triangleCount);

    return result;
}

std::vector<ModelRecord> LibraryManager::getAllModels() {
    return m_modelRepo.findAll();
}

std::vector<ModelRecord> LibraryManager::searchModels(const std::string& query) {
    return m_modelRepo.findByName(query);
}

std::vector<ModelRecord> LibraryManager::filterByFormat(const std::string& format) {
    return m_modelRepo.findByFormat(format);
}

std::vector<ModelRecord> LibraryManager::filterByTag(const std::string& tag) {
    return m_modelRepo.findByTag(tag);
}

std::optional<ModelRecord> LibraryManager::getModel(i64 modelId) {
    return m_modelRepo.findById(modelId);
}

std::optional<ModelRecord> LibraryManager::getModelByHash(const std::string& hash) {
    return m_modelRepo.findByHash(hash);
}

MeshPtr LibraryManager::loadMesh(i64 modelId) {
    auto record = getModel(modelId);
    if (!record) {
        return nullptr;
    }
    return loadMesh(*record);
}

MeshPtr LibraryManager::loadMesh(const ModelRecord& record) {
    auto loadResult = LoaderFactory::load(record.filePath);
    if (!loadResult) {
        log::errorf("Library", "Failed to load mesh: %s", loadResult.error.c_str());
        return nullptr;
    }

    loadResult.mesh->setName(record.name);
    return loadResult.mesh;
}

bool LibraryManager::updateModel(const ModelRecord& record) {
    return m_modelRepo.update(record);
}

bool LibraryManager::updateTags(i64 modelId, const std::vector<std::string>& tags) {
    return m_modelRepo.updateTags(modelId, tags);
}

bool LibraryManager::removeModel(i64 modelId) {
    auto record = getModel(modelId);
    if (!record) {
        return false;
    }

    // Remove thumbnail if exists (best-effort cleanup)
    if (!record->thumbnailPath.empty() && file::exists(record->thumbnailPath)) {
        (void)file::remove(record->thumbnailPath);
    }

    return m_modelRepo.remove(modelId);
}

i64 LibraryManager::modelCount() {
    return m_modelRepo.count();
}

bool LibraryManager::modelExists(const std::string& hash) {
    return m_modelRepo.exists(hash);
}

std::string LibraryManager::computeFileHash(const Path& path) {
    return hash::computeFile(path);
}

bool LibraryManager::generateThumbnail(i64 modelId, const Mesh& mesh,
                                       const Texture* materialTexture) {
    if (!m_thumbnailGen) {
        log::warning("Library", "Thumbnail generation skipped - no generator available");
        return false;
    }

    // Ensure thumbnail directory exists
    Path thumbnailDir = paths::getThumbnailDir();
    if (!file::exists(thumbnailDir)) {
        if (!file::createDirectories(thumbnailDir)) {
            log::warning("Library", "Failed to create thumbnail directory");
            return false;
        }
    }

    Path thumbnailPath = thumbnailDir / (std::to_string(modelId) + ".tga");

    ThumbnailSettings settings;
    settings.materialTexture = materialTexture;

    if (!m_thumbnailGen->generate(mesh, thumbnailPath, settings)) {
        log::warningf("Library", "Failed to generate thumbnail for model %lld",
                      static_cast<long long>(modelId));
        return false;
    }

    // Update database with thumbnail path
    m_modelRepo.updateThumbnail(modelId, thumbnailPath);

    log::infof("Library", "Generated thumbnail: %s", thumbnailPath.string().c_str());
    return true;
}

// --- G-code operations ---

std::vector<GCodeRecord> LibraryManager::getAllGCodeFiles() {
    return m_gcodeRepo.findAll();
}

std::vector<GCodeRecord> LibraryManager::searchGCodeFiles(const std::string& query) {
    return m_gcodeRepo.findByName(query);
}

std::optional<GCodeRecord> LibraryManager::getGCodeFile(i64 id) {
    return m_gcodeRepo.findById(id);
}

bool LibraryManager::deleteGCodeFile(i64 id) {
    auto record = getGCodeFile(id);
    if (!record) {
        return false;
    }

    // Remove thumbnail if exists (best-effort cleanup)
    if (!record->thumbnailPath.empty() && file::exists(record->thumbnailPath)) {
        (void)file::remove(record->thumbnailPath);
    }

    return m_gcodeRepo.remove(id);
}

// --- Hierarchy operations ---

std::optional<i64> LibraryManager::createOperationGroup(i64 modelId, const std::string& name,
                                                        int sortOrder) {
    return m_gcodeRepo.createGroup(modelId, name, sortOrder);
}

std::vector<OperationGroup> LibraryManager::getOperationGroups(i64 modelId) {
    return m_gcodeRepo.getGroups(modelId);
}

bool LibraryManager::addGCodeToGroup(i64 groupId, i64 gcodeId, int sortOrder) {
    return m_gcodeRepo.addToGroup(groupId, gcodeId, sortOrder);
}

bool LibraryManager::removeGCodeFromGroup(i64 groupId, i64 gcodeId) {
    return m_gcodeRepo.removeFromGroup(groupId, gcodeId);
}

std::vector<GCodeRecord> LibraryManager::getGroupGCodeFiles(i64 groupId) {
    return m_gcodeRepo.getGroupMembers(groupId);
}

bool LibraryManager::deleteOperationGroup(i64 groupId) {
    return m_gcodeRepo.deleteGroup(groupId);
}

// --- Template operations ---

std::vector<GCodeTemplate> LibraryManager::getTemplates() {
    return m_gcodeRepo.getTemplates();
}

bool LibraryManager::applyTemplate(i64 modelId, const std::string& templateName) {
    return m_gcodeRepo.applyTemplate(modelId, templateName);
}

// --- Auto-detect ---

std::optional<i64> LibraryManager::autoDetectModelMatch(const std::string& gcodeFilename) {
    // Strip extension
    std::string baseName = gcodeFilename;
    size_t dotPos = baseName.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
    }

    // Strip common G-code suffixes
    const std::vector<std::string> suffixes = {
        "_roughing", "_finishing",  "_profile", "_profiling", "_drill", "_drilling",
        "_contour",  "_contouring", "_pocket",  "_pocketing", "_trace", "_tracing",
        "_engrave",  "_engraving",  "_cut",     "_cutting",   "_mill",  "_milling"};

    for (const auto& suffix : suffixes) {
        if (baseName.size() > suffix.size()) {
            size_t pos = baseName.size() - suffix.size();
            if (baseName.substr(pos) == suffix) {
                baseName = baseName.substr(0, pos);
                break;
            }
        }
    }

    // Search for models with this base name
    auto matches = m_modelRepo.findByName(baseName);

    // Only return confident single match
    if (matches.size() == 1) {
        return matches[0].id;
    }

    // Zero or multiple matches: ambiguous, return nullopt
    return std::nullopt;
}

} // namespace dw
