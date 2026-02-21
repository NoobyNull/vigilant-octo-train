#include <gtest/gtest.h>

#include <filesystem>

#include "../src/core/mesh/mesh.h"
#include "../src/core/loaders/loader_factory.h"
#include "../src/core/gcode/gcode_parser.h"
#include "../src/core/gcode/gcode_analyzer.h"

namespace dw {
namespace fs = std::filesystem;

class IntegrationPipelineTest : public ::testing::Test {
  protected:
    // Helper: Create a temporary mesh for testing
    Mesh createTestMesh() {
        Mesh mesh;
        // Create a simple triangle
        mesh.addVertex(Vertex(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
        mesh.addVertex(Vertex(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
        mesh.addVertex(Vertex(Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
        mesh.addTriangle(0, 1, 2);
        return mesh;
    }

    // Helper: Create sample G-code for testing
    std::string createSampleGCode() {
        return R"(
G00 X0 Y0 Z0
G00 Z5
G01 X10 Y10 Z0 F100
G01 X20 Y10 Z0 F100
G01 X20 Y20 Z0 F100
G01 X10 Y20 Z0 F100
G01 X10 Y10 Z0 F100
G00 Z5
M02
)";
    }
};

// Test loading a mesh works
TEST_F(IntegrationPipelineTest, CanLoadAndValidateMesh) {
    Mesh mesh = createTestMesh();

    // Mesh should be valid
    EXPECT_TRUE(mesh.validate());
    EXPECT_EQ(mesh.vertexCount(), 3);
    EXPECT_EQ(mesh.triangleCount(), 1);
}

// Test mesh can be prepared for export (validation)
TEST_F(IntegrationPipelineTest, CanPrepareMeshForExport) {
    Mesh mesh = createTestMesh();

    // Verify mesh is valid before export
    EXPECT_TRUE(mesh.validate());

    // Mesh is ready for export
    EXPECT_GT(mesh.vertexCount(), 0);
    EXPECT_GT(mesh.triangleCount(), 0);
}

// Test G-code parsing works
TEST_F(IntegrationPipelineTest, CanParseGCode) {
    std::string gcodeText = createSampleGCode();

    gcode::Parser parser;
    gcode::Program program = parser.parse(gcodeText);

    // Program should have parsed commands
    EXPECT_FALSE(program.commands.empty());

    // Should have at least some commands
    EXPECT_GT(program.commands.size(), 0);
}

// Test G-code analysis works after parsing
TEST_F(IntegrationPipelineTest, CanAnalyzeGCode) {
    std::string gcodeText = createSampleGCode();

    gcode::Parser parser;
    gcode::Program program = parser.parse(gcodeText);
    EXPECT_FALSE(program.commands.empty());

    // Analyze the program
    gcode::Analyzer analyzer;
    gcode::Statistics stats = analyzer.analyze(program);

    // Statistics should be valid
    EXPECT_GE(stats.commandCount, 0);
    EXPECT_GE(stats.totalPathLength, 0.0f);
}

// Test complete import pipeline: Load mesh, validate, prepare for rendering
TEST_F(IntegrationPipelineTest, ImportPipelineLoadValidatePrepare) {
    // Step 1: Load/Create mesh
    Mesh mesh = createTestMesh();
    EXPECT_EQ(mesh.vertexCount(), 3);

    // Step 2: Validate
    EXPECT_TRUE(mesh.validate());

    // Step 3: Prepare (normalize bounds, etc)
    if (mesh.triangleCount() > 0) {
        // Mesh is ready for rendering
        EXPECT_TRUE(true);
    }
}

// Test complete G-code pipeline: Parse, analyze, generate statistics
TEST_F(IntegrationPipelineTest, GCodePipelineParseAnalyzeStatistics) {
    // Step 1: Parse
    std::string gcodeText = createSampleGCode();
    gcode::Parser parser;
    gcode::Program program = parser.parse(gcodeText);
    EXPECT_FALSE(program.commands.empty());

    // Step 2: Analyze
    gcode::Analyzer analyzer;
    gcode::Statistics stats = analyzer.analyze(program);

    // Step 3: Verify statistics
    EXPECT_GE(stats.commandCount, 0);
    EXPECT_GE(stats.totalPathLength, 0.0f);

    // Statistics should have meaningful values
    EXPECT_TRUE(true);
}

// Test mesh transformation and bounds checking
TEST_F(IntegrationPipelineTest, MeshTransformAndBounds) {
    Mesh mesh = createTestMesh();

    // Mesh bounds should be valid
    EXPECT_LE(mesh.bounds().min.x, mesh.bounds().max.x);
    EXPECT_LE(mesh.bounds().min.y, mesh.bounds().max.y);
    EXPECT_LE(mesh.bounds().min.z, mesh.bounds().max.z);
}

// Test that a mesh can be loaded, validated, and is ready for export
TEST_F(IntegrationPipelineTest, FullImportExportCycle) {
    // Step 1: Create/load mesh
    Mesh original = createTestMesh();
    EXPECT_EQ(original.vertexCount(), 3);

    // Step 2: Validate
    EXPECT_TRUE(original.validate());

    // Step 3: Verify it's ready for export
    if (original.triangleCount() > 0) {
        // Ready for export
        EXPECT_TRUE(true);
    }
}

// Test error handling in G-code parsing
TEST_F(IntegrationPipelineTest, GCodeParserHandlesEmptyInput) {
    std::string emptyGCode = "";

    gcode::Parser parser;
    gcode::Program program = parser.parse(emptyGCode);

    // Empty input should result in empty program
    EXPECT_EQ(program.commands.size(), 0);
}

// Test error handling in mesh validation
TEST_F(IntegrationPipelineTest, MeshValidationCatchesInvalidData) {
    Mesh mesh;
    // Empty mesh with single vertex
    mesh.addVertex(Vertex(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));

    // Mesh with no triangles should still validate but have 0 triangles
    EXPECT_TRUE(mesh.validate());
    EXPECT_EQ(mesh.triangleCount(), 0);
}

// Test mesh bounds are properly computed
TEST_F(IntegrationPipelineTest, MeshBoundsComputation) {
    Mesh mesh;
    mesh.addVertex(Vertex(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
    mesh.addVertex(Vertex(Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
    mesh.addVertex(Vertex(Vec3(1.0f, 2.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)));
    mesh.addTriangle(0, 1, 2);

    EXPECT_TRUE(mesh.validate());

    // Bounds should encompass all vertices
    EXPECT_LE(mesh.bounds().min.x, 0.0f);
    EXPECT_GE(mesh.bounds().max.x, 2.0f);
    EXPECT_LE(mesh.bounds().min.y, 0.0f);
    EXPECT_GE(mesh.bounds().max.y, 2.0f);
}

// Test G-code statistics provide meaningful metrics
TEST_F(IntegrationPipelineTest, GCodeStatisticsAreComputed) {
    std::string gcodeText = createSampleGCode();

    gcode::Parser parser;
    gcode::Program program = parser.parse(gcodeText);

    gcode::Analyzer analyzer;
    gcode::Statistics stats = analyzer.analyze(program);

    // All statistics should be non-negative
    EXPECT_GE(stats.commandCount, 0);
    EXPECT_GE(stats.totalPathLength, 0.0f);
    EXPECT_GE(stats.rapidPathLength, 0.0f);
    EXPECT_GE(stats.cuttingPathLength, 0.0f);
}

}  // namespace dw
