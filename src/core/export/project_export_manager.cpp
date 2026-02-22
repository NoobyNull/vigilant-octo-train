// Digital Workshop - Project Export Manager
// Exports/imports projects as .dwproj ZIP archives (manifest.json + model blobs)

#include "project_export_manager.h"

#include "../materials/material_archive.h"
#include "../paths/app_paths.h"
#include "../project/project.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

#include <chrono>
#include <cstring>
#include <miniz.h>
#include <nlohmann/json.hpp>

namespace dw {

static constexpr const char* kAppVersion = "1.1.0";
static constexpr const char* kManifestFile = "manifest.json";
static constexpr const char* kLogModule = "ProjectExport";

// --- Helpers ---

static std::string isoTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm utc {};
    gmtime_r(&time, &utc);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buf;
}

static std::string getBasename(const Path& p) {
    return p.filename().string();
}

static std::string getFileExtension(const Path& p) {
    return p.extension().string();
}

static bool containsPathTraversal(const std::string& path) {
    return path.find("..") != std::string::npos;
}

// --- Construction ---

ProjectExportManager::ProjectExportManager(Database& db) : m_db(db) {}

// --- Helpers ---

std::optional<i64> ProjectExportManager::getModelMaterialId(i64 modelId) {
    auto stmt = m_db.prepare("SELECT material_id FROM models WHERE id = ?");
    if (!stmt.isValid() || !stmt.bindInt(1, modelId)) {
        return std::nullopt;
    }
    if (stmt.step()) {
        if (stmt.isNull(0)) {
            return std::nullopt;
        }
        return stmt.getInt(0);
    }
    return std::nullopt;
}

// --- Export ---

DwprojExportResult
ProjectExportManager::exportProject(const Project& project, const Path& outputPath,
                                    ExportProgressCallback progress) {
    ModelRepository modelRepo(m_db);
    ProjectRepository projectRepo(m_db);

    // Get model IDs for this project
    auto modelIds = projectRepo.getModelIds(project.id());
    if (modelIds.empty()) {
        return DwprojExportResult::fail("Project has no models to export");
    }

    // Collect model records
    std::vector<ModelRecord> models;
    models.reserve(modelIds.size());
    for (auto id : modelIds) {
        auto rec = modelRepo.findById(id);
        if (rec) {
            models.push_back(std::move(*rec));
        }
    }

    if (models.empty()) {
        return DwprojExportResult::fail("No valid models found in project");
    }

    MaterialRepository materialRepo(m_db);

    // Initialize ZIP writer
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_file(&zip, outputPath.string().c_str(), 0)) {
        return DwprojExportResult::fail("Failed to create archive file: " +
                                        outputPath.string());
    }

    // Add each model blob
    uint64_t totalBytes = 0;
    int itemIndex = 0;
    int totalItems = static_cast<int>(models.size());

    for (const auto& model : models) {
        auto blobData = file::readBinary(model.filePath);
        if (!blobData) {
            log::warningf(kLogModule, "Skipping model '%s': cannot read file '%s'",
                          model.name.c_str(), model.filePath.string().c_str());
            itemIndex++;
            continue;
        }

        std::string ext = getFileExtension(model.filePath);
        std::string archPath = "models/" + model.hash + ext;

        if (!mz_zip_writer_add_mem(&zip, archPath.c_str(), blobData->data(),
                                   blobData->size(), MZ_DEFAULT_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            (void)file::remove(outputPath);
            return DwprojExportResult::fail("Failed to add model blob: " +
                                            model.name);
        }

        totalBytes += blobData->size();
        itemIndex++;

        if (progress) {
            progress(itemIndex, totalItems, model.name);
        }
    }

    // Phase A: Export thumbnails
    std::unordered_map<std::string, std::string> hashToThumbnailPath;
    for (const auto& model : models) {
        if (model.thumbnailPath.empty() || !file::exists(model.thumbnailPath)) {
            continue;
        }
        auto thumbData = file::readBinary(model.thumbnailPath);
        if (!thumbData) {
            continue;
        }
        std::string thumbArchPath =
            "thumbnails/" + model.hash + ".png";
        if (!mz_zip_writer_add_mem(&zip, thumbArchPath.c_str(),
                                   thumbData->data(), thumbData->size(),
                                   MZ_DEFAULT_COMPRESSION)) {
            log::warningf(kLogModule, "Failed to add thumbnail for model '%s'",
                          model.name.c_str());
            continue;
        }
        hashToThumbnailPath[model.hash] = thumbArchPath;
    }

    // Phase B: Export materials
    std::unordered_map<i64, i64> modelIdToMaterialId;
    std::unordered_set<i64> writtenMaterialIds;
    for (const auto& model : models) {
        auto matId = getModelMaterialId(model.id);
        if (!matId) {
            continue;
        }
        modelIdToMaterialId[model.id] = *matId;

        if (writtenMaterialIds.count(*matId)) {
            continue; // Already written (shared material)
        }

        auto matRec = materialRepo.findById(*matId);
        if (!matRec || matRec->archivePath.empty() ||
            !file::exists(matRec->archivePath)) {
            continue;
        }

        auto matData = file::readBinary(matRec->archivePath);
        if (!matData) {
            continue;
        }

        std::string matArchPath =
            "materials/" + std::to_string(*matId) + ".dwmat";
        if (!mz_zip_writer_add_mem(&zip, matArchPath.c_str(), matData->data(),
                                   matData->size(), MZ_DEFAULT_COMPRESSION)) {
            log::warningf(kLogModule,
                          "Failed to add material %lld for model '%s'",
                          static_cast<long long>(*matId), model.name.c_str());
            continue;
        }
        writtenMaterialIds.insert(*matId);
    }

    // Build manifest with material/thumbnail info and add to ZIP
    std::string manifestJson =
        buildManifestJson(project, models, modelIdToMaterialId,
                          hashToThumbnailPath);

    if (!mz_zip_writer_add_mem(&zip, kManifestFile, manifestJson.data(),
                               manifestJson.size(), MZ_DEFAULT_COMPRESSION)) {
        mz_zip_writer_end(&zip);
        (void)file::remove(outputPath);
        return DwprojExportResult::fail("Failed to write manifest to archive");
    }

    // Finalize archive
    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        (void)file::remove(outputPath);
        return DwprojExportResult::fail("Failed to finalize archive");
    }

    mz_zip_writer_end(&zip);

    int modelCount = static_cast<int>(models.size());
    log::infof(kLogModule,
               "Exported project '%s' with %d models (%llu bytes) to '%s'",
               project.name().c_str(), modelCount,
               static_cast<unsigned long long>(totalBytes),
               outputPath.string().c_str());

    return DwprojExportResult::ok(modelCount, totalBytes);
}

// --- Import ---

DwprojExportResult
ProjectExportManager::importProject(const Path& archivePath,
                                    ExportProgressCallback progress) {
    ModelRepository modelRepo(m_db);
    ProjectRepository projectRepo(m_db);

    // Open ZIP reader
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, archivePath.string().c_str(), 0)) {
        return DwprojExportResult::fail("Failed to open archive: " +
                                        archivePath.string());
    }

    // Extract manifest.json
    size_t manifestSize = 0;
    void* manifestData =
        mz_zip_reader_extract_file_to_heap(&zip, kManifestFile, &manifestSize, 0);
    if (!manifestData) {
        mz_zip_reader_end(&zip);
        return DwprojExportResult::fail("Archive missing manifest.json");
    }

    std::string manifestJson(static_cast<const char*>(manifestData), manifestSize);
    mz_free(manifestData);

    // Parse manifest
    Manifest manifest;
    std::string parseError;
    if (!parseManifest(manifestJson, manifest, parseError)) {
        mz_zip_reader_end(&zip);
        return DwprojExportResult::fail("Invalid manifest: " + parseError);
    }

    // Warn on newer format version (but continue for forward compat)
    if (manifest.formatVersion > FormatVersion) {
        log::warningf(kLogModule,
                      "Archive format version %d is newer than supported version %d. "
                      "Some features may be unavailable.",
                      manifest.formatVersion, FormatVersion);
    }

    // Create project record
    ProjectRecord projRec;
    projRec.name = manifest.projectName;
    projRec.description = "Imported from " + archivePath.filename().string();

    auto projectId = projectRepo.insert(projRec);
    if (!projectId) {
        mz_zip_reader_end(&zip);
        return DwprojExportResult::fail("Failed to create project in database");
    }

    // Ensure models directory exists
    Path modelsDir = paths::getDataDir() / "models";
    (void)file::createDirectories(modelsDir);

    // Import each model
    int totalModels = static_cast<int>(manifest.models.size());
    int importedCount = 0;
    uint64_t totalBytes = 0;

    for (size_t i = 0; i < manifest.models.size(); i++) {
        const auto& mm = manifest.models[i];

        // Path traversal check
        if (containsPathTraversal(mm.fileInArchive)) {
            log::warningf(kLogModule, "Skipping model with suspicious path: %s",
                          mm.fileInArchive.c_str());
            continue;
        }

        // Check if model already exists (dedup by hash)
        bool alreadyExists = modelRepo.exists(mm.hash);

        if (!alreadyExists) {
            // Extract blob from ZIP
            size_t blobSize = 0;
            void* blobData = mz_zip_reader_extract_file_to_heap(
                &zip, mm.fileInArchive.c_str(), &blobSize, 0);
            if (!blobData) {
                log::warningf(kLogModule, "Failed to extract model blob: %s",
                              mm.fileInArchive.c_str());
                continue;
            }

            // Determine file extension from archive path
            Path archPath(mm.fileInArchive);
            std::string ext = archPath.extension().string();
            if (ext.empty()) {
                ext = "." + mm.fileFormat;
            }

            // Write blob to models directory
            Path destPath = modelsDir / (mm.hash + ext);
            if (!file::writeBinary(destPath, blobData, blobSize)) {
                mz_free(blobData);
                log::warningf(kLogModule, "Failed to write model blob to: %s",
                              destPath.string().c_str());
                continue;
            }

            totalBytes += blobSize;
            mz_free(blobData);

            // Create model record
            ModelRecord rec;
            rec.hash = mm.hash;
            rec.name = mm.name;
            rec.filePath = destPath;
            rec.fileFormat = mm.fileFormat;
            rec.fileSize = blobSize;
            rec.vertexCount = mm.vertexCount;
            rec.triangleCount = mm.triangleCount;
            rec.boundsMin = mm.boundsMin;
            rec.boundsMax = mm.boundsMax;
            rec.tags = mm.tags;

            auto modelId = modelRepo.insert(rec);
            if (modelId) {
                (void)projectRepo.addModel(*projectId, *modelId);
            }
        } else {
            // Model already exists -- just link to project
            auto existing = modelRepo.findByHash(mm.hash);
            if (existing) {
                (void)projectRepo.addModel(*projectId, existing->id);
            }
        }

        importedCount++;

        if (progress) {
            progress(static_cast<int>(i) + 1, totalModels, mm.name);
        }
    }

    // Phase A: Restore thumbnails
    Path thumbnailDir = paths::getThumbnailDir();
    (void)file::createDirectories(thumbnailDir);

    for (size_t i = 0; i < manifest.models.size(); i++) {
        const auto& mm = manifest.models[i];
        if (mm.thumbnailInArchive.empty() ||
            containsPathTraversal(mm.thumbnailInArchive)) {
            continue;
        }

        size_t thumbSize = 0;
        void* thumbData = mz_zip_reader_extract_file_to_heap(
            &zip, mm.thumbnailInArchive.c_str(), &thumbSize, 0);
        if (!thumbData) {
            continue;
        }

        Path thumbDest = thumbnailDir / (mm.hash + ".png");
        if (file::writeBinary(thumbDest, thumbData, thumbSize)) {
            // Find the model we just imported and update its thumbnail
            auto imported = modelRepo.findByHash(mm.hash);
            if (imported) {
                modelRepo.updateThumbnail(imported->id, thumbDest);
            }
        }
        mz_free(thumbData);
    }

    // Phase B: Restore materials
    MaterialRepository materialRepo(m_db);
    Path materialsDir = paths::getMaterialsDir();
    (void)file::createDirectories(materialsDir);

    std::unordered_map<i64, i64> oldToNewMaterialId;
    for (size_t i = 0; i < manifest.models.size(); i++) {
        const auto& mm = manifest.models[i];
        if (!mm.materialId || mm.materialInArchive.empty() ||
            containsPathTraversal(mm.materialInArchive)) {
            continue;
        }

        i64 oldMatId = *mm.materialId;

        // Find the imported model
        auto imported = modelRepo.findByHash(mm.hash);
        if (!imported) {
            continue;
        }

        i64 newMatId = 0;
        auto mapIt = oldToNewMaterialId.find(oldMatId);
        if (mapIt != oldToNewMaterialId.end()) {
            newMatId = mapIt->second;
        } else {
            // Extract .dwmat from ZIP
            size_t matSize = 0;
            void* matData = mz_zip_reader_extract_file_to_heap(
                &zip, mm.materialInArchive.c_str(), &matSize, 0);
            if (!matData) {
                continue;
            }

            std::string matFilename =
                std::to_string(oldMatId) + ".dwmat";
            Path matDest = materialsDir / matFilename;
            if (!file::writeBinary(matDest, matData, matSize)) {
                mz_free(matData);
                continue;
            }
            mz_free(matData);

            // Try to load metadata from the .dwmat archive
            MaterialRecord matRec;
            auto matArchiveData =
                MaterialArchive::load(matDest.string());
            if (matArchiveData) {
                matRec = matArchiveData->metadata;
            } else {
                matRec.name = "Imported Material " +
                              std::to_string(oldMatId);
            }
            matRec.archivePath = matDest;

            auto insertedId = materialRepo.insert(matRec);
            if (!insertedId) {
                continue;
            }
            newMatId = *insertedId;
            oldToNewMaterialId[oldMatId] = newMatId;
        }

        // Assign material to model
        auto stmt = m_db.prepare(
            "UPDATE models SET material_id = ? WHERE id = ?");
        if (stmt.isValid() && stmt.bindInt(1, newMatId) &&
            stmt.bindInt(2, imported->id)) {
            (void)stmt.execute();
        }
    }

    mz_zip_reader_end(&zip);

    log::infof(kLogModule, "Imported project '%s' with %d models from '%s'",
               manifest.projectName.c_str(), importedCount,
               archivePath.string().c_str());

    return DwprojExportResult::ok(importedCount, totalBytes);
}

// --- Manifest JSON ---

std::string ProjectExportManager::buildManifestJson(
    const Project& project, const std::vector<ModelRecord>& models,
    const std::unordered_map<i64, i64>& modelIdToMaterialId,
    const std::unordered_map<std::string, std::string>& hashToThumbnailPath) {
    nlohmann::json j;
    j["format_version"] = FormatVersion;
    j["app_version"] = kAppVersion;
    j["created_at"] = isoTimestamp();
    j["project_id"] = project.id();
    j["project_name"] = project.name();

    nlohmann::json modelsArr = nlohmann::json::array();
    for (const auto& m : models) {
        std::string ext = getFileExtension(m.filePath);
        std::string archPath = "models/" + m.hash + ext;

        nlohmann::json mj;
        mj["name"] = m.name;
        mj["hash"] = m.hash;
        mj["original_filename"] = getBasename(m.filePath);
        mj["file_in_archive"] = archPath;
        mj["file_format"] = m.fileFormat;
        mj["tags"] = m.tags;
        mj["vertex_count"] = m.vertexCount;
        mj["triangle_count"] = m.triangleCount;
        mj["bounds_min"] = {m.boundsMin.x, m.boundsMin.y, m.boundsMin.z};
        mj["bounds_max"] = {m.boundsMax.x, m.boundsMax.y, m.boundsMax.z};

        // Material info
        auto matIt = modelIdToMaterialId.find(m.id);
        if (matIt != modelIdToMaterialId.end()) {
            mj["material_id"] = matIt->second;
            mj["material_in_archive"] =
                "materials/" + std::to_string(matIt->second) + ".dwmat";
        } else {
            mj["material_id"] = nullptr;
            mj["material_in_archive"] = "";
        }

        // Thumbnail info
        auto thumbIt = hashToThumbnailPath.find(m.hash);
        if (thumbIt != hashToThumbnailPath.end()) {
            mj["thumbnail_in_archive"] = thumbIt->second;
        } else {
            mj["thumbnail_in_archive"] = "";
        }

        modelsArr.push_back(mj);
    }
    j["models"] = modelsArr;

    return j.dump(2);
}

bool ProjectExportManager::parseManifest(const std::string& json, Manifest& out,
                                         std::string& error) {
    try {
        auto j = nlohmann::json::parse(json);

        // Required fields
        if (!j.contains("format_version") || !j.contains("models")) {
            error = "Missing required fields (format_version, models)";
            return false;
        }

        out.formatVersion = j.value("format_version", 1);
        out.appVersion = j.value("app_version", "unknown");
        out.createdAt = j.value("created_at", "");
        out.projectId = j.value("project_id", static_cast<i64>(0));
        out.projectName = j.value("project_name", "Imported Project");

        // Parse models array -- unknown fields are silently ignored
        for (const auto& mj : j["models"]) {
            ManifestModel mm;
            mm.name = mj.value("name", "Unnamed");
            mm.hash = mj.value("hash", "");
            mm.originalFilename = mj.value("original_filename", "");
            mm.fileInArchive = mj.value("file_in_archive", "");
            mm.fileFormat = mj.value("file_format", "stl");
            mm.vertexCount = mj.value("vertex_count", static_cast<u32>(0));
            mm.triangleCount = mj.value("triangle_count", static_cast<u32>(0));

            if (mj.contains("tags") && mj["tags"].is_array()) {
                for (const auto& t : mj["tags"]) {
                    mm.tags.push_back(t.get<std::string>());
                }
            }

            if (mj.contains("bounds_min") && mj["bounds_min"].is_array() &&
                mj["bounds_min"].size() == 3) {
                mm.boundsMin = Vec3(mj["bounds_min"][0].get<float>(),
                                    mj["bounds_min"][1].get<float>(),
                                    mj["bounds_min"][2].get<float>());
            }
            if (mj.contains("bounds_max") && mj["bounds_max"].is_array() &&
                mj["bounds_max"].size() == 3) {
                mm.boundsMax = Vec3(mj["bounds_max"][0].get<float>(),
                                    mj["bounds_max"][1].get<float>(),
                                    mj["bounds_max"][2].get<float>());
            }

            // Material and thumbnail fields (optional, for forward compat)
            if (mj.contains("material_id") && !mj["material_id"].is_null()) {
                mm.materialId = mj["material_id"].get<i64>();
            }
            mm.materialInArchive = mj.value("material_in_archive", "");
            mm.thumbnailInArchive = mj.value("thumbnail_in_archive", "");

            if (mm.hash.empty()) {
                log::warningf(kLogModule,
                              "Skipping model with missing hash in manifest");
                continue;
            }

            out.models.push_back(std::move(mm));
        }

        return true;
    } catch (const nlohmann::json::exception& e) {
        error = std::string("JSON parse error: ") + e.what();
        return false;
    }
}

} // namespace dw
