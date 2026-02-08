#include "model_repository.h"

#include <sstream>

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

ModelRepository::ModelRepository(Database& db) : m_db(db) {}

std::optional<i64> ModelRepository::insert(const ModelRecord& model) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO models (
            hash, name, file_path, file_format, file_size,
            vertex_count, triangle_count,
            bounds_min_x, bounds_min_y, bounds_min_z,
            bounds_max_x, bounds_max_y, bounds_max_z,
            thumbnail_path, tags
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, model.hash) || !stmt.bindText(2, model.name) ||
        !stmt.bindText(3, model.filePath.string()) || !stmt.bindText(4, model.fileFormat) ||
        !stmt.bindInt(5, static_cast<i64>(model.fileSize)) ||
        !stmt.bindInt(6, static_cast<i64>(model.vertexCount)) ||
        !stmt.bindInt(7, static_cast<i64>(model.triangleCount)) ||
        !stmt.bindDouble(8, static_cast<f64>(model.boundsMin.x)) ||
        !stmt.bindDouble(9, static_cast<f64>(model.boundsMin.y)) ||
        !stmt.bindDouble(10, static_cast<f64>(model.boundsMin.z)) ||
        !stmt.bindDouble(11, static_cast<f64>(model.boundsMax.x)) ||
        !stmt.bindDouble(12, static_cast<f64>(model.boundsMax.y)) ||
        !stmt.bindDouble(13, static_cast<f64>(model.boundsMax.z)) ||
        !stmt.bindText(14, model.thumbnailPath.string()) ||
        !stmt.bindText(15, tagsToJson(model.tags))) {
        log::error("ModelRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("ModelRepo", "Failed to insert model: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<ModelRecord> ModelRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM models WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToModel(stmt);
    }

    return std::nullopt;
}

std::optional<ModelRecord> ModelRepository::findByHash(std::string_view hash) {
    auto stmt = m_db.prepare("SELECT * FROM models WHERE hash = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, std::string(hash))) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToModel(stmt);
    }

    return std::nullopt;
}

std::vector<ModelRecord> ModelRepository::findAll() {
    std::vector<ModelRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM models ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToModel(stmt));
    }

    return results;
}

std::vector<ModelRecord> ModelRepository::findByName(std::string_view searchTerm) {
    std::vector<ModelRecord> results;

    auto stmt = m_db.prepare(
        "SELECT * FROM models WHERE name LIKE ? ESCAPE '\\' ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, "%" + str::escapeLike(searchTerm) + "%")) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToModel(stmt));
    }

    return results;
}

std::vector<ModelRecord> ModelRepository::findByFormat(std::string_view format) {
    std::vector<ModelRecord> results;

    auto stmt =
        m_db.prepare("SELECT * FROM models WHERE file_format = ? ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, std::string(format))) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToModel(stmt));
    }

    return results;
}

std::vector<ModelRecord> ModelRepository::findByTag(std::string_view tag) {
    std::vector<ModelRecord> results;

    // Simple JSON search using LIKE (for now)
    auto stmt = m_db.prepare(
        "SELECT * FROM models WHERE tags LIKE ? ESCAPE '\\' ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, "%\"" + str::escapeLike(tag) + "\"%")) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToModel(stmt));
    }

    return results;
}

bool ModelRepository::update(const ModelRecord& model) {
    auto stmt = m_db.prepare(R"(
        UPDATE models SET
            name = ?,
            file_path = ?,
            file_format = ?,
            file_size = ?,
            vertex_count = ?,
            triangle_count = ?,
            bounds_min_x = ?,
            bounds_min_y = ?,
            bounds_min_z = ?,
            bounds_max_x = ?,
            bounds_max_y = ?,
            bounds_max_z = ?,
            thumbnail_path = ?,
            tags = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, model.name) || !stmt.bindText(2, model.filePath.string()) ||
        !stmt.bindText(3, model.fileFormat) || !stmt.bindInt(4, static_cast<i64>(model.fileSize)) ||
        !stmt.bindInt(5, static_cast<i64>(model.vertexCount)) ||
        !stmt.bindInt(6, static_cast<i64>(model.triangleCount)) ||
        !stmt.bindDouble(7, static_cast<f64>(model.boundsMin.x)) ||
        !stmt.bindDouble(8, static_cast<f64>(model.boundsMin.y)) ||
        !stmt.bindDouble(9, static_cast<f64>(model.boundsMin.z)) ||
        !stmt.bindDouble(10, static_cast<f64>(model.boundsMax.x)) ||
        !stmt.bindDouble(11, static_cast<f64>(model.boundsMax.y)) ||
        !stmt.bindDouble(12, static_cast<f64>(model.boundsMax.z)) ||
        !stmt.bindText(13, model.thumbnailPath.string()) ||
        !stmt.bindText(14, tagsToJson(model.tags)) || !stmt.bindInt(15, model.id)) {
        return false;
    }

    return stmt.execute();
}

bool ModelRepository::updateThumbnail(i64 id, const Path& thumbnailPath) {
    auto stmt = m_db.prepare("UPDATE models SET thumbnail_path = ? WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, thumbnailPath.string()) || !stmt.bindInt(2, id)) {
        return false;
    }

    return stmt.execute();
}

bool ModelRepository::updateTags(i64 id, const std::vector<std::string>& tags) {
    auto stmt = m_db.prepare("UPDATE models SET tags = ? WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, tagsToJson(tags)) || !stmt.bindInt(2, id)) {
        return false;
    }

    return stmt.execute();
}

bool ModelRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM models WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }
    return stmt.execute();
}

bool ModelRepository::removeByHash(std::string_view hash) {
    auto stmt = m_db.prepare("DELETE FROM models WHERE hash = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, std::string(hash))) {
        return false;
    }
    return stmt.execute();
}

bool ModelRepository::exists(std::string_view hash) {
    auto stmt = m_db.prepare("SELECT 1 FROM models WHERE hash = ? LIMIT 1");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, std::string(hash))) {
        return false;
    }
    return stmt.step();
}

i64 ModelRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM models");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

ModelRecord ModelRepository::rowToModel(Statement& stmt) {
    ModelRecord model;
    model.id = stmt.getInt(0);
    model.hash = stmt.getText(1);
    model.name = stmt.getText(2);
    model.filePath = stmt.getText(3);
    model.fileFormat = stmt.getText(4);
    model.fileSize = static_cast<u64>(stmt.getInt(5));
    model.vertexCount = static_cast<u32>(stmt.getInt(6));
    model.triangleCount = static_cast<u32>(stmt.getInt(7));
    model.boundsMin.x = static_cast<f32>(stmt.getDouble(8));
    model.boundsMin.y = static_cast<f32>(stmt.getDouble(9));
    model.boundsMin.z = static_cast<f32>(stmt.getDouble(10));
    model.boundsMax.x = static_cast<f32>(stmt.getDouble(11));
    model.boundsMax.y = static_cast<f32>(stmt.getDouble(12));
    model.boundsMax.z = static_cast<f32>(stmt.getDouble(13));
    model.thumbnailPath = stmt.getText(14);
    model.importedAt = stmt.getText(15);
    model.tags = jsonToTags(stmt.getText(16));
    return model;
}

std::string ModelRepository::tagsToJson(const std::vector<std::string>& tags) {
    if (tags.empty()) {
        return "[]";
    }

    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << "\"" << str::escapeJsonString(tags[i]) << "\"";
    }
    ss << "]";
    return ss.str();
}

std::vector<std::string> ModelRepository::jsonToTags(const std::string& json) {
    std::vector<std::string> tags;

    if (json.size() < 2 || json[0] != '[') {
        return tags;
    }

    size_t pos = 1;
    while (pos < json.size()) {
        // Find opening quote
        size_t start = json.find('"', pos);
        if (start == std::string::npos) {
            break;
        }

        // Find closing quote, skipping escaped quotes
        size_t end = start + 1;
        while (end < json.size()) {
            if (json[end] == '"' && json[end - 1] != '\\') {
                break;
            }
            ++end;
        }
        if (end >= json.size()) {
            break;
        }

        // Unescape the string
        std::string tag;
        for (size_t i = start + 1; i < end; ++i) {
            if (json[i] == '\\' && i + 1 < end) {
                char next = json[i + 1];
                if (next == '"' || next == '\\') {
                    tag.push_back(next);
                    ++i;
                    continue;
                }
            }
            tag.push_back(json[i]);
        }
        tags.push_back(std::move(tag));
        pos = end + 1;
    }

    return tags;
}

} // namespace dw
