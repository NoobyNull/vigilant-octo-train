#include "material_repository.h"

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

MaterialRepository::MaterialRepository(Database& db) : m_db(db) {}

std::optional<i64> MaterialRepository::insert(const MaterialRecord& material) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO materials (
            name, category, archive_path,
            janka_hardness, feed_rate, spindle_speed,
            depth_of_cut, cost_per_board_foot, grain_direction_deg,
            thumbnail_path, is_bundled, is_hidden
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, material.name) ||
        !stmt.bindText(2, materialCategoryToString(material.category)) ||
        !stmt.bindText(3, material.archivePath.string()) ||
        !stmt.bindDouble(4, static_cast<f64>(material.jankaHardness)) ||
        !stmt.bindDouble(5, static_cast<f64>(material.feedRate)) ||
        !stmt.bindDouble(6, static_cast<f64>(material.spindleSpeed)) ||
        !stmt.bindDouble(7, static_cast<f64>(material.depthOfCut)) ||
        !stmt.bindDouble(8, static_cast<f64>(material.costPerBoardFoot)) ||
        !stmt.bindDouble(9, static_cast<f64>(material.grainDirectionDeg)) ||
        !stmt.bindText(10, material.thumbnailPath.string()) ||
        !stmt.bindInt(11, material.isBundled ? 1 : 0) ||
        !stmt.bindInt(12, material.isHidden ? 1 : 0)) {
        log::error("MaterialRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("MaterialRepo", "Failed to insert material: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<MaterialRecord> MaterialRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM materials WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToMaterial(stmt);
    }

    return std::nullopt;
}

std::vector<MaterialRecord> MaterialRepository::findAll() {
    std::vector<MaterialRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM materials WHERE is_hidden = 0 ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToMaterial(stmt));
    }

    return results;
}

std::vector<MaterialRecord> MaterialRepository::findByCategory(MaterialCategory category) {
    std::vector<MaterialRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM materials WHERE category = ? AND is_hidden = 0 ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, materialCategoryToString(category))) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToMaterial(stmt));
    }

    return results;
}

std::vector<MaterialRecord> MaterialRepository::findByName(std::string_view searchTerm) {
    std::vector<MaterialRecord> results;

    auto stmt =
        m_db.prepare("SELECT * FROM materials WHERE name LIKE ? ESCAPE '\\' ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, "%" + str::escapeLike(searchTerm) + "%")) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToMaterial(stmt));
    }

    return results;
}

std::optional<MaterialRecord> MaterialRepository::findByExactName(const std::string& name) {
    auto stmt = m_db.prepare("SELECT * FROM materials WHERE name = ? LIMIT 1");
    if (!stmt.isValid()) {
        return std::nullopt;
    }
    if (!stmt.bindText(1, name)) {
        return std::nullopt;
    }
    if (stmt.step()) {
        return rowToMaterial(stmt);
    }
    return std::nullopt;
}

bool MaterialRepository::update(const MaterialRecord& material) {
    auto stmt = m_db.prepare(R"(
        UPDATE materials SET
            name = ?,
            category = ?,
            archive_path = ?,
            janka_hardness = ?,
            feed_rate = ?,
            spindle_speed = ?,
            depth_of_cut = ?,
            cost_per_board_foot = ?,
            grain_direction_deg = ?,
            thumbnail_path = ?,
            is_bundled = ?,
            is_hidden = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, material.name) ||
        !stmt.bindText(2, materialCategoryToString(material.category)) ||
        !stmt.bindText(3, material.archivePath.string()) ||
        !stmt.bindDouble(4, static_cast<f64>(material.jankaHardness)) ||
        !stmt.bindDouble(5, static_cast<f64>(material.feedRate)) ||
        !stmt.bindDouble(6, static_cast<f64>(material.spindleSpeed)) ||
        !stmt.bindDouble(7, static_cast<f64>(material.depthOfCut)) ||
        !stmt.bindDouble(8, static_cast<f64>(material.costPerBoardFoot)) ||
        !stmt.bindDouble(9, static_cast<f64>(material.grainDirectionDeg)) ||
        !stmt.bindText(10, material.thumbnailPath.string()) ||
        !stmt.bindInt(11, material.isBundled ? 1 : 0) ||
        !stmt.bindInt(12, material.isHidden ? 1 : 0) ||
        !stmt.bindInt(13, material.id)) {
        log::error("MaterialRepo", "Failed to bind update parameters");
        return false;
    }

    return stmt.execute();
}

bool MaterialRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM materials WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }

    if (!stmt.execute()) {
        return false;
    }

    return m_db.changesCount() > 0;
}

i64 MaterialRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM materials");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

MaterialRecord MaterialRepository::rowToMaterial(Statement& stmt) {
    MaterialRecord material;
    material.id = stmt.getInt(0);
    material.name = stmt.getText(1);
    material.category = stringToMaterialCategory(stmt.getText(2));
    material.archivePath = Path(stmt.getText(3));
    material.jankaHardness = static_cast<f32>(stmt.getDouble(4));
    material.feedRate = static_cast<f32>(stmt.getDouble(5));
    material.spindleSpeed = static_cast<f32>(stmt.getDouble(6));
    material.depthOfCut = static_cast<f32>(stmt.getDouble(7));
    material.costPerBoardFoot = static_cast<f32>(stmt.getDouble(8));
    material.grainDirectionDeg = static_cast<f32>(stmt.getDouble(9));
    material.thumbnailPath = Path(stmt.getText(10));
    material.importedAt = stmt.getText(11);
    material.isBundled = stmt.getInt(12) != 0;
    material.isHidden = stmt.getInt(13) != 0;
    return material;
}

} // namespace dw
