// Digital Workshop - RateCategory Repository Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/rate_category_repository.h"
#include "core/database/schema.h"

namespace {

// Fixture: in-memory DB with schema initialized
class RateCategoryRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::RateCategoryRepository>(m_db);
    }

    dw::RateCategory makeGlobal(const std::string& name, dw::f64 rate) {
        dw::RateCategory cat;
        cat.name = name;
        cat.ratePerCuUnit = rate;
        cat.projectId = 0;
        cat.notes = "";
        return cat;
    }

    dw::RateCategory makeProjectRate(const std::string& name,
                                     dw::f64 rate,
                                     dw::i64 projectId) {
        dw::RateCategory cat;
        cat.name = name;
        cat.ratePerCuUnit = rate;
        cat.projectId = projectId;
        cat.notes = "";
        return cat;
    }

    dw::Database m_db;
    std::unique_ptr<dw::RateCategoryRepository> m_repo;
};

} // namespace

// --- 1. InsertAndFindById ---

TEST_F(RateCategoryRepoTest, InsertAndFindById) {
    auto cat = makeGlobal("Finishing supplies", 3.0);
    cat.notes = "Sandpaper, finish, etc.";

    auto id = m_repo->insert(cat);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Finishing supplies");
    EXPECT_DOUBLE_EQ(found->ratePerCuUnit, 3.0);
    EXPECT_EQ(found->projectId, 0);
    EXPECT_EQ(found->notes, "Sandpaper, finish, etc.");
}

// --- 2. InsertGlobalCategory ---

TEST_F(RateCategoryRepoTest, InsertGlobalCategory) {
    m_repo->insert(makeGlobal("Adhesives", 1.50));
    m_repo->insert(makeGlobal("Finishing", 3.00));

    auto globals = m_repo->findGlobal();
    ASSERT_EQ(globals.size(), 2u);
    // All should have projectId=0
    for (const auto& g : globals) {
        EXPECT_EQ(g.projectId, 0);
    }
}

// --- 3. InsertProjectOverride ---

TEST_F(RateCategoryRepoTest, InsertProjectOverride) {
    m_repo->insert(makeGlobal("Finishing", 3.00));
    m_repo->insert(makeProjectRate("Finishing", 5.00, 5));

    auto globals = m_repo->findGlobal();
    EXPECT_EQ(globals.size(), 1u);

    auto projectRates = m_repo->findByProject(5);
    ASSERT_EQ(projectRates.size(), 1u);
    EXPECT_EQ(projectRates[0].name, "Finishing");
    EXPECT_DOUBLE_EQ(projectRates[0].ratePerCuUnit, 5.00);
    EXPECT_EQ(projectRates[0].projectId, 5);
}

// --- 4. GetEffectiveRates_GlobalOnly ---

TEST_F(RateCategoryRepoTest, GetEffectiveRates_GlobalOnly) {
    m_repo->insert(makeGlobal("Adhesives", 1.50));
    m_repo->insert(makeGlobal("Finishing", 3.00));
    m_repo->insert(makeGlobal("Sanding", 2.00));

    auto effective = m_repo->getEffectiveRates(99);
    ASSERT_EQ(effective.size(), 3u);
}

// --- 5. GetEffectiveRates_ProjectOverride ---

TEST_F(RateCategoryRepoTest, GetEffectiveRates_ProjectOverride) {
    m_repo->insert(makeGlobal("Finishing", 3.00));
    m_repo->insert(makeGlobal("Adhesives", 1.50));
    m_repo->insert(makeProjectRate("Finishing", 5.00, 1));

    auto effective = m_repo->getEffectiveRates(1);
    ASSERT_EQ(effective.size(), 2u);

    // Find the "Finishing" rate — should be the project override at $5
    bool foundFinishing = false;
    bool foundAdhesives = false;
    for (const auto& r : effective) {
        if (r.name == "Finishing") {
            EXPECT_DOUBLE_EQ(r.ratePerCuUnit, 5.00);
            EXPECT_EQ(r.projectId, 1);
            foundFinishing = true;
        }
        if (r.name == "Adhesives") {
            EXPECT_DOUBLE_EQ(r.ratePerCuUnit, 1.50);
            EXPECT_EQ(r.projectId, 0);
            foundAdhesives = true;
        }
    }
    EXPECT_TRUE(foundFinishing);
    EXPECT_TRUE(foundAdhesives);
}

// --- 6. GetEffectiveRates_ProjectOnlyAddsNew ---

TEST_F(RateCategoryRepoTest, GetEffectiveRates_ProjectOnlyAddsNew) {
    m_repo->insert(makeGlobal("Adhesives", 1.50));
    m_repo->insert(makeProjectRate("Special Coating", 10.00, 2));

    auto effective = m_repo->getEffectiveRates(2);
    ASSERT_EQ(effective.size(), 2u);

    bool foundAdhesives = false;
    bool foundSpecial = false;
    for (const auto& r : effective) {
        if (r.name == "Adhesives") foundAdhesives = true;
        if (r.name == "Special Coating") foundSpecial = true;
    }
    EXPECT_TRUE(foundAdhesives);
    EXPECT_TRUE(foundSpecial);
}

// --- 7. Update ---

TEST_F(RateCategoryRepoTest, Update) {
    auto id = m_repo->insert(makeGlobal("Finishing", 3.00));
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());

    found->ratePerCuUnit = 4.50;
    found->notes = "Updated rate";
    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_DOUBLE_EQ(updated->ratePerCuUnit, 4.50);
    EXPECT_EQ(updated->notes, "Updated rate");
}

// --- 8. Remove ---

TEST_F(RateCategoryRepoTest, Remove) {
    auto id = m_repo->insert(makeGlobal("Temp", 1.00));
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(m_repo->count(), 1);

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_EQ(m_repo->count(), 0);

    auto found = m_repo->findById(id.value());
    EXPECT_FALSE(found.has_value());
}

// --- 9. RemoveByProject ---

TEST_F(RateCategoryRepoTest, RemoveByProject) {
    m_repo->insert(makeProjectRate("Rate A", 1.00, 7));
    m_repo->insert(makeProjectRate("Rate B", 2.00, 7));
    m_repo->insert(makeGlobal("Global", 5.00));

    EXPECT_EQ(m_repo->count(), 3);

    EXPECT_TRUE(m_repo->removeByProject(7));
    EXPECT_EQ(m_repo->count(), 1);

    auto remaining = m_repo->findGlobal();
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0].name, "Global");
}

// --- 10. Count ---

TEST_F(RateCategoryRepoTest, Count) {
    EXPECT_EQ(m_repo->count(), 0);
    m_repo->insert(makeGlobal("A", 1.0));
    EXPECT_EQ(m_repo->count(), 1);
    m_repo->insert(makeGlobal("B", 2.0));
    EXPECT_EQ(m_repo->count(), 2);
    m_repo->insert(makeProjectRate("C", 3.0, 1));
    EXPECT_EQ(m_repo->count(), 3);
}

// --- 11. ComputeCost ---

TEST_F(RateCategoryRepoTest, ComputeCost) {
    dw::RateCategory cat;
    cat.ratePerCuUnit = 3.0;
    EXPECT_DOUBLE_EQ(cat.computeCost(10.0), 30.0);
    EXPECT_DOUBLE_EQ(cat.computeCost(0.0), 0.0);
    EXPECT_DOUBLE_EQ(cat.computeCost(1.5), 4.5);
}
