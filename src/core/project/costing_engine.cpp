#include "costing_engine.h"

namespace dw {

void CostingEngine::setEntries(std::vector<CostingEntry> entries) {
    m_entries = std::move(entries);
}

const std::vector<CostingEntry>& CostingEngine::entries() const {
    return m_entries;
}

void CostingEngine::addEntry(CostingEntry entry) {
    m_entries.push_back(std::move(entry));
}

void CostingEngine::removeEntry(const std::string& /*entryId*/) {
    // stub
}

CostingEntry* CostingEngine::findEntry(const std::string& /*entryId*/) {
    return nullptr; // stub
}

CostingEntry CostingEngine::createMaterialEntry(
    const std::string& /*name*/,
    i64 /*dbId*/,
    const std::string& /*dimensions*/,
    f64 /*priceAtCapture*/,
    f64 /*quantity*/,
    const std::string& /*unit*/) {
    return {}; // stub
}

CostingEntry CostingEngine::createLaborEntry(
    const std::string& /*name*/,
    f64 /*hourlyRate*/,
    f64 /*estimatedHours*/) {
    return {}; // stub
}

CostingEntry CostingEngine::createOverheadEntry(
    const std::string& /*name*/,
    f64 /*amount*/,
    const std::string& /*notes*/) {
    return {}; // stub
}

void CostingEngine::applyRateCategories(const std::vector<RateCategory>& /*rates*/,
                                         f64 /*projectVolumeCuUnits*/) {
    // stub
}

void CostingEngine::recalculate() {
    // stub
}

f64 CostingEngine::totalEstimated() const {
    return 0.0; // stub
}

f64 CostingEngine::totalActual() const {
    return 0.0; // stub
}

f64 CostingEngine::variance() const {
    return 0.0; // stub
}

std::vector<CategoryTotal> CostingEngine::categoryTotals() const {
    return {}; // stub
}

std::string CostingEngine::generateId() const {
    return "stub-id"; // stub
}

} // namespace dw
