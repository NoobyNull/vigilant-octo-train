#include "stock_size_repository.h"

#include "../utils/log.h"

namespace dw {

StockSizeRepository::StockSizeRepository(Database& db) : m_db(db) {}

std::optional<i64> StockSizeRepository::insert(const StockSize& /*size*/) {
    return std::nullopt;
}

std::optional<StockSize> StockSizeRepository::findById(i64 /*id*/) {
    return std::nullopt;
}

std::vector<StockSize> StockSizeRepository::findByMaterial(i64 /*materialId*/) {
    return {};
}

std::vector<StockSize> StockSizeRepository::findAll() {
    return {};
}

bool StockSizeRepository::update(const StockSize& /*size*/) {
    return false;
}

bool StockSizeRepository::remove(i64 /*id*/) {
    return false;
}

bool StockSizeRepository::removeByMaterial(i64 /*materialId*/) {
    return false;
}

i64 StockSizeRepository::count() {
    return 0;
}

i64 StockSizeRepository::countByMaterial(i64 /*materialId*/) {
    return 0;
}

StockSize StockSizeRepository::rowToStockSize(Statement& /*stmt*/) {
    return {};
}

} // namespace dw
