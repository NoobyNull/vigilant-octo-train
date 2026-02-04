#pragma once

#include "cut_optimizer.h"

namespace dw {
namespace optimizer {

// Guillotine algorithm - restricts cuts to guillotine patterns
// (straight through cuts), which is more practical for CNC cutting
class GuillotineOptimizer : public CutOptimizer {
public:
    GuillotineOptimizer() = default;

    CutPlan optimize(const std::vector<Part>& parts,
                     const std::vector<Sheet>& sheets) override;

private:
    struct Node {
        f32 x, y, width, height;
        bool used = false;
        std::unique_ptr<Node> right;
        std::unique_ptr<Node> down;
    };

    Node* insert(Node* node, f32 width, f32 height);
    void collectPlacements(Node* node, std::vector<Placement>& placements,
                           const Part* part, int partIndex, int instanceIndex);
};

}  // namespace optimizer
}  // namespace dw
