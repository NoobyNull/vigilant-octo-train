// Digital Workshop - ProjectCostingIO Tests

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "core/project/project_costing_io.h"
#include "core/utils/file_utils.h"

namespace fs = std::filesystem;

namespace {

class ProjectCostingIOTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_tempDir = fs::temp_directory_path() / "dw_costing_test";
        fs::create_directories(m_tempDir);
    }

    void TearDown() override { fs::remove_all(m_tempDir); }

    dw::CostingEntry makeEntry(const std::string& name, const std::string& category) {
        dw::CostingEntry e;
        e.id = "entry-" + name;
        e.name = name;
        e.category = category;
        e.quantity = 2.0;
        e.unitRate = 25.0;
        e.estimatedTotal = 50.0;
        e.actualTotal = 0.0;
        e.unit = "sheet";
        e.notes = "test note";
        return e;
    }

    dw::CostingEstimate makeEstimate(const std::string& name) {
        dw::CostingEstimate est;
        est.name = name;
        est.salePrice = 500.0;
        est.notes = "estimate notes";
        est.createdAt = "2026-03-01T10:00:00";
        est.modifiedAt = "2026-03-02T15:00:00";
        est.entries.push_back(makeEntry("Plywood", "material"));
        est.entries.push_back(makeEntry("Assembly", "labor"));
        return est;
    }

    fs::path m_tempDir;
};

} // namespace

// --- SaveEstimates ---

TEST_F(ProjectCostingIOTest, SaveEstimates_CreatesFile) {
    auto est = makeEstimate("Test Estimate");
    std::vector<dw::CostingEstimate> estimates = {est};

    EXPECT_TRUE(dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), estimates));
    EXPECT_TRUE(fs::exists(m_tempDir / "estimates.json"));
}

TEST_F(ProjectCostingIOTest, SaveEstimates_HasSchemaVersion) {
    auto est = makeEstimate("Version Test");
    ASSERT_TRUE(
        dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), {est}));

    auto text = dw::file::readText((m_tempDir / "estimates.json").string());
    ASSERT_TRUE(text.has_value());
    auto j = nlohmann::json::parse(*text);
    EXPECT_EQ(j["schema_version"].get<int>(), 1);
}

// --- LoadEstimates ---

TEST_F(ProjectCostingIOTest, LoadEstimates_Empty) {
    auto loaded = dw::ProjectCostingIO::loadEstimates(m_tempDir.string());
    EXPECT_TRUE(loaded.empty());
}

TEST_F(ProjectCostingIOTest, LoadEstimates_RoundTrip) {
    auto est1 = makeEstimate("Estimate A");
    auto est2 = makeEstimate("Estimate B");
    est2.salePrice = 1200.0;

    ASSERT_TRUE(
        dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), {est1, est2}));

    auto loaded = dw::ProjectCostingIO::loadEstimates(m_tempDir.string());
    ASSERT_EQ(loaded.size(), 2u);

    EXPECT_EQ(loaded[0].name, "Estimate A");
    EXPECT_DOUBLE_EQ(loaded[0].salePrice, 500.0);
    EXPECT_EQ(loaded[0].notes, "estimate notes");
    EXPECT_EQ(loaded[0].createdAt, "2026-03-01T10:00:00");
    EXPECT_EQ(loaded[0].modifiedAt, "2026-03-02T15:00:00");
    ASSERT_EQ(loaded[0].entries.size(), 2u);
    EXPECT_EQ(loaded[0].entries[0].name, "Plywood");
    EXPECT_EQ(loaded[0].entries[0].category, "material");
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].quantity, 2.0);
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].unitRate, 25.0);
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].estimatedTotal, 50.0);
    EXPECT_EQ(loaded[0].entries[0].unit, "sheet");

    EXPECT_EQ(loaded[1].name, "Estimate B");
    EXPECT_DOUBLE_EQ(loaded[1].salePrice, 1200.0);
}

// --- SaveOrders ---

TEST_F(ProjectCostingIOTest, SaveOrders_CreatesFile) {
    dw::CostingOrder order;
    order.name = "Test Order";
    order.salePrice = 100.0;
    order.finalizedAt = "2026-03-04T12:00:00";

    EXPECT_TRUE(dw::ProjectCostingIO::saveOrders(m_tempDir.string(), {order}));
    EXPECT_TRUE(fs::exists(m_tempDir / "orders.json"));
}

TEST_F(ProjectCostingIOTest, LoadOrders_RoundTrip) {
    dw::CostingOrder order;
    order.name = "Final Order";
    order.salePrice = 750.0;
    order.notes = "order notes";
    order.createdAt = "2026-03-01T10:00:00";
    order.finalizedAt = "2026-03-04T12:00:00";
    order.sourceEstimateName = "Original Estimate";
    order.entries.push_back(makeEntry("Wood", "material"));

    ASSERT_TRUE(dw::ProjectCostingIO::saveOrders(m_tempDir.string(), {order}));

    auto loaded = dw::ProjectCostingIO::loadOrders(m_tempDir.string());
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].name, "Final Order");
    EXPECT_DOUBLE_EQ(loaded[0].salePrice, 750.0);
    EXPECT_EQ(loaded[0].notes, "order notes");
    EXPECT_EQ(loaded[0].finalizedAt, "2026-03-04T12:00:00");
    EXPECT_EQ(loaded[0].sourceEstimateName, "Original Estimate");
    ASSERT_EQ(loaded[0].entries.size(), 1u);
    EXPECT_EQ(loaded[0].entries[0].name, "Wood");
}

// --- ConvertToOrder ---

TEST_F(ProjectCostingIOTest, ConvertToOrder_CopiesAllEntries) {
    auto est = makeEstimate("Source Estimate");
    est.entries.push_back(makeEntry("Router Bit", "tooling"));

    auto order = dw::ProjectCostingIO::convertToOrder(est);
    EXPECT_EQ(order.entries.size(), 3u);
}

TEST_F(ProjectCostingIOTest, ConvertToOrder_SetsSourceEstimateName) {
    auto est = makeEstimate("My Estimate");
    auto order = dw::ProjectCostingIO::convertToOrder(est);
    EXPECT_EQ(order.sourceEstimateName, "My Estimate");
}

TEST_F(ProjectCostingIOTest, ConvertToOrder_SetsFinalizedAt) {
    auto est = makeEstimate("Some Estimate");
    auto order = dw::ProjectCostingIO::convertToOrder(est);
    EXPECT_FALSE(order.finalizedAt.empty());
}

// --- Snapshot ---

TEST_F(ProjectCostingIOTest, SnapshotPreservation_RoundTrip) {
    auto est = makeEstimate("Snapshot Test");
    est.entries[0].snapshot.dbId = 42;
    est.entries[0].snapshot.name = "Oak Plywood";
    est.entries[0].snapshot.dimensions = "1220x2440x19mm";
    est.entries[0].snapshot.priceAtCapture = 45.99;

    ASSERT_TRUE(
        dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), {est}));

    auto loaded = dw::ProjectCostingIO::loadEstimates(m_tempDir.string());
    ASSERT_EQ(loaded.size(), 1u);
    ASSERT_GE(loaded[0].entries.size(), 1u);
    EXPECT_EQ(loaded[0].entries[0].snapshot.dbId, 42);
    EXPECT_EQ(loaded[0].entries[0].snapshot.name, "Oak Plywood");
    EXPECT_EQ(loaded[0].entries[0].snapshot.dimensions, "1220x2440x19mm");
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].snapshot.priceAtCapture, 45.99);
}

// --- Entry Categories ---

TEST_F(ProjectCostingIOTest, EntryCategories_AllTypes) {
    dw::CostingEstimate est;
    est.name = "Category Test";
    const std::vector<std::string> categories = {
        "material", "tooling", "consumable", "labor", "overhead"};

    for (const auto& cat : categories) {
        est.entries.push_back(makeEntry(cat + "-item", cat));
    }

    ASSERT_TRUE(
        dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), {est}));

    auto loaded = dw::ProjectCostingIO::loadEstimates(m_tempDir.string());
    ASSERT_EQ(loaded.size(), 1u);
    ASSERT_EQ(loaded[0].entries.size(), 5u);
    for (size_t i = 0; i < categories.size(); ++i) {
        EXPECT_EQ(loaded[0].entries[i].category, categories[i]);
    }
}

// --- Actual vs Estimated Total ---

TEST_F(ProjectCostingIOTest, ActualTotal_PreservedSeparately) {
    dw::CostingEstimate est;
    est.name = "Actual Test";
    auto entry = makeEntry("Item", "material");
    entry.estimatedTotal = 100.0;
    entry.actualTotal = 120.0;
    est.entries.push_back(entry);

    ASSERT_TRUE(
        dw::ProjectCostingIO::saveEstimates(m_tempDir.string(), {est}));

    auto loaded = dw::ProjectCostingIO::loadEstimates(m_tempDir.string());
    ASSERT_EQ(loaded.size(), 1u);
    ASSERT_EQ(loaded[0].entries.size(), 1u);
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].estimatedTotal, 100.0);
    EXPECT_DOUBLE_EQ(loaded[0].entries[0].actualTotal, 120.0);
}
