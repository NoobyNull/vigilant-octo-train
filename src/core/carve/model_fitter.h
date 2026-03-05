#pragma once

#include "../types.h"

#include <string>

namespace dw {
namespace carve {

struct StockDimensions {
    f32 width = 0.0f;      // X extent in mm
    f32 height = 0.0f;     // Y extent in mm
    f32 thickness = 0.0f;  // Z extent in mm
};

struct FitParams {
    f32 scale = 1.0f;      // Uniform XY scale (locked aspect)
    f32 depthMm = 0.0f;    // Z depth from top surface (0 = auto from model)
    f32 offsetX = 0.0f;    // X offset on stock (mm)
    f32 offsetY = 0.0f;    // Y offset on stock (mm)
};

struct FitResult {
    Vec3 modelMin{0.0f};          // Transformed model bounds min
    Vec3 modelMax{0.0f};          // Transformed model bounds max
    bool fitsStock = false;       // Model fits within stock dimensions
    bool fitsMachine = false;     // Model fits within machine travel
    std::string warning;          // Human-readable warning if !fits
};

class ModelFitter {
public:
    // Set the source model bounds (from loaded STL)
    void setModelBounds(const Vec3& min, const Vec3& max);

    // Set constraints
    void setStock(const StockDimensions& stock);
    void setMachineTravel(f32 travelX, f32 travelY, f32 travelZ);

    // Compute fitted bounds
    FitResult fit(const FitParams& params) const;

    // Auto-fit: compute scale to fill stock width/height
    f32 autoScale() const;

    // Auto-depth: model's full Z range
    f32 autoDepth() const;

    // Transform a point from model space to fitted space
    Vec3 transform(const Vec3& modelPoint, const FitParams& params) const;

    // Accessors
    const Vec3& modelMin() const { return m_modelMin; }
    const Vec3& modelMax() const { return m_modelMax; }
    const StockDimensions& stock() const { return m_stock; }

private:
    Vec3 m_modelMin{0.0f};
    Vec3 m_modelMax{0.0f};
    StockDimensions m_stock;
    f32 m_travelX = 0.0f;
    f32 m_travelY = 0.0f;
    f32 m_travelZ = 0.0f;
};

} // namespace carve
} // namespace dw
