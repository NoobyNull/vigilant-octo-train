#include "stock_size_repository.h"

#include "../utils/log.h"

namespace dw {

StockSizeRepository::StockSizeRepository(Database& db) : m_db(db) {}

std::optional<i64> StockSizeRepository::insert(const StockSize& size) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO stock_sizes (
            material_id, name, width_mm, height_mm,
            thickness_mm, price_per_unit, unit_label
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    bool bindOk = stmt.bindInt(1, size.materialId) && stmt.bindText(2, size.name);

    // Nullable width/height: store NULL if value is 0.0
    if (size.widthMm == 0.0) {
        bindOk = bindOk && stmt.bindNull(3);
    } else {
        bindOk = bindOk && stmt.bindDouble(3, size.widthMm);
    }

    if (size.heightMm == 0.0) {
        bindOk = bindOk && stmt.bindNull(4);
    } else {
        bindOk = bindOk && stmt.bindDouble(4, size.heightMm);
    }

    bindOk = bindOk && stmt.bindDouble(5, size.thicknessMm) &&
             stmt.bindDouble(6, size.pricePerUnit) && stmt.bindText(7, size.unitLabel);

    if (!bindOk) {
        log::error("StockSizeRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("StockSizeRepo", "Failed to insert stock size: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<StockSize> StockSizeRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM stock_sizes WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToStockSize(stmt);
    }

    return std::nullopt;
}

std::vector<StockSize> StockSizeRepository::findByMaterial(i64 materialId) {
    std::vector<StockSize> results;

    auto stmt = m_db.prepare("SELECT * FROM stock_sizes WHERE material_id = ? ORDER BY name");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, materialId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToStockSize(stmt));
    }

    return results;
}

std::vector<StockSize> StockSizeRepository::findAll() {
    std::vector<StockSize> results;

    auto stmt = m_db.prepare("SELECT * FROM stock_sizes ORDER BY material_id, name");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToStockSize(stmt));
    }

    return results;
}

bool StockSizeRepository::update(const StockSize& size) {
    auto stmt = m_db.prepare(R"(
        UPDATE stock_sizes SET
            material_id = ?,
            name = ?,
            width_mm = ?,
            height_mm = ?,
            thickness_mm = ?,
            price_per_unit = ?,
            unit_label = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    bool bindOk = stmt.bindInt(1, size.materialId) && stmt.bindText(2, size.name);

    if (size.widthMm == 0.0) {
        bindOk = bindOk && stmt.bindNull(3);
    } else {
        bindOk = bindOk && stmt.bindDouble(3, size.widthMm);
    }

    if (size.heightMm == 0.0) {
        bindOk = bindOk && stmt.bindNull(4);
    } else {
        bindOk = bindOk && stmt.bindDouble(4, size.heightMm);
    }

    bindOk = bindOk && stmt.bindDouble(5, size.thicknessMm) &&
             stmt.bindDouble(6, size.pricePerUnit) && stmt.bindText(7, size.unitLabel) &&
             stmt.bindInt(8, size.id);

    if (!bindOk) {
        return false;
    }

    return stmt.execute();
}

bool StockSizeRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM stock_sizes WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }

    return stmt.execute();
}

bool StockSizeRepository::removeByMaterial(i64 materialId) {
    auto stmt = m_db.prepare("DELETE FROM stock_sizes WHERE material_id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, materialId)) {
        return false;
    }

    return stmt.execute();
}

i64 StockSizeRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM stock_sizes");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

i64 StockSizeRepository::countByMaterial(i64 materialId) {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM stock_sizes WHERE material_id = ?");
    if (!stmt.isValid()) {
        return 0;
    }

    if (!stmt.bindInt(1, materialId)) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

StockSize StockSizeRepository::rowToStockSize(Statement& stmt) {
    StockSize s;
    s.id = stmt.getInt(0);
    s.materialId = stmt.getInt(1);
    s.name = stmt.getText(2);
    s.widthMm = stmt.isNull(3) ? 0.0 : stmt.getDouble(3);
    s.heightMm = stmt.isNull(4) ? 0.0 : stmt.getDouble(4);
    s.thicknessMm = stmt.getDouble(5);
    s.pricePerUnit = stmt.getDouble(6);
    s.unitLabel = stmt.getText(7);
    return s;
}

} // namespace dw
