// Digital Workshop - Cut Plan Repository Tests

#include <gtest/gtest.h>

#include "core/database/cut_plan_repository.h"
#include "core/database/database.h"
#include "core/database/project_repository.h"
#include "core/database/schema.h"

namespace {

class CutPlanRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::CutPlanRepository>(m_db);
        m_projectRepo = std::make_unique<dw::ProjectRepository>(m_db);
    }

    dw::i64 createProject(const std::string& name) {
        dw::ProjectRecord p;
        p.name = name;
        auto id = m_projectRepo->insert(p);
        return id.value_or(0);
    }

    dw::CutPlanRecord makeRecord(const std::string& name,
                                 std::optional<dw::i64> projectId = std::nullopt) {
        dw::CutPlanRecord rec;
        rec.name = name;
        rec.projectId = projectId;
        rec.algorithm = "guillotine";
        rec.sheetConfigJson = "{\"width\":48,\"height\":96,\"cost\":25,\"quantity\":0,\"name\":\"Plywood\"}";
        rec.partsJson = "[{\"id\":1,\"name\":\"Side\",\"width\":12,\"height\":24,\"quantity\":2}]";
        rec.resultJson = "{\"sheets\":[],\"unplacedParts\":[],\"totalUsedArea\":0,\"totalWasteArea\":0,\"totalCost\":0,\"sheetsUsed\":0}";
        rec.allowRotation = true;
        rec.kerf = 0.125f;
        rec.margin = 0.25f;
        rec.sheetsUsed = 1;
        rec.efficiency = 0.85f;
        return rec;
    }

    dw::Database m_db;
    std::unique_ptr<dw::CutPlanRepository> m_repo;
    std::unique_ptr<dw::ProjectRepository> m_projectRepo;
};

} // namespace

// --- InsertAndFindById ---

TEST_F(CutPlanRepoTest, InsertAndFindById) {
    auto pid = createProject("Test Project");
    auto rec = makeRecord("Test Plan", pid);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Test Plan");
    EXPECT_EQ(found->algorithm, "guillotine");
    EXPECT_TRUE(found->projectId.has_value());
    EXPECT_EQ(found->projectId.value(), pid);
    EXPECT_TRUE(found->allowRotation);
    EXPECT_NEAR(found->kerf, 0.125f, 0.001f);
    EXPECT_NEAR(found->margin, 0.25f, 0.001f);
    EXPECT_EQ(found->sheetsUsed, 1);
    EXPECT_NEAR(found->efficiency, 0.85f, 0.01f);
    EXPECT_FALSE(found->createdAt.empty());
    EXPECT_FALSE(found->modifiedAt.empty());
}

// --- FindByProject ---

TEST_F(CutPlanRepoTest, FindByProject) {
    auto pid1 = createProject("Project 1");
    auto pid2 = createProject("Project 2");

    m_repo->insert(makeRecord("Plan A", pid1));
    m_repo->insert(makeRecord("Plan B", pid1));
    m_repo->insert(makeRecord("Plan C", pid2));

    auto proj1Plans = m_repo->findByProject(pid1);
    EXPECT_EQ(proj1Plans.size(), 2u);

    auto proj2Plans = m_repo->findByProject(pid2);
    EXPECT_EQ(proj2Plans.size(), 1u);
    EXPECT_EQ(proj2Plans[0].name, "Plan C");
}

// --- Update ---

TEST_F(CutPlanRepoTest, Update) {
    auto id = m_repo->insert(makeRecord("Original"));
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    found->name = "Updated Name";
    found->algorithm = "first_fit_decreasing";
    found->sheetsUsed = 3;
    found->efficiency = 0.72f;
    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Name");
    EXPECT_EQ(updated->algorithm, "first_fit_decreasing");
    EXPECT_EQ(updated->sheetsUsed, 3);
    EXPECT_NEAR(updated->efficiency, 0.72f, 0.01f);
}

// --- Remove ---

TEST_F(CutPlanRepoTest, Remove) {
    auto id = m_repo->insert(makeRecord("To Remove"));
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(m_repo->count(), 1);

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_EQ(m_repo->count(), 0);
}

// --- JsonRoundTrip_Sheet ---

TEST_F(CutPlanRepoTest, JsonRoundTrip_Sheet) {
    dw::optimizer::Sheet sheet(48.0f, 96.0f, 25.0f);
    sheet.quantity = 10;
    sheet.name = "4x8 Plywood";

    std::string json = dw::CutPlanRepository::sheetToJson(sheet);
    auto parsed = dw::CutPlanRepository::jsonToSheet(json);

    EXPECT_NEAR(parsed.width, 48.0f, 0.01f);
    EXPECT_NEAR(parsed.height, 96.0f, 0.01f);
    EXPECT_NEAR(parsed.cost, 25.0f, 0.01f);
    EXPECT_EQ(parsed.quantity, 10);
    EXPECT_EQ(parsed.name, "4x8 Plywood");
}

// --- JsonRoundTrip_Parts ---

TEST_F(CutPlanRepoTest, JsonRoundTrip_Parts) {
    std::vector<dw::optimizer::Part> parts;
    parts.emplace_back(1, "Side Panel", 12.0f, 24.0f, 2);
    parts.emplace_back(2, "Top", 24.0f, 36.0f, 1);

    std::string json = dw::CutPlanRepository::partsToJson(parts);
    auto parsed = dw::CutPlanRepository::jsonToParts(json);

    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed[0].id, 1);
    EXPECT_EQ(parsed[0].name, "Side Panel");
    EXPECT_NEAR(parsed[0].width, 12.0f, 0.01f);
    EXPECT_NEAR(parsed[0].height, 24.0f, 0.01f);
    EXPECT_EQ(parsed[0].quantity, 2);

    EXPECT_EQ(parsed[1].id, 2);
    EXPECT_EQ(parsed[1].name, "Top");
    EXPECT_NEAR(parsed[1].width, 24.0f, 0.01f);
    EXPECT_NEAR(parsed[1].height, 36.0f, 0.01f);
    EXPECT_EQ(parsed[1].quantity, 1);
}

// --- NullProjectId ---

TEST_F(CutPlanRepoTest, NullProjectId) {
    auto rec = makeRecord("No Project");
    // projectId is already nullopt by default in makeRecord when not specified
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->projectId.has_value());

    // findByProject should return empty for any project
    auto results = m_repo->findByProject(999);
    EXPECT_TRUE(results.empty());
}
