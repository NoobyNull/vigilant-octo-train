#include "costing_engine.h"

#include <algorithm>
#include <chrono>

#include "../database/rate_category_repository.h"

namespace dw {

void CostingEngine::setEntries(std::vector<CostingEntry> entries) {
    m_entries = std::move(entries);
}

const std::vector<CostingEntry>& CostingEngine::entries() const {
    return m_entries;
}

void CostingEngine::addEntry(CostingEntry entry) {
    if (entry.id.empty()) {
        entry.id = generateId();
    }
    m_entries.push_back(std::move(entry));
}

void CostingEngine::removeEntry(const std::string& entryId) {
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                        [&entryId](const CostingEntry& e) { return e.id == entryId; }),
        m_entries.end());
}

CostingEntry* CostingEngine::findEntry(const std::string& entryId) {
    for (auto& e : m_entries) {
        if (e.id == entryId) {
            return &e;
        }
    }
    return nullptr;
}

CostingEntry CostingEngine::createMaterialEntry(
    const std::string& name,
    i64 dbId,
    const std::string& dimensions,
    f64 priceAtCapture,
    f64 quantity,
    const std::string& unit) {
    CostingEntry e;
    e.name = name;
    e.category = CostCat::Material;
    e.quantity = quantity;
    e.unitRate = priceAtCapture;
    e.estimatedTotal = quantity * priceAtCapture;
    e.unit = unit;
    e.snapshot.dbId = dbId;
    e.snapshot.name = name;
    e.snapshot.dimensions = dimensions;
    e.snapshot.priceAtCapture = priceAtCapture;
    return e;
}

CostingEntry CostingEngine::createLaborEntry(
    const std::string& name,
    f64 hourlyRate,
    f64 estimatedHours) {
    CostingEntry e;
    e.name = name;
    e.category = CostCat::Labor;
    e.quantity = estimatedHours;
    e.unitRate = hourlyRate;
    e.estimatedTotal = hourlyRate * estimatedHours;
    e.unit = "hour";
    return e;
}

CostingEntry CostingEngine::createOverheadEntry(
    const std::string& name,
    f64 amount,
    const std::string& notes) {
    CostingEntry e;
    e.name = name;
    e.category = CostCat::Overhead;
    e.quantity = 1.0;
    e.unitRate = amount;
    e.estimatedTotal = amount;
    e.unit = "each";
    e.notes = notes;
    return e;
}

void CostingEngine::applyRateCategories(const std::vector<RateCategory>& rates,
                                         f64 projectVolumeCuUnits) {
    // Remove existing auto-generated rate entries
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                        [](const CostingEntry& e) {
                            return e.notes.find("[auto:rate]") == 0;
                        }),
        m_entries.end());

    // Generate new entries from rate categories
    for (const auto& rate : rates) {
        CostingEntry e;
        e.category = CostCat::Consumable;
        e.name = rate.name;
        e.quantity = projectVolumeCuUnits;
        e.unitRate = rate.ratePerCuUnit;
        e.estimatedTotal = rate.computeCost(projectVolumeCuUnits);
        e.unit = "cu unit";
        e.notes = "[auto:rate] " + rate.notes;
        addEntry(std::move(e));
    }
}

void CostingEngine::recalculate() {
    for (auto& e : m_entries) {
        e.estimatedTotal = e.quantity * e.unitRate;
    }
}

f64 CostingEngine::totalEstimated() const {
    f64 total = 0.0;
    for (const auto& e : m_entries) {
        total += e.estimatedTotal;
    }
    return total;
}

f64 CostingEngine::totalActual() const {
    f64 total = 0.0;
    for (const auto& e : m_entries) {
        total += e.actualTotal;
    }
    return total;
}

f64 CostingEngine::variance() const {
    return totalActual() - totalEstimated();
}

std::vector<CategoryTotal> CostingEngine::categoryTotals() const {
    // Maintain canonical order
    const char* categoryOrder[] = {
        CostCat::Material,
        CostCat::Tooling,
        CostCat::Consumable,
        CostCat::Labor,
        CostCat::Overhead,
    };

    std::vector<CategoryTotal> result;
    for (const auto* cat : categoryOrder) {
        CategoryTotal ct;
        ct.category = cat;
        for (const auto& e : m_entries) {
            if (e.category == cat) {
                ct.estimatedTotal += e.estimatedTotal;
                ct.actualTotal += e.actualTotal;
                ct.entryCount++;
            }
        }
        if (ct.entryCount > 0) {
            result.push_back(std::move(ct));
        }
    }
    return result;
}

std::string CostingEngine::generateId() const {
    static int counter = 0;
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    return "ce-" + std::to_string(ms) + "-" + std::to_string(++counter);
}

} // namespace dw
