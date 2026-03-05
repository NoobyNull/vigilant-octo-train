// Digital Workshop - CarveJob Tests

#include <gtest/gtest.h>

#include "core/carve/carve_job.h"

#include <chrono>
#include <thread>

using namespace dw;
using namespace dw::carve;

namespace {

// Helper: create a flat quad mesh
void makeFlatMesh(f32 size, f32 z,
                  std::vector<Vertex>& verts,
                  std::vector<u32>& indices)
{
    verts.clear();
    indices.clear();
    verts.push_back(Vertex{Vec3{0.0f, 0.0f, z}});
    verts.push_back(Vertex{Vec3{size, 0.0f, z}});
    verts.push_back(Vertex{Vec3{size, size, z}});
    verts.push_back(Vertex{Vec3{0.0f, size, z}});
    indices = {0, 1, 2, 0, 2, 3};
}

} // namespace

TEST(CarveJob, InitialState) {
    CarveJob job;
    EXPECT_EQ(job.state(), CarveJobState::Idle);
    EXPECT_FLOAT_EQ(job.progress(), 0.0f);
    EXPECT_TRUE(job.heightmap().empty());
}

TEST(CarveJob, ComputeSimpleMesh) {
    CarveJob job;

    std::vector<Vertex> verts;
    std::vector<u32> indices;
    makeFlatMesh(10.0f, 5.0f, verts, indices);

    ModelFitter fitter;
    fitter.setModelBounds(Vec3{0, 0, 0}, Vec3{10, 10, 5});

    StockDimensions stock;
    stock.width = 10.0f;
    stock.height = 10.0f;
    stock.thickness = 5.0f;
    fitter.setStock(stock);

    FitParams fp;
    fp.scale = 1.0f;

    HeightmapConfig hcfg;
    hcfg.resolutionMm = 1.0f;

    job.startHeightmap(verts, indices, fitter, fp, hcfg);

    // Wait for completion (with timeout)
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (job.state() == CarveJobState::Computing) {
        if (std::chrono::steady_clock::now() > deadline) {
            FAIL() << "CarveJob timed out";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(job.state(), CarveJobState::Ready);
    EXPECT_FALSE(job.heightmap().empty());
    EXPECT_TRUE(job.errorMessage().empty());
}

TEST(CarveJob, CancelMidCompute) {
    CarveJob job;

    // Create a mesh with enough resolution to take some time
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    makeFlatMesh(100.0f, 5.0f, verts, indices);

    ModelFitter fitter;
    fitter.setModelBounds(Vec3{0, 0, 0}, Vec3{100, 100, 5});

    StockDimensions stock;
    stock.width = 100.0f;
    stock.height = 100.0f;
    stock.thickness = 5.0f;
    fitter.setStock(stock);

    FitParams fp;
    fp.scale = 1.0f;

    HeightmapConfig hcfg;
    hcfg.resolutionMm = 0.01f; // Very fine grid to ensure it takes time

    job.startHeightmap(verts, indices, fitter, fp, hcfg);

    // Cancel immediately
    job.cancel();

    // Wait for the job to finish (cancelled)
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (job.state() == CarveJobState::Computing) {
        if (std::chrono::steady_clock::now() > deadline) {
            FAIL() << "CarveJob cancel timed out";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Should be back to Idle (cancelled)
    EXPECT_EQ(job.state(), CarveJobState::Idle);
}
