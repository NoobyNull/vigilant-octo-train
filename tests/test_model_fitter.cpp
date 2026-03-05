// Digital Workshop - ModelFitter Tests

#include <gtest/gtest.h>

#include "core/carve/model_fitter.h"

#include <cmath>

using namespace dw;
using namespace dw::carve;

class ModelFitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Model: 20x10x5 mm, positioned at origin
        fitter.setModelBounds(Vec3{0.0f, 0.0f, 0.0f},
                              Vec3{20.0f, 10.0f, 5.0f});

        // Stock: 200x200x25 mm
        StockDimensions stock;
        stock.width = 200.0f;
        stock.height = 200.0f;
        stock.thickness = 25.0f;
        fitter.setStock(stock);

        // Machine travel: 300x300x100 mm
        fitter.setMachineTravel(300.0f, 300.0f, 100.0f);
    }

    ModelFitter fitter;
};

TEST_F(ModelFitterTest, AutoScale) {
    // Model is 20x10, stock is 200x200
    // Scale to fill: min(200/20, 200/10) = min(10, 20) = 10
    const f32 scale = fitter.autoScale();
    EXPECT_NEAR(scale, 10.0f, 0.001f);
}

TEST_F(ModelFitterTest, FitsStock) {
    FitParams params;
    params.scale = 1.0f;
    params.depthMm = 0.0f; // auto from model
    params.offsetX = 0.0f;
    params.offsetY = 0.0f;

    FitResult result = fitter.fit(params);

    EXPECT_TRUE(result.fitsStock);
    EXPECT_TRUE(result.fitsMachine);
    EXPECT_TRUE(result.warning.empty());
}

TEST_F(ModelFitterTest, ExceedsStock) {
    // Scale 20x10 by 15 -> 300x150 which exceeds 200x200 stock width
    FitParams params;
    params.scale = 15.0f;

    FitResult result = fitter.fit(params);

    EXPECT_FALSE(result.fitsStock);
    EXPECT_FALSE(result.warning.empty());
    // Width 300 exceeds stock width 200
    EXPECT_NE(result.warning.find("width"), std::string::npos);
}

TEST_F(ModelFitterTest, MachineTravel) {
    // Place model at offset that exceeds machine travel
    FitParams params;
    params.scale = 1.0f;
    params.offsetX = 290.0f; // 290 + 20 = 310, exceeds travel 300

    FitResult result = fitter.fit(params);

    EXPECT_TRUE(result.fitsStock); // 20mm < 200mm stock
    EXPECT_FALSE(result.fitsMachine);
    EXPECT_FALSE(result.warning.empty());
}

TEST_F(ModelFitterTest, UniformScale) {
    // XY scale is uniform (locked aspect ratio)
    FitParams params;
    params.scale = 5.0f;

    FitResult result = fitter.fit(params);

    const f32 extX = result.modelMax.x - result.modelMin.x;
    const f32 extY = result.modelMax.y - result.modelMin.y;

    // Original is 20x10 -> scaled 100x50
    EXPECT_NEAR(extX, 100.0f, 0.001f);
    EXPECT_NEAR(extY, 50.0f, 0.001f);

    // Aspect ratio preserved: 2:1
    EXPECT_NEAR(extX / extY, 2.0f, 0.001f);
}

TEST_F(ModelFitterTest, DepthControl) {
    // Auto depth = model Z range = 5mm
    EXPECT_NEAR(fitter.autoDepth(), 5.0f, 0.001f);

    // With explicit depth override
    FitParams params;
    params.scale = 1.0f;
    params.depthMm = 10.0f; // Override: 10mm instead of 5mm

    FitResult result = fitter.fit(params);

    // Z range should reflect explicit depth
    const f32 zRange = result.modelMax.z - result.modelMin.z;
    EXPECT_NEAR(zRange, 10.0f, 0.001f);

    // Top surface at stock thickness (25mm), bottom at 15mm
    EXPECT_NEAR(result.modelMax.z, 25.0f, 0.001f);
    EXPECT_NEAR(result.modelMin.z, 15.0f, 0.001f);
}

TEST_F(ModelFitterTest, TransformPreservesRelativePosition) {
    FitParams params;
    params.scale = 2.0f;
    params.offsetX = 10.0f;
    params.offsetY = 5.0f;

    // Model corners
    Vec3 modelOrigin{0.0f, 0.0f, 0.0f};
    Vec3 modelCorner{20.0f, 10.0f, 5.0f};

    Vec3 tOrigin = fitter.transform(modelOrigin, params);
    Vec3 tCorner = fitter.transform(modelCorner, params);

    // Origin should map to offset position
    EXPECT_NEAR(tOrigin.x, 10.0f, 0.001f);
    EXPECT_NEAR(tOrigin.y, 5.0f, 0.001f);

    // Corner should map to offset + scaled extent
    EXPECT_NEAR(tCorner.x, 10.0f + 40.0f, 0.001f); // 20 * 2
    EXPECT_NEAR(tCorner.y, 5.0f + 20.0f, 0.001f);  // 10 * 2
}

TEST_F(ModelFitterTest, DepthExceedsStockThickness) {
    FitParams params;
    params.scale = 1.0f;
    params.depthMm = 30.0f; // Exceeds 25mm stock

    FitResult result = fitter.fit(params);

    EXPECT_FALSE(result.fitsStock);
    EXPECT_FALSE(result.warning.empty());
    EXPECT_NE(result.warning.find("depth"), std::string::npos);
}

TEST_F(ModelFitterTest, AutoScaleWithAsymmetricStock) {
    // Set a narrow stock
    StockDimensions narrow;
    narrow.width = 100.0f;
    narrow.height = 30.0f;
    narrow.thickness = 25.0f;
    fitter.setStock(narrow);

    // Model 20x10 -> min(100/20, 30/10) = min(5, 3) = 3
    const f32 scale = fitter.autoScale();
    EXPECT_NEAR(scale, 3.0f, 0.001f);
}
