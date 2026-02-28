// Integration tests for Direct Carve pipeline: heightmap -> analysis ->
// toolpath -> G-code export, plus job control state transitions.

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "core/carve/carve_job.h"
#include "core/carve/gcode_export.h"
#include "core/carve/heightmap.h"
#include "core/carve/model_fitter.h"
#include "core/carve/toolpath_generator.h"
#include "core/carve/toolpath_types.h"
#include "core/cnc/cnc_tool.h"
#include "core/mesh/vertex.h"
#include "core/types.h"

using namespace dw;
using namespace dw::carve;

namespace {

// Generate a simple flat plane mesh with a single pit in the center.
// The mesh is a regular grid with vertices lowered in the center to create
// a depression that needs clearing.
void buildTestMesh(std::vector<Vertex>& vertices, std::vector<u32>& indices,
                   Vec3& boundsMin, Vec3& boundsMax) {
    constexpr int GRID = 10;
    constexpr f32 SIZE = 50.0f; // 50mm x 50mm

    vertices.clear();
    indices.clear();

    for (int y = 0; y <= GRID; ++y) {
        for (int x = 0; x <= GRID; ++x) {
            f32 fx = static_cast<f32>(x) / GRID * SIZE;
            f32 fy = static_cast<f32>(y) / GRID * SIZE;

            // Create a pit in the center (cells 4-6)
            f32 fz = 0.0f;
            if (x >= 4 && x <= 6 && y >= 4 && y <= 6)
                fz = -3.0f; // 3mm deep pit

            Vec3 pos{fx, fy, fz};
            Vec3 normal{0.0f, 0.0f, 1.0f};
            vertices.push_back({pos, normal, {0, 0}});
        }
    }

    // Build triangle indices
    for (int y = 0; y < GRID; ++y) {
        for (int x = 0; x < GRID; ++x) {
            u32 tl = static_cast<u32>(y * (GRID + 1) + x);
            u32 tr = tl + 1;
            u32 bl = tl + static_cast<u32>(GRID + 1);
            u32 br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    boundsMin = {0.0f, 0.0f, -3.0f};
    boundsMax = {SIZE, SIZE, 0.0f};
}

VtdbToolGeometry makeVbitTool() {
    VtdbToolGeometry tool;
    tool.tool_type = VtdbToolType::VBit;
    tool.diameter = 6.35;        // 1/4" shank
    tool.included_angle = 90.0;
    tool.flat_diameter = 0.5;    // 0.5mm flat tip (typical V-bit)
    tool.num_flutes = 2;
    return tool;
}

} // namespace

// Full pipeline: heightmap -> analysis -> toolpath -> G-code export
TEST(CarveIntegration, FullPipeline) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    Vec3 boundsMin, boundsMax;
    buildTestMesh(vertices, indices, boundsMin, boundsMax);

    // Stage 1: Heightmap via CarveJob
    CarveJob job;
    ModelFitter fitter;
    fitter.setModelBounds(boundsMin, boundsMax);
    StockDimensions stock;
    stock.width = 60.0f;
    stock.height = 60.0f;
    stock.thickness = 10.0f;
    fitter.setStock(stock);

    FitParams fitParams;
    fitParams.scale = 1.0f;
    fitParams.depthMm = 5.0f;
    fitParams.offsetX = 5.0f;
    fitParams.offsetY = 5.0f;

    HeightmapConfig hmCfg;
    hmCfg.resolutionMm = 2.0f;
    job.startHeightmap(vertices, indices, fitter, fitParams, hmCfg);

    // Wait for async completion (poll)
    int timeout = 200;
    while (job.state() == CarveJobState::Computing && --timeout > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(job.state(), CarveJobState::Ready) << "Heightmap generation timed out";

    const auto& hm = job.heightmap();
    EXPECT_GT(hm.rows(), 0);
    EXPECT_GT(hm.cols(), 0);

    // Stage 2: Analysis
    auto finishTool = makeVbitTool();
    f32 toolAngle = static_cast<f32>(finishTool.included_angle);
    job.analyzeHeightmap(toolAngle);

    // Stage 3: Toolpath generation
    ToolpathConfig config;
    config.axis = ScanAxis::XOnly;
    config.direction = MillDirection::Alternating;
    config.stepoverPreset = StepoverPreset::Basic;
    config.safeZMm = 5.0f;
    config.feedRateMmMin = 1000.0f;
    config.plungeRateMmMin = 300.0f;

    job.generateToolpath(config, finishTool, nullptr);
    const auto& tp = job.toolpath();

    EXPECT_GT(tp.finishing.points.size(), 0u);
    EXPECT_GT(tp.finishing.lineCount, 0);
    EXPECT_GT(tp.totalTimeSec, 0.0f);
    EXPECT_GT(tp.totalLineCount, 0);

    // Stage 4: G-code export (in-memory)
    std::string gcode = generateGcode(tp, config, "test_model", "V-bit 90deg");
    EXPECT_FALSE(gcode.empty());

    // Verify G-code structure
    EXPECT_NE(gcode.find("G90"), std::string::npos) << "Missing absolute mode";
    EXPECT_NE(gcode.find("G21"), std::string::npos) << "Missing metric mode";
    EXPECT_NE(gcode.find("G0"), std::string::npos) << "Missing rapid moves";
    EXPECT_NE(gcode.find("G1"), std::string::npos) << "Missing feed moves";
    EXPECT_NE(gcode.find("M30"), std::string::npos) << "Missing program end";

    // Line count should match toolpath expectation
    int lineCount = 0;
    for (char c : gcode)
        if (c == '\n') ++lineCount;
    EXPECT_GT(lineCount, 5) << "G-code too short";
}

// Verify that cancelling a CarveJob during heightmap computation works cleanly
TEST(CarveIntegration, CancelDuringHeightmap) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    Vec3 boundsMin, boundsMax;
    buildTestMesh(vertices, indices, boundsMin, boundsMax);

    CarveJob job;
    ModelFitter fitter;
    fitter.setModelBounds(boundsMin, boundsMax);
    StockDimensions stock{60.0f, 60.0f, 10.0f};
    fitter.setStock(stock);

    FitParams fitParams;
    fitParams.scale = 1.0f;
    fitParams.depthMm = 5.0f;

    HeightmapConfig hmCfg;
    hmCfg.resolutionMm = 0.1f; // Fine resolution to give time to cancel
    job.startHeightmap(vertices, indices, fitter, fitParams, hmCfg);

    // Cancel immediately
    job.cancel();

    // Wait for state to settle
    int timeout = 100;
    while (job.state() == CarveJobState::Computing && --timeout > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Job should not be in Computing state after cancel
    EXPECT_NE(job.state(), CarveJobState::Computing);
}

// Verify toolpath produces valid G-code lines (no empty, no NaN coordinates)
TEST(CarveIntegration, GcodeLineValidity) {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    Vec3 boundsMin, boundsMax;
    buildTestMesh(vertices, indices, boundsMin, boundsMax);

    CarveJob job;
    ModelFitter fitter;
    fitter.setModelBounds(boundsMin, boundsMax);
    StockDimensions stock{60.0f, 60.0f, 10.0f};
    fitter.setStock(stock);

    FitParams fitParams;
    fitParams.scale = 1.0f;
    fitParams.depthMm = 5.0f;
    fitParams.offsetX = 5.0f;
    fitParams.offsetY = 5.0f;

    HeightmapConfig hmCfg;
    hmCfg.resolutionMm = 2.0f;
    job.startHeightmap(vertices, indices, fitter, fitParams, hmCfg);

    int timeout = 200;
    while (job.state() == CarveJobState::Computing && --timeout > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(job.state(), CarveJobState::Ready);

    auto finishTool = makeVbitTool();
    job.analyzeHeightmap(static_cast<f32>(finishTool.included_angle));

    ToolpathConfig config;
    config.feedRateMmMin = 1000.0f;
    config.plungeRateMmMin = 300.0f;
    config.safeZMm = 5.0f;
    job.generateToolpath(config, finishTool, nullptr);

    std::string gcode = generateGcode(job.toolpath(), config, "test", "vbit");

    // Parse each line — no NaN, no empty meaningful lines
    std::istringstream stream(gcode);
    std::string line;
    int lineNum = 0;
    while (std::getline(stream, line)) {
        ++lineNum;
        // Skip comments and empty lines
        if (line.empty() || line[0] == '(' || line[0] == ';') continue;

        // No NaN in any coordinate
        EXPECT_EQ(line.find("nan"), std::string::npos)
            << "NaN found on line " << lineNum << ": " << line;
        EXPECT_EQ(line.find("inf"), std::string::npos)
            << "Inf found on line " << lineNum << ": " << line;
    }
    EXPECT_GT(lineNum, 0);
}
