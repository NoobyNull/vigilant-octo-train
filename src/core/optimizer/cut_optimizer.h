#pragma once

#include "sheet.h"

#include <memory>

namespace dw {
namespace optimizer {

// Algorithm types
enum class Algorithm { FirstFitDecreasing, Guillotine };

// Abstract optimizer interface
class CutOptimizer {
public:
    virtual ~CutOptimizer() = default;

    // Set parameters
    void setAllowRotation(bool allow) { m_allowRotation = allow; }
    void setKerf(f32 kerf) { m_kerf = kerf; }  // Blade thickness
    void setMargin(f32 margin) { m_margin = margin; }  // Edge margin

    // Run optimization
    virtual CutPlan optimize(const std::vector<Part>& parts,
                             const std::vector<Sheet>& sheets) = 0;

    // Factory
    static std::unique_ptr<CutOptimizer> create(Algorithm algorithm);

protected:
    bool m_allowRotation = true;
    f32 m_kerf = 0.0f;       // Blade width/material loss
    f32 m_margin = 0.0f;     // Edge margin on sheets
};

}  // namespace optimizer
}  // namespace dw
