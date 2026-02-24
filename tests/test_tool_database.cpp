// Digital Workshop - Tool Database Tests

#include <gtest/gtest.h>

#include "core/database/tool_database.h"

namespace {

// Fixture: provides an in-memory ToolDatabase per test
class ToolDatabaseTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_toolDb.open(":memory:"));
    }

    dw::ToolDatabase m_toolDb;
};

}  // namespace

// --- Schema ---

TEST_F(ToolDatabaseTest, Schema_TablesCreated) {
    auto& db = m_toolDb.database();

    // Check version table
    auto stmt1 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='version'");
    EXPECT_TRUE(stmt1.step());

    // Check material table
    auto stmt2 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='material'");
    EXPECT_TRUE(stmt2.step());

    // Check machine table
    auto stmt3 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='machine'");
    EXPECT_TRUE(stmt3.step());

    // Check tool_geometry table
    auto stmt4 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='tool_geometry'");
    EXPECT_TRUE(stmt4.step());

    // Check tool_cutting_data table
    auto stmt5 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='tool_cutting_data'");
    EXPECT_TRUE(stmt5.step());

    // Check tool_entity table
    auto stmt6 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='tool_entity'");
    EXPECT_TRUE(stmt6.step());

    // Check tool_tree_entry table
    auto stmt7 = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='tool_tree_entry'");
    EXPECT_TRUE(stmt7.step());
}

TEST_F(ToolDatabaseTest, Schema_VersionSet) {
    auto& db = m_toolDb.database();
    auto stmt = db.prepare("SELECT version FROM version");
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.getInt(0), 1);
}

// --- Basic operations ---

TEST_F(ToolDatabaseTest, CanOpenDatabase) {
    dw::ToolDatabase db;
    EXPECT_TRUE(db.open(":memory:"));
}

// --- CRUD Operations ---

TEST_F(ToolDatabaseTest, Material_InsertAndFind) {
    dw::VtdbMaterial mat;
    mat.id = "test-mat-001";
    mat.name = "Aluminum";

    EXPECT_TRUE(m_toolDb.insertMaterial(mat));

    auto found = m_toolDb.findMaterialById("test-mat-001");
    ASSERT_TRUE(found);
    EXPECT_EQ(found->name, "Aluminum");
}

TEST_F(ToolDatabaseTest, Machine_InsertAndFind) {
    dw::VtdbMachine mach;
    mach.id = "test-mach-001";
    mach.name = "Shapeoko";
    mach.make = "Carbide3D";
    mach.model = "Shapeoko 5";

    EXPECT_TRUE(m_toolDb.insertMachine(mach));

    auto found = m_toolDb.findMachineById("test-mach-001");
    ASSERT_TRUE(found);
    EXPECT_EQ(found->name, "Shapeoko");
}

TEST_F(ToolDatabaseTest, ToolGeometry_InsertAndFind) {
    dw::VtdbToolGeometry geom;
    geom.id = "test-geom-001";
    geom.name_format = "EndMill_1/4in";
    geom.tool_type = dw::VtdbToolType::EndMill;
    geom.units = dw::VtdbUnits::Imperial;
    geom.diameter = 0.25;
    geom.num_flutes = 2;

    EXPECT_TRUE(m_toolDb.insertGeometry(geom));

    auto found = m_toolDb.findGeometryById("test-geom-001");
    ASSERT_TRUE(found);
    EXPECT_EQ(found->diameter, 0.25);
}

// --- Import Tests ---

TEST_F(ToolDatabaseTest, Import_FromVectricDatabase) {
    // Test importing from the real Vectric database if it exists
    const char* vectricPath = "/home/matthew/Downloads/IDC Woodcraft Vectric Tool Database Library Update rev 25-12.vtdb";

    // Create a new in-memory database to import into
    dw::ToolDatabase importDb;
    ASSERT_TRUE(importDb.open(":memory:"));

    // Attempt import (should succeed if file exists, gracefully fail if not)
    int imported = importDb.importFromVtdb(vectricPath);

    // If import succeeded, verify data was imported
    if (imported > 0) {
        auto materials = importDb.findAllMaterials();
        auto machines = importDb.findAllMachines();
        auto geometries = importDb.findAllGeometries();

        EXPECT_GT(materials.size(), 0);
        EXPECT_GT(machines.size(), 0);
        EXPECT_GT(geometries.size(), 0);
        EXPECT_EQ(imported, static_cast<int>(geometries.size()));
    }
}

TEST_F(ToolDatabaseTest, TreeEntry_InsertAndFind) {
    // First create a geometry
    dw::VtdbToolGeometry geom;
    geom.id = "test-geom-tree";
    geom.name_format = "BallNose_1/8in";
    geom.tool_type = dw::VtdbToolType::BallNose;
    EXPECT_TRUE(m_toolDb.insertGeometry(geom));

    // Create a tree entry
    dw::VtdbTreeEntry entry;
    entry.id = "test-tree-001";
    entry.parent_group_id = "";  // root entry
    entry.sibling_order = 0;
    entry.tool_geometry_id = "test-geom-tree";
    entry.name = "Ball Nose 1/8\"";
    entry.notes = "For detail work";
    entry.expanded = 0;

    EXPECT_TRUE(m_toolDb.insertTreeEntry(entry));

    // Find root entries
    auto roots = m_toolDb.findRootEntries();
    EXPECT_GT(roots.size(), 0);
}
