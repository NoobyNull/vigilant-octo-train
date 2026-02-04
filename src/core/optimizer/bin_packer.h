#pragma once

#include "cut_optimizer.h"

namespace dw {
namespace optimizer {

// First Fit Decreasing bin packing algorithm
// Simple and fast, good for general use
class BinPacker : public CutOptimizer {
public:
    BinPacker() = default;

    CutPlan optimize(const std::vector<Part>& parts,
                     const std::vector<Sheet>& sheets) override;

private:
    struct FreeRect {
        f32 x, y, width, height;
    };

    bool tryPlace(const Part& part, int partIndex, int instanceIndex,
                  std::vector<FreeRect>& freeRects, Placement& outPlacement);

    void splitFreeRect(std::vector<FreeRect>& freeRects, usize rectIndex,
                       const Placement& placement);
};

}  // namespace optimizer
}  // namespace dw
