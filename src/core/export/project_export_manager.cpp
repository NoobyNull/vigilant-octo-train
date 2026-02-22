// Digital Workshop - Project Export Manager
// Exports/imports projects as .dwproj ZIP archives (manifest.json + model blobs)

#include "project_export_manager.h"

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

    // Build manifest JSON
    std::string manifestJson = buildManifestJson(project, models);

    // Initialize ZIP writer
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_file(&zip, outputPath.string().c_str(), 0)) {
        return DwprojExportResult::fail("Failed to create archive file: " +
                                        outputPath.string());
    }

    // Add manifest.json
    if (!mz_zip_writer_add_mem(&zip, kManifestFile, manifestJson.data(),
                               manifestJson.size(), MZ_DEFAULT_COMPRESSION)) {
        mz_zip_writer_end(&zip);
        (void)file::remove(outputPath);
        return DwprojExportResult::fail("Failed to write manifest to archive");
    }

    // Add each model blob
    uint64_t totalBytes = 0;
    int modelIndex = 0;
    int totalModels = static_cast<int>(models.size());

    for (const auto& model : models) {
        // Read model file from disk
        auto blobData = file::readBinary(model.filePath);
        if (!blobData) {
            log::warningf(kLogModule, "Skipping model '%s': cannot read file '%s'",
                          model.name.c_str(), model.filePath.string().c_str());
            modelIndex++;
            continue;
        }

        // Construct archive path: models/<hash>.<ext>
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
        modelIndex++;

        if (progress) {
            progress(modelIndex, totalModels, model.name);
        }
    }

    // Finalize archive
    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        (void)file::remove(outputPath);
        return DwprojExportResult::fail("Failed to finalize archive");
    }

    mz_zip_writer_end(&zip);

    log::infof(kLogModule, "Exported project '%s' with %d models (%llu bytes) to '%s'",
               project.name().c_str(), modelIndex,
               static_cast<unsigned long long>(totalBytes),
               outputPath.string().c_str());

    return DwprojExportResult::ok(modelIndex, totalBytes);
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

    mz_zip_reader_end(&zip);

    log::infof(kLogModule, "Imported project '%s' with %d models from '%s'",
               manifest.projectName.c_str(), importedCount,
               archivePath.string().c_str());

    return DwprojExportResult::ok(importedCount, totalBytes);
}

// --- Manifest JSON ---

std::string
ProjectExportManager::buildManifestJson(const Project& project,
                                        const std::vector<ModelRecord>& models) {
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
