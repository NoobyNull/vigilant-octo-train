// Digital Workshop - Schema Tests

#include <gtest/gtest.h>

#include "core/database/database.h"
#include "core/database/schema.h"

TEST(Schema, Initialize_FreshDatabase) {
    dw::Database db;
    db.open(":memory:");

    EXPECT_FALSE(dw::Schema::isInitialized(db));
    EXPECT_TRUE(dw::Schema::initialize(db));
    EXPECT_TRUE(dw::Schema::isInitialized(db));
}

TEST(Schema, GetVersion_AfterInit) {
    dw::Database db;
    db.open(":memory:");
    dw::Schema::initialize(db);

    EXPECT_EQ(dw::Schema::getVersion(db), 1);
}

TEST(Schema, GetVersion_BeforeInit) {
    dw::Database db;
    db.open(":memory:");

    EXPECT_EQ(dw::Schema::getVersion(db), 0);
}

TEST(Schema, DoubleInit_Idempotent) {
    dw::Database db;
    db.open(":memory:");

    EXPECT_TRUE(dw::Schema::initialize(db));
    EXPECT_TRUE(dw::Schema::initialize(db));
    EXPECT_EQ(dw::Schema::getVersion(db), 1);
}

TEST(Schema, TablesCreated) {
    dw::Database db;
    db.open(":memory:");
    dw::Schema::initialize(db);

    // Verify models table exists
    auto stmt1 = db.prepare(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='models'");
    EXPECT_TRUE(stmt1.step());

    // Verify projects table exists
    auto stmt2 = db.prepare(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='projects'");
    EXPECT_TRUE(stmt2.step());

    // Verify junction table exists
    auto stmt3 = db.prepare(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='project_models'");
    EXPECT_TRUE(stmt3.step());
}

TEST(Schema, IndexesCreated) {
    dw::Database db;
    db.open(":memory:");
    dw::Schema::initialize(db);

    auto stmt = db.prepare(
        "SELECT name FROM sqlite_master WHERE type='index' AND name='idx_models_hash'");
    EXPECT_TRUE(stmt.step());
}
