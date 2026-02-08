// Digital Workshop - Project Repository Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/model_repository.h"
#include "core/database/project_repository.h"
#include "core/database/schema.h"

namespace {

class ProjectRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::ProjectRepository>(m_db);
        m_modelRepo = std::make_unique<dw::ModelRepository>(m_db);
    }

    dw::ProjectRecord makeProject(const std::string& name) {
        dw::ProjectRecord rec;
        rec.name = name;
        rec.description = "Test project: " + name;
        rec.filePath = "/projects/" + name + ".dwp";
        return rec;
    }

    // Insert a model and return its ID
    dw::i64 insertModel(const std::string& hash, const std::string& name) {
        dw::ModelRecord rec;
        rec.hash = hash;
        rec.name = name;
        rec.filePath = "/models/" + name + ".stl";
        rec.fileFormat = "stl";
        return m_modelRepo->insert(rec).value();
    }

    dw::Database m_db;
    std::unique_ptr<dw::ProjectRepository> m_repo;
    std::unique_ptr<dw::ModelRepository> m_modelRepo;
};

}  // namespace

// --- Insert ---

TEST_F(ProjectRepoTest, Insert_ReturnsId) {
    auto id = m_repo->insert(makeProject("My Project"));
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}

// --- FindById ---

TEST_F(ProjectRepoTest, FindById_Found) {
    auto id = m_repo->insert(makeProject("Test"));
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Test");
    EXPECT_EQ(found->description, "Test project: Test");
}

TEST_F(ProjectRepoTest, FindById_NotFound) {
    auto found = m_repo->findById(999);
    EXPECT_FALSE(found.has_value());
}

// --- FindAll ---

TEST_F(ProjectRepoTest, FindAll_Empty) {
    auto all = m_repo->findAll();
    EXPECT_TRUE(all.empty());
}

TEST_F(ProjectRepoTest, FindAll_Multiple) {
    m_repo->insert(makeProject("A"));
    m_repo->insert(makeProject("B"));
    m_repo->insert(makeProject("C"));

    auto all = m_repo->findAll();
    EXPECT_EQ(all.size(), 3u);
}

// --- FindByName ---

TEST_F(ProjectRepoTest, FindByName) {
    m_repo->insert(makeProject("CNC Bracket"));
    m_repo->insert(makeProject("CNC Gear"));
    m_repo->insert(makeProject("3D Print Case"));

    auto results = m_repo->findByName("CNC");
    EXPECT_EQ(results.size(), 2u);
}

// --- Update ---

TEST_F(ProjectRepoTest, Update_ChangesDescription) {
    auto id = m_repo->insert(makeProject("Original"));
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    found->description = "Updated description";
    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->description, "Updated description");
}

// --- Remove ---

TEST_F(ProjectRepoTest, Remove) {
    auto id = m_repo->insert(makeProject("Delete Me"));
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(m_repo->count(), 1);

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_EQ(m_repo->count(), 0);
}

// --- Count ---

TEST_F(ProjectRepoTest, Count) {
    EXPECT_EQ(m_repo->count(), 0);
    m_repo->insert(makeProject("A"));
    EXPECT_EQ(m_repo->count(), 1);
    m_repo->insert(makeProject("B"));
    EXPECT_EQ(m_repo->count(), 2);
}

// --- Project-Model Links ---

TEST_F(ProjectRepoTest, AddModel_ToProject) {
    auto projId = m_repo->insert(makeProject("Proj")).value();
    auto modelId = insertModel("h1", "cube");

    EXPECT_TRUE(m_repo->addModel(projId, modelId));
    EXPECT_TRUE(m_repo->hasModel(projId, modelId));
}

TEST_F(ProjectRepoTest, RemoveModel_FromProject) {
    auto projId = m_repo->insert(makeProject("Proj")).value();
    auto modelId = insertModel("h1", "cube");

    m_repo->addModel(projId, modelId);
    ASSERT_TRUE(m_repo->hasModel(projId, modelId));

    EXPECT_TRUE(m_repo->removeModel(projId, modelId));
    EXPECT_FALSE(m_repo->hasModel(projId, modelId));
}

TEST_F(ProjectRepoTest, GetModelIds) {
    auto projId = m_repo->insert(makeProject("Proj")).value();
    auto m1 = insertModel("h1", "a");
    auto m2 = insertModel("h2", "b");
    auto m3 = insertModel("h3", "c");

    m_repo->addModel(projId, m1);
    m_repo->addModel(projId, m2);
    m_repo->addModel(projId, m3);

    auto ids = m_repo->getModelIds(projId);
    EXPECT_EQ(ids.size(), 3u);
}

TEST_F(ProjectRepoTest, GetProjectsForModel) {
    auto p1 = m_repo->insert(makeProject("Proj A")).value();
    auto p2 = m_repo->insert(makeProject("Proj B")).value();
    auto modelId = insertModel("h1", "shared_model");

    m_repo->addModel(p1, modelId);
    m_repo->addModel(p2, modelId);

    auto projects = m_repo->getProjectsForModel(modelId);
    EXPECT_EQ(projects.size(), 2u);
}

TEST_F(ProjectRepoTest, HasModel_False) {
    auto projId = m_repo->insert(makeProject("Empty")).value();
    EXPECT_FALSE(m_repo->hasModel(projId, 999));
}

TEST_F(ProjectRepoTest, UpdateModelOrder) {
    auto projId = m_repo->insert(makeProject("Ordered")).value();
    auto m1 = insertModel("h1", "first");

    m_repo->addModel(projId, m1, 0);
    EXPECT_TRUE(m_repo->updateModelOrder(projId, m1, 5));
}
