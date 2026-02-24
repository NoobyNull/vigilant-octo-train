// Digital Workshop - CNC Tool Repository Tests

#include <gtest/gtest.h>

#include "core/database/cnc_tool_repository.h"
#include "core/database/database.h"
#include "core/database/material_repository.h"
#include "core/database/schema.h"

namespace {

// Fixture: in-memory DB with schema initialized
class CncToolRepoTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_db.open(":memory:"));
        ASSERT_TRUE(dw::Schema::initialize(m_db));
        m_repo = std::make_unique<dw::CncToolRepository>(m_db);
        m_matRepo = std::make_unique<dw::MaterialRepository>(m_db);
    }

    dw::CncToolRecord makeTool(const std::string& name, dw::CncToolType type) {
        dw::CncToolRecord rec;
        rec.name = name;
        rec.type = type;
        rec.diameter = 0.25;
        rec.fluteCount = 2;
        rec.maxRPM = 24000.0;
        rec.maxDOC = 0.5;
        rec.shankDiameter = 0.25;
        rec.notes = "test tool";
        return rec;
    }

    dw::MaterialRecord makeMaterial(const std::string& name) {
        dw::MaterialRecord rec;
        rec.name = name;
        rec.category = dw::MaterialCategory::Hardwood;
        rec.archivePath = "/materials/" + name;
        rec.jankaHardness = 1000.0f;
        rec.feedRate = 100.0f;
        rec.spindleSpeed = 18000.0f;
        rec.depthOfCut = 0.125f;
        rec.costPerBoardFoot = 5.50f;
        rec.grainDirectionDeg = 45.0f;
        rec.thumbnailPath = "/thumbnails/" + name + ".png";
        return rec;
    }

    dw::Database m_db;
    std::unique_ptr<dw::CncToolRepository> m_repo;
    std::unique_ptr<dw::MaterialRepository> m_matRepo;
};

} // namespace

// --- Insert ---

TEST_F(CncToolRepoTest, Insert_ReturnsId) {
    auto rec = makeTool("1/4\" Flat End Mill", dw::CncToolType::FlatEndMill);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}

TEST_F(CncToolRepoTest, Insert_MultipleTools) {
    auto t1 = makeTool("1/4\" Flat", dw::CncToolType::FlatEndMill);
    auto t2 = makeTool("1/8\" Ball Nose", dw::CncToolType::BallNose);

    auto id1 = m_repo->insert(t1);
    auto id2 = m_repo->insert(t2);

    EXPECT_TRUE(id1.has_value());
    EXPECT_TRUE(id2.has_value());
    EXPECT_NE(id1.value(), id2.value());
}

// --- FindById ---

TEST_F(CncToolRepoTest, FindById_Found) {
    auto rec = makeTool("V-Bit 60deg", dw::CncToolType::VBit);
    rec.diameter = 0.5;
    rec.fluteCount = 1;
    rec.maxRPM = 18000.0;
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "V-Bit 60deg");
    EXPECT_EQ(found->type, dw::CncToolType::VBit);
    EXPECT_DOUBLE_EQ(found->diameter, 0.5);
    EXPECT_EQ(found->fluteCount, 1);
    EXPECT_DOUBLE_EQ(found->maxRPM, 18000.0);
}

TEST_F(CncToolRepoTest, FindById_NotFound) {
    auto found = m_repo->findById(999);
    EXPECT_FALSE(found.has_value());
}

// --- FindAll ---

TEST_F(CncToolRepoTest, FindAll_Empty) {
    auto all = m_repo->findAll();
    EXPECT_TRUE(all.empty());
}

TEST_F(CncToolRepoTest, FindAll_OrderedByName) {
    m_repo->insert(makeTool("Surfacing Bit", dw::CncToolType::SurfacingBit));
    m_repo->insert(makeTool("Ball Nose", dw::CncToolType::BallNose));
    m_repo->insert(makeTool("Flat End Mill", dw::CncToolType::FlatEndMill));

    auto all = m_repo->findAll();
    ASSERT_EQ(all.size(), 3);
    EXPECT_EQ(all[0].name, "Ball Nose");
    EXPECT_EQ(all[1].name, "Flat End Mill");
    EXPECT_EQ(all[2].name, "Surfacing Bit");
}

// --- FindByType ---

TEST_F(CncToolRepoTest, FindByType_Filters) {
    m_repo->insert(makeTool("Flat 1/4", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("Flat 1/8", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("Ball 1/4", dw::CncToolType::BallNose));

    auto flats = m_repo->findByType(dw::CncToolType::FlatEndMill);
    ASSERT_EQ(flats.size(), 2);

    auto balls = m_repo->findByType(dw::CncToolType::BallNose);
    ASSERT_EQ(balls.size(), 1);
    EXPECT_EQ(balls[0].name, "Ball 1/4");
}

TEST_F(CncToolRepoTest, FindByType_NoneFound) {
    m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto vbits = m_repo->findByType(dw::CncToolType::VBit);
    EXPECT_TRUE(vbits.empty());
}

// --- FindByName ---

TEST_F(CncToolRepoTest, FindByName_PartialMatch) {
    m_repo->insert(makeTool("1/4\" Flat End Mill", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("1/8\" Flat End Mill", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("Ball Nose", dw::CncToolType::BallNose));

    auto results = m_repo->findByName("Flat");
    ASSERT_EQ(results.size(), 2);
}

TEST_F(CncToolRepoTest, FindByName_NotFound) {
    m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto results = m_repo->findByName("Drill");
    EXPECT_TRUE(results.empty());
}

// --- Update ---

TEST_F(CncToolRepoTest, Update_Success) {
    auto rec = makeTool("Test Tool", dw::CncToolType::FlatEndMill);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findById(id.value());
    ASSERT_TRUE(found.has_value());

    found->name = "Updated Tool";
    found->type = dw::CncToolType::BallNose;
    found->diameter = 0.125;

    EXPECT_TRUE(m_repo->update(*found));

    auto updated = m_repo->findById(id.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Tool");
    EXPECT_EQ(updated->type, dw::CncToolType::BallNose);
    EXPECT_DOUBLE_EQ(updated->diameter, 0.125);
}

// --- Remove ---

TEST_F(CncToolRepoTest, Remove_Success) {
    auto rec = makeTool("Delete Me", dw::CncToolType::FlatEndMill);
    auto id = m_repo->insert(rec);
    ASSERT_TRUE(id.has_value());

    EXPECT_TRUE(m_repo->remove(id.value()));
    EXPECT_FALSE(m_repo->findById(id.value()).has_value());
}

TEST_F(CncToolRepoTest, Remove_NonExistent) {
    EXPECT_FALSE(m_repo->remove(999));
}

// --- Count ---

TEST_F(CncToolRepoTest, Count_Empty) {
    EXPECT_EQ(m_repo->count(), 0);
}

TEST_F(CncToolRepoTest, Count_Multiple) {
    m_repo->insert(makeTool("A", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("B", dw::CncToolType::BallNose));
    m_repo->insert(makeTool("C", dw::CncToolType::VBit));

    EXPECT_EQ(m_repo->count(), 3);
}

TEST_F(CncToolRepoTest, Count_AfterRemove) {
    auto id1 = m_repo->insert(makeTool("A", dw::CncToolType::FlatEndMill));
    m_repo->insert(makeTool("B", dw::CncToolType::BallNose));

    EXPECT_EQ(m_repo->count(), 2);
    m_repo->remove(id1.value());
    EXPECT_EQ(m_repo->count(), 1);
}

// --- Junction: InsertParams ---

TEST_F(CncToolRepoTest, InsertParams_ReturnsId) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));
    ASSERT_TRUE(toolId.has_value());
    ASSERT_TRUE(matId.has_value());

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;
    params.spindleSpeed = 18000.0;
    params.depthOfCut = 0.125;
    params.chipLoad = 0.003;

    auto id = m_repo->insertParams(params);
    ASSERT_TRUE(id.has_value());
    EXPECT_GT(id.value(), 0);
}

TEST_F(CncToolRepoTest, InsertParams_Upsert) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;
    params.spindleSpeed = 18000.0;
    params.depthOfCut = 0.125;
    params.chipLoad = 0.003;

    m_repo->insertParams(params);

    // Upsert with different values
    params.feedRate = 80.0;
    params.spindleSpeed = 20000.0;
    auto id2 = m_repo->insertParams(params);
    ASSERT_TRUE(id2.has_value());

    auto found = m_repo->findParams(toolId.value(), matId.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(found->feedRate, 80.0);
    EXPECT_DOUBLE_EQ(found->spindleSpeed, 20000.0);
}

// --- Junction: FindParams ---

TEST_F(CncToolRepoTest, FindParams_Found) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Walnut"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 50.0;
    params.spindleSpeed = 16000.0;
    params.depthOfCut = 0.1;
    params.chipLoad = 0.002;

    m_repo->insertParams(params);

    auto found = m_repo->findParams(toolId.value(), matId.value());
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->toolId, toolId.value());
    EXPECT_EQ(found->materialId, matId.value());
    EXPECT_DOUBLE_EQ(found->feedRate, 50.0);
    EXPECT_DOUBLE_EQ(found->chipLoad, 0.002);
}

TEST_F(CncToolRepoTest, FindParams_NotFound) {
    auto found = m_repo->findParams(999, 999);
    EXPECT_FALSE(found.has_value());
}

// --- Junction: FindParamsForTool ---

TEST_F(CncToolRepoTest, FindParamsForTool_Multiple) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto mat1 = m_matRepo->insert(makeMaterial("Oak"));
    auto mat2 = m_matRepo->insert(makeMaterial("Pine"));

    dw::ToolMaterialParams p1;
    p1.toolId = toolId.value();
    p1.materialId = mat1.value();
    p1.feedRate = 60.0;
    m_repo->insertParams(p1);

    dw::ToolMaterialParams p2;
    p2.toolId = toolId.value();
    p2.materialId = mat2.value();
    p2.feedRate = 80.0;
    m_repo->insertParams(p2);

    auto results = m_repo->findParamsForTool(toolId.value());
    EXPECT_EQ(results.size(), 2);
}

// --- Junction: FindParamsForMaterial ---

TEST_F(CncToolRepoTest, FindParamsForMaterial_Multiple) {
    auto tool1 = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto tool2 = m_repo->insert(makeTool("Ball", dw::CncToolType::BallNose));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams p1;
    p1.toolId = tool1.value();
    p1.materialId = matId.value();
    p1.feedRate = 60.0;
    m_repo->insertParams(p1);

    dw::ToolMaterialParams p2;
    p2.toolId = tool2.value();
    p2.materialId = matId.value();
    p2.feedRate = 40.0;
    m_repo->insertParams(p2);

    auto results = m_repo->findParamsForMaterial(matId.value());
    EXPECT_EQ(results.size(), 2);
}

// --- Junction: UpdateParams ---

TEST_F(CncToolRepoTest, UpdateParams_Success) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;
    params.spindleSpeed = 18000.0;
    params.depthOfCut = 0.125;
    params.chipLoad = 0.003;

    auto id = m_repo->insertParams(params);
    ASSERT_TRUE(id.has_value());

    auto found = m_repo->findParams(toolId.value(), matId.value());
    ASSERT_TRUE(found.has_value());
    found->feedRate = 90.0;
    found->chipLoad = 0.005;

    EXPECT_TRUE(m_repo->updateParams(*found));

    auto updated = m_repo->findParams(toolId.value(), matId.value());
    ASSERT_TRUE(updated.has_value());
    EXPECT_DOUBLE_EQ(updated->feedRate, 90.0);
    EXPECT_DOUBLE_EQ(updated->chipLoad, 0.005);
}

// --- Junction: RemoveParams ---

TEST_F(CncToolRepoTest, RemoveParams_Success) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;

    m_repo->insertParams(params);

    EXPECT_TRUE(m_repo->removeParams(toolId.value(), matId.value()));
    EXPECT_FALSE(m_repo->findParams(toolId.value(), matId.value()).has_value());
}

TEST_F(CncToolRepoTest, RemoveParams_NonExistent) {
    EXPECT_FALSE(m_repo->removeParams(999, 999));
}

// --- CASCADE Deletes ---

TEST_F(CncToolRepoTest, CascadeDelete_ToolRemoval) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;

    m_repo->insertParams(params);

    // Verify params exist
    EXPECT_TRUE(m_repo->findParams(toolId.value(), matId.value()).has_value());

    // Delete tool — params should be cascade-deleted
    m_repo->remove(toolId.value());

    EXPECT_FALSE(m_repo->findParams(toolId.value(), matId.value()).has_value());
    auto forTool = m_repo->findParamsForTool(toolId.value());
    EXPECT_TRUE(forTool.empty());
}

TEST_F(CncToolRepoTest, CascadeDelete_MaterialRemoval) {
    auto toolId = m_repo->insert(makeTool("Flat", dw::CncToolType::FlatEndMill));
    auto matId = m_matRepo->insert(makeMaterial("Oak"));

    dw::ToolMaterialParams params;
    params.toolId = toolId.value();
    params.materialId = matId.value();
    params.feedRate = 60.0;

    m_repo->insertParams(params);

    // Verify params exist
    EXPECT_TRUE(m_repo->findParams(toolId.value(), matId.value()).has_value());

    // Delete material — params should be cascade-deleted
    m_matRepo->remove(matId.value());

    EXPECT_FALSE(m_repo->findParams(toolId.value(), matId.value()).has_value());
    auto forMaterial = m_repo->findParamsForMaterial(matId.value());
    EXPECT_TRUE(forMaterial.empty());
}
