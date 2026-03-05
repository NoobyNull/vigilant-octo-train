#include "rate_category_repository.h"

#include "../utils/log.h"

namespace dw {

RateCategoryRepository::RateCategoryRepository(Database& db) : m_db(db) {}

std::optional<i64> RateCategoryRepository::insert(const RateCategory& /*cat*/) {
    return std::nullopt;
}

std::optional<RateCategory> RateCategoryRepository::findById(i64 /*id*/) {
    return std::nullopt;
}

std::vector<RateCategory> RateCategoryRepository::findGlobal() {
    return {};
}

std::vector<RateCategory> RateCategoryRepository::findByProject(i64 /*projectId*/) {
    return {};
}

std::vector<RateCategory> RateCategoryRepository::getEffectiveRates(i64 /*projectId*/) {
    return {};
}

bool RateCategoryRepository::update(const RateCategory& /*cat*/) {
    return false;
}

bool RateCategoryRepository::remove(i64 /*id*/) {
    return false;
}

bool RateCategoryRepository::removeByProject(i64 /*projectId*/) {
    return false;
}

i64 RateCategoryRepository::count() {
    return 0;
}

RateCategory RateCategoryRepository::rowToRateCategory(Statement& /*stmt*/) {
    return {};
}

} // namespace dw
