#include "rate_category_repository.h"

#include <unordered_map>

#include "../utils/log.h"

namespace dw {

RateCategoryRepository::RateCategoryRepository(Database& db) : m_db(db) {}

std::optional<i64> RateCategoryRepository::insert(const RateCategory& cat) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO rate_categories (name, rate_per_cu_unit, project_id, notes)
        VALUES (?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    bool bindOk = stmt.bindText(1, cat.name) && stmt.bindDouble(2, cat.ratePerCuUnit) &&
                  stmt.bindInt(3, cat.projectId) && stmt.bindText(4, cat.notes);

    if (!bindOk) {
        log::error("RateCategoryRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf(
            "RateCategoryRepo", "Failed to insert rate category: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<RateCategory> RateCategoryRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM rate_categories WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToRateCategory(stmt);
    }

    return std::nullopt;
}

std::vector<RateCategory> RateCategoryRepository::findGlobal() {
    std::vector<RateCategory> results;

    auto stmt = m_db.prepare("SELECT * FROM rate_categories WHERE project_id = 0 ORDER BY name");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToRateCategory(stmt));
    }

    return results;
}

std::vector<RateCategory> RateCategoryRepository::findByProject(i64 projectId) {
    std::vector<RateCategory> results;

    auto stmt =
        m_db.prepare("SELECT * FROM rate_categories WHERE project_id = ? ORDER BY name");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, projectId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToRateCategory(stmt));
    }

    return results;
}

std::vector<RateCategory> RateCategoryRepository::getEffectiveRates(i64 projectId) {
    // Get global rates as the base
    auto globals = findGlobal();

    // If projectId is 0 or no project rates exist, just return globals
    if (projectId == 0) {
        return globals;
    }

    auto projectRates = findByProject(projectId);
    if (projectRates.empty()) {
        return globals;
    }

    // Build result: project overrides global by name, plus project-only entries
    std::unordered_map<std::string, RateCategory> merged;

    // Start with globals
    for (auto& g : globals) {
        merged[g.name] = std::move(g);
    }

    // Override with project-specific (or add new project-only entries)
    for (auto& p : projectRates) {
        merged[p.name] = std::move(p);
    }

    // Convert map to vector
    std::vector<RateCategory> result;
    result.reserve(merged.size());
    for (auto& [name, cat] : merged) {
        result.push_back(std::move(cat));
    }

    return result;
}

bool RateCategoryRepository::update(const RateCategory& cat) {
    auto stmt = m_db.prepare(R"(
        UPDATE rate_categories SET
            name = ?,
            rate_per_cu_unit = ?,
            project_id = ?,
            notes = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    bool bindOk = stmt.bindText(1, cat.name) && stmt.bindDouble(2, cat.ratePerCuUnit) &&
                  stmt.bindInt(3, cat.projectId) && stmt.bindText(4, cat.notes) &&
                  stmt.bindInt(5, cat.id);

    if (!bindOk) {
        return false;
    }

    return stmt.execute();
}

bool RateCategoryRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM rate_categories WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }

    return stmt.execute();
}

bool RateCategoryRepository::removeByProject(i64 projectId) {
    auto stmt = m_db.prepare("DELETE FROM rate_categories WHERE project_id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, projectId)) {
        return false;
    }

    return stmt.execute();
}

i64 RateCategoryRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM rate_categories");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

RateCategory RateCategoryRepository::rowToRateCategory(Statement& stmt) {
    RateCategory cat;
    cat.id = stmt.getInt(0);
    cat.name = stmt.getText(1);
    cat.ratePerCuUnit = stmt.getDouble(2);
    cat.projectId = stmt.getInt(3);
    cat.notes = stmt.getText(4);
    return cat;
}

} // namespace dw
