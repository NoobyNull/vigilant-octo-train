#include "project_costing_io.h"

namespace dw {

bool ProjectCostingIO::saveEstimates(const Path& /*costingDir*/,
                                     const std::vector<CostingEstimate>& /*estimates*/) {
    return false;
}

std::vector<CostingEstimate> ProjectCostingIO::loadEstimates(const Path& /*costingDir*/) {
    return {};
}

bool ProjectCostingIO::saveOrders(const Path& /*costingDir*/,
                                  const std::vector<CostingOrder>& /*orders*/) {
    return false;
}

std::vector<CostingOrder> ProjectCostingIO::loadOrders(const Path& /*costingDir*/) {
    return {};
}

CostingOrder ProjectCostingIO::convertToOrder(const CostingEstimate& /*estimate*/) {
    return {};
}

} // namespace dw
