// Digital Workshop - GCode Repository Tests (project association)

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/gcode_repository.h"
#include "core/database/project_repository.h"
#include "core/database/schema.h"

namespace {

class GCodeRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::GCodeRepository>(m_db);
        m_projectRepo = std::make_unique<dw::ProjectRepository>(m_db);
    }

    dw::i64 createProject(const std::string& name) {
        dw::ProjectRecord p;
        p.name = name;
        auto id = m_projectRepo->insert(p);
        return id.value_or(0);
    }

    dw::i64 createGCode(const std::string& name, const std::string& hash) {
        dw::GCodeRecord rec;
        rec.name = name;
        rec.hash = hash;
        rec.filePath = "/tmp/" + name + ".gcode";
        auto id = m_repo->insert(rec);
        return id.value_or(0);
    }

    dw::Database m_db;
    std::unique_ptr<dw::GCodeRepository> m_repo;
    std::unique_ptr<dw::ProjectRepository> m_projectRepo;
};

} // namespace

// --- AddToProject ---

TEST_F(GCodeRepoTest, AddToProject) {
    auto pid = createProject("Test Project");
    auto gid = createGCode("test.gcode", "hash1");
    ASSERT_GT(pid, 0);
    ASSERT_GT(gid, 0);

    EXPECT_TRUE(m_repo->addToProject(pid, gid));

    auto results = m_repo->findByProject(pid);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].name, "test.gcode");
}

// --- RemoveFromProject ---

TEST_F(GCodeRepoTest, RemoveFromProject) {
    auto pid = createProject("Test Project");
    auto gid = createGCode("test.gcode", "hash1");

    EXPECT_TRUE(m_repo->addToProject(pid, gid));
    EXPECT_EQ(m_repo->findByProject(pid).size(), 1u);

    EXPECT_TRUE(m_repo->removeFromProject(pid, gid));
    EXPECT_TRUE(m_repo->findByProject(pid).empty());
}

// --- IsInProject ---

TEST_F(GCodeRepoTest, IsInProject) {
    auto pid = createProject("Test Project");
    auto gid = createGCode("test.gcode", "hash1");

    EXPECT_FALSE(m_repo->isInProject(pid, gid));

    m_repo->addToProject(pid, gid);
    EXPECT_TRUE(m_repo->isInProject(pid, gid));
}

// --- CascadeDelete ---

TEST_F(GCodeRepoTest, CascadeDelete) {
    auto pid = createProject("Test Project");
    auto gid1 = createGCode("file1.gcode", "hash1");
    auto gid2 = createGCode("file2.gcode", "hash2");

    m_repo->addToProject(pid, gid1);
    m_repo->addToProject(pid, gid2);
    EXPECT_EQ(m_repo->findByProject(pid).size(), 2u);

    // Enable foreign keys for cascade to work
    (void)m_db.execute("PRAGMA foreign_keys = ON");

    // Delete project
    m_projectRepo->remove(pid);

    // project_gcode rows should be gone
    // We can't use findByProject since project is gone, but we can check
    // getProjectsForGCode returns empty
    EXPECT_TRUE(m_repo->getProjectsForGCode(gid1).empty());
    EXPECT_TRUE(m_repo->getProjectsForGCode(gid2).empty());
}
