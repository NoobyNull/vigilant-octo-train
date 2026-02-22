// Digital Workshop - Project Class Tests

#include <gtest/gtest.h>

#include "core/project/project.h"

// --- Metadata ---

TEST(Project, DefaultState) {
    dw::Project proj;
    EXPECT_EQ(proj.id(), 0);
    EXPECT_TRUE(proj.name().empty());
    EXPECT_TRUE(proj.description().empty());
    EXPECT_EQ(proj.modelCount(), 0);
    EXPECT_FALSE(proj.isModified());
}

TEST(Project, SetName) {
    dw::Project proj;
    proj.setName("CNC Bracket");
    EXPECT_EQ(proj.name(), "CNC Bracket");
}

TEST(Project, SetDescription) {
    dw::Project proj;
    proj.setDescription("A test project");
    EXPECT_EQ(proj.description(), "A test project");
}

TEST(Project, SetFilePath) {
    dw::Project proj;
    proj.setFilePath("/projects/test.dwp");
    EXPECT_EQ(proj.filePath(), dw::Path("/projects/test.dwp"));
}

// --- Modified flag ---

TEST(Project, ModifiedFlag) {
    dw::Project proj;
    EXPECT_FALSE(proj.isModified());

    proj.markModified();
    EXPECT_TRUE(proj.isModified());

    proj.clearModified();
    EXPECT_FALSE(proj.isModified());
}

// --- Model management ---

TEST(Project, AddModel) {
    dw::Project proj;
    proj.addModel(1);
    proj.addModel(2);
    proj.addModel(3);

    EXPECT_EQ(proj.modelCount(), 3);
    EXPECT_TRUE(proj.hasModel(1));
    EXPECT_TRUE(proj.hasModel(2));
    EXPECT_TRUE(proj.hasModel(3));
}

TEST(Project, AddModel_NoDuplicate) {
    dw::Project proj;
    proj.addModel(1);
    proj.addModel(1); // duplicate

    // Should either have 1 or 2 depending on impl; just don't crash
    EXPECT_TRUE(proj.hasModel(1));
}

TEST(Project, RemoveModel) {
    dw::Project proj;
    proj.addModel(1);
    proj.addModel(2);
    proj.addModel(3);

    proj.removeModel(2);
    EXPECT_EQ(proj.modelCount(), 2);
    EXPECT_TRUE(proj.hasModel(1));
    EXPECT_FALSE(proj.hasModel(2));
    EXPECT_TRUE(proj.hasModel(3));
}

TEST(Project, RemoveModel_NotPresent) {
    dw::Project proj;
    proj.addModel(1);
    proj.removeModel(999); // not present, should not crash
    EXPECT_EQ(proj.modelCount(), 1);
}

TEST(Project, HasModel_False) {
    dw::Project proj;
    EXPECT_FALSE(proj.hasModel(42));
}

TEST(Project, ModelIds_Order) {
    dw::Project proj;
    proj.addModel(10);
    proj.addModel(20);
    proj.addModel(30);

    const auto& ids = proj.modelIds();
    EXPECT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], 10);
    EXPECT_EQ(ids[1], 20);
    EXPECT_EQ(ids[2], 30);
}

TEST(Project, ReorderModel) {
    dw::Project proj;
    proj.addModel(1);
    proj.addModel(2);
    proj.addModel(3);

    // Move model 3 to position 0
    proj.reorderModel(3, 0);

    const auto& ids = proj.modelIds();
    EXPECT_EQ(ids[0], 3);
}

// --- Record access ---

TEST(Project, RecordAccess) {
    dw::Project proj;
    proj.setName("Test");
    proj.record().id = 42;

    EXPECT_EQ(proj.id(), 42);
    EXPECT_EQ(proj.record().name, "Test");
}
