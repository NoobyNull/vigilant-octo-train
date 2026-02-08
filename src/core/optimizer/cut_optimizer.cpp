#include "cut_optimizer.h"

#include "bin_packer.h"
#include "guillotine.h"

namespace dw {
namespace optimizer {

std::unique_ptr<CutOptimizer> CutOptimizer::create(Algorithm algorithm) {
    switch (algorithm) {
    case Algorithm::FirstFitDecreasing:
        return std::make_unique<BinPacker>();
    case Algorithm::Guillotine:
        return std::make_unique<GuillotineOptimizer>();
    }
    return std::make_unique<BinPacker>(); // Default
}

} // namespace optimizer
} // namespace dw
