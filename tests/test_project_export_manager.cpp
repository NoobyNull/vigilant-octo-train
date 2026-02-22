// Digital Workshop - Project Export Manager Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/model_repository.h"
#include "core/database/project_repository.h"
#include "core/database/schema.h"
#include "core/export/project_export_manager.h"
#include "core/project/project.h"
#include "core/utils/file_utils.h"

#include <filesystem>
#include <miniz.h>
#include <nlohmann/json.hpp>

namespace {

class ProjectExportTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_baseDir = std::filesystem::temp_directory_path() / "dw_test_export";
        std::filesystem::create_directories(m_baseDir);

        m_modelsDir = m_baseDir / "models";
        std::filesystem::create_directories(m_modelsDir);

        m_archivePath = (m_baseDir / "test.dwproj").string();

        // Open in-memory DB and initialize schema
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
    }

    void TearDown() override { std::filesystem::remove_all(m_baseDir); }

    // Create a dummy model file and insert a ModelRecord
    dw::i64 insertModelWithFile(const std::string& hash, const std::string& name,
                                const std::string& ext = ".stl") {
        // Write a small binary blob to disk
        dw::Path filePath = m_modelsDir / (hash + ext);
        std::string content = "BINARYDATA_" + hash;
        EXPECT_TRUE(dw::file::writeText(filePath, content));

        dw::ModelRecord rec;
        rec.hash = hash;
        rec.name = name;
        rec.filePath = filePath;
        rec.fileFormat = ext.substr(1); // strip dot
        rec.fileSize = content.size();
        rec.vertexCount = 100;
        rec.triangleCount = 50;
        rec.boundsMin = dw::Vec3(-1.0f, -2.0f, -3.0f);
        rec.boundsMax = dw::Vec3(1.0f, 2.0f, 3.0f);
        rec.tags = {"tag1", "tag2"};

        dw::ModelRepository repo(m_db);
        auto id = repo.insert(rec);
        EXPECT_TRUE(id.has_value());
        return id.value_or(0);
    }

    // Create a project with linked models, return the Project object
    std::shared_ptr<dw::Project> createProjectWithModels(
        const std::string& projectName,
        const std::vector<std::pair<std::string, std::string>>& models) {
        dw::ProjectRepository projRepo(m_db);

        dw::ProjectRecord projRec;
        projRec.name = projectName;
        projRec.description = "Test project";
        auto projId = projRepo.insert(projRec);
        EXPECT_TRUE(projId.has_value());

        for (const auto& [hash, name] : models) {
            auto modelId = insertModelWithFile(hash, name);
            EXPECT_TRUE(projRepo.addModel(*projId, modelId));
        }

        // Build a Project object
        auto project = std::make_shared<dw::Project>();
        project->record().id = *projId;
        project->record().name = projectName;

        auto modelIds = projRepo.getModelIds(*projId);
        for (auto mid : modelIds) {
            project->addModel(mid);
        }

        return project;
    }

    // Manually create a .dwproj ZIP with custom manifest JSON
    void createArchiveWithManifest(const std::string& archivePath,
                                   const nlohmann::json& manifest,
                                   const std::vector<std::pair<std::string, std::string>>&
                                       blobEntries = {}) {
        mz_zip_archive zip{};
        ASSERT_TRUE(mz_zip_writer_init_file(&zip, archivePath.c_str(), 0));

        std::string manifestStr = manifest.dump(2);
        ASSERT_TRUE(mz_zip_writer_add_mem(&zip, "manifest.json", manifestStr.data(),
                                          manifestStr.size(), MZ_DEFAULT_COMPRESSION));

        for (const auto& [path, data] : blobEntries) {
            ASSERT_TRUE(mz_zip_writer_add_mem(&zip, path.c_str(), data.data(),
                                              data.size(), MZ_DEFAULT_COMPRESSION));
        }

        ASSERT_TRUE(mz_zip_writer_finalize_archive(&zip));
        mz_zip_writer_end(&zip);
    }

    std::filesystem::path m_baseDir;
    dw::Path m_modelsDir;
    std::string m_archivePath;
    dw::Database m_db;
};

} // namespace

// --- Test 1: Export creates valid ZIP with manifest.json ---

TEST_F(ProjectExportTest, ExportCreatesValidZipWithManifest) {
    auto project = createProjectWithModels(
        "Test Export Project",
        {{"aabbccdd1111", "Widget"}, {"eeff00112233", "Bracket"}});

    dw::ProjectExportManager exporter(m_db);
    auto result = exporter.exportProject(*project, m_archivePath);

    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.modelCount, 2);
    EXPECT_GT(result.totalBytes, 0u);

    // Verify the .dwproj file exists
    EXPECT_TRUE(dw::file::exists(m_archivePath));

    // Open ZIP and verify manifest.json exists
    mz_zip_archive zip{};
    ASSERT_TRUE(mz_zip_reader_init_file(&zip, m_archivePath.c_str(), 0));

    size_t manifestSize = 0;
    void* manifestData =
        mz_zip_reader_extract_file_to_heap(&zip, "manifest.json", &manifestSize, 0);
    ASSERT_NE(manifestData, nullptr);

    std::string manifestStr(static_cast<const char*>(manifestData), manifestSize);
    mz_free(manifestData);
    mz_zip_reader_end(&zip);

    // Parse and validate manifest
    auto j = nlohmann::json::parse(manifestStr);
    EXPECT_EQ(j["format_version"].get<int>(), 1);
    EXPECT_EQ(j["project_name"].get<std::string>(), "Test Export Project");
    EXPECT_TRUE(j.contains("app_version"));
    EXPECT_TRUE(j.contains("created_at"));
    EXPECT_TRUE(j.contains("project_id"));

    auto models = j["models"];
    ASSERT_EQ(models.size(), 2u);

    for (const auto& m : models) {
        EXPECT_TRUE(m.contains("hash"));
        EXPECT_TRUE(m.contains("name"));
        EXPECT_TRUE(m.contains("file_in_archive"));
    }
}

// --- Test 2: Import round-trip preserves model metadata ---

TEST_F(ProjectExportTest, ImportRoundTripPreservesMetadata) {
    auto project = createProjectWithModels(
        "Roundtrip Project",
        {{"hash_alpha_001", "Alpha Model"}, {"hash_beta_0002", "Beta Model"}});

    dw::ProjectExportManager exporter(m_db);

    // Export
    auto exportResult = exporter.exportProject(*project, m_archivePath);
    ASSERT_TRUE(exportResult.success) << exportResult.error;

    // Create a second database (simulating a different machine)
    dw::Database db2;
    ASSERT_TRUE(db2.open(":memory:"));
    ASSERT_TRUE(dw::Schema::initialize(db2));

    dw::ProjectExportManager importer(db2);
    auto importResult = importer.importProject(m_archivePath);

    ASSERT_TRUE(importResult.success) << importResult.error;
    EXPECT_EQ(importResult.modelCount, 2);

    // Verify project was created
    dw::ProjectRepository projRepo2(db2);
    auto projects = projRepo2.findAll();
    ASSERT_EQ(projects.size(), 1u);
    EXPECT_EQ(projects[0].name, "Roundtrip Project");

    // Verify models were linked
    auto modelIds = projRepo2.getModelIds(projects[0].id);
    EXPECT_EQ(modelIds.size(), 2u);

    // Verify model metadata
    dw::ModelRepository modelRepo2(db2);
    for (auto mid : modelIds) {
        auto model = modelRepo2.findById(mid);
        ASSERT_TRUE(model.has_value());
        EXPECT_FALSE(model->hash.empty());
        EXPECT_FALSE(model->name.empty());
        EXPECT_EQ(model->vertexCount, 100u);
        EXPECT_EQ(model->triangleCount, 50u);
        // Verify blob file exists on disk
        EXPECT_TRUE(dw::file::exists(model->filePath));
    }
}

// --- Test 3: Import ignores unknown manifest fields (forward compat) ---

TEST_F(ProjectExportTest, ImportIgnoresUnknownManifestFields) {
    // Build a manifest with an unknown future field
    nlohmann::json manifest;
    manifest["format_version"] = 1;
    manifest["app_version"] = "2.0.0";
    manifest["created_at"] = "2026-01-01T00:00:00Z";
    manifest["project_id"] = 42;
    manifest["project_name"] = "Future Project";
    manifest["future_feature"] = true;
    manifest["another_unknown"] = {{"nested", "data"}};

    nlohmann::json model;
    model["name"] = "Future Model";
    model["hash"] = "futurehash001";
    model["original_filename"] = "future.stl";
    model["file_in_archive"] = "models/futurehash001.stl";
    model["file_format"] = "stl";
    model["tags"] = nlohmann::json::array();
    model["vertex_count"] = 10;
    model["triangle_count"] = 5;
    model["bounds_min"] = {0.0, 0.0, 0.0};
    model["bounds_max"] = {1.0, 1.0, 1.0};
    model["unknown_model_field"] = "should be ignored";
    manifest["models"] = {model};

    // Create archive with blob
    std::string blobContent = "FAKE_STL_BLOB";
    createArchiveWithManifest(m_archivePath, manifest,
                              {{"models/futurehash001.stl", blobContent}});

    dw::ProjectExportManager importer(m_db);
    auto result = importer.importProject(m_archivePath);

    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.modelCount, 1);

    // Verify project was created correctly
    dw::ProjectRepository projRepo(m_db);
    auto projects = projRepo.findAll();
    ASSERT_EQ(projects.size(), 1u);
    EXPECT_EQ(projects[0].name, "Future Project");
}

// --- Test 4: Import deduplicates existing models ---

TEST_F(ProjectExportTest, ImportDeduplicatesExistingModels) {
    const std::string sharedHash = "dedup_hash_shared";

    // Pre-insert a model with the same hash into the DB
    insertModelWithFile(sharedHash, "Pre-existing Model");

    // Build manifest referencing a model with the same hash
    nlohmann::json manifest;
    manifest["format_version"] = 1;
    manifest["app_version"] = "1.1.0";
    manifest["created_at"] = "2026-01-01T00:00:00Z";
    manifest["project_id"] = 1;
    manifest["project_name"] = "Dedup Test Project";

    nlohmann::json model;
    model["name"] = "Same Model Different Name";
    model["hash"] = sharedHash;
    model["original_filename"] = "same.stl";
    model["file_in_archive"] = "models/" + sharedHash + ".stl";
    model["file_format"] = "stl";
    model["tags"] = nlohmann::json::array();
    model["vertex_count"] = 200;
    model["triangle_count"] = 100;
    model["bounds_min"] = {0.0, 0.0, 0.0};
    model["bounds_max"] = {1.0, 1.0, 1.0};
    manifest["models"] = {model};

    std::string blobContent = "FAKE_BLOB_DEDUP";
    createArchiveWithManifest(m_archivePath, manifest,
                              {{"models/" + sharedHash + ".stl", blobContent}});

    dw::ProjectExportManager importer(m_db);
    auto result = importer.importProject(m_archivePath);

    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.modelCount, 1);

    // Verify only ONE model with this hash exists (not duplicated)
    dw::ModelRepository modelRepo(m_db);
    auto all = modelRepo.findAll();
    int hashCount = 0;
    for (const auto& m : all) {
        if (m.hash == sharedHash) hashCount++;
    }
    EXPECT_EQ(hashCount, 1) << "Model should not be duplicated";

    // Verify the project still links to the model
    dw::ProjectRepository projRepo(m_db);
    auto projects = projRepo.findAll();
    // Find the imported project (not any pre-existing ones)
    bool found = false;
    for (const auto& p : projects) {
        if (p.name == "Dedup Test Project") {
            auto modelIds = projRepo.getModelIds(p.id);
            EXPECT_EQ(modelIds.size(), 1u);
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Imported project should exist";
}
