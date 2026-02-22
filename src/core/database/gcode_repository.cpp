#include "gcode_repository.h"

#include <sstream>

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

GCodeRepository::GCodeRepository(Database& db) : m_db(db) {}

// ===== G-code file CRUD =====

std::optional<i64> GCodeRepository::insert(const GCodeRecord& record) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO gcode_files (
            hash, name, file_path, file_size,
            bounds_min_x, bounds_min_y, bounds_min_z,
            bounds_max_x, bounds_max_y, bounds_max_z,
            total_distance, estimated_time,
            feed_rates, tool_numbers, thumbnail_path
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, record.hash) || !stmt.bindText(2, record.name) ||
        !stmt.bindText(3, record.filePath.string()) ||
        !stmt.bindInt(4, static_cast<i64>(record.fileSize)) ||
        !stmt.bindDouble(5, static_cast<f64>(record.boundsMin.x)) ||
        !stmt.bindDouble(6, static_cast<f64>(record.boundsMin.y)) ||
        !stmt.bindDouble(7, static_cast<f64>(record.boundsMin.z)) ||
        !stmt.bindDouble(8, static_cast<f64>(record.boundsMax.x)) ||
        !stmt.bindDouble(9, static_cast<f64>(record.boundsMax.y)) ||
        !stmt.bindDouble(10, static_cast<f64>(record.boundsMax.z)) ||
        !stmt.bindDouble(11, static_cast<f64>(record.totalDistance)) ||
        !stmt.bindDouble(12, static_cast<f64>(record.estimatedTime)) ||
        !stmt.bindText(13, feedRatesToJson(record.feedRates)) ||
        !stmt.bindText(14, toolNumbersToJson(record.toolNumbers)) ||
        !stmt.bindText(15, record.thumbnailPath.string())) {
        log::error("GCodeRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("GCodeRepo", "Failed to insert gcode: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<GCodeRecord> GCodeRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM gcode_files WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToGCode(stmt);
    }

    return std::nullopt;
}

std::optional<GCodeRecord> GCodeRepository::findByHash(std::string_view hash) {
    auto stmt = m_db.prepare("SELECT * FROM gcode_files WHERE hash = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, std::string(hash))) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToGCode(stmt);
    }

    return std::nullopt;
}

std::vector<GCodeRecord> GCodeRepository::findAll() {
    std::vector<GCodeRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM gcode_files ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToGCode(stmt));
    }

    return results;
}

std::vector<GCodeRecord> GCodeRepository::findByName(std::string_view searchTerm) {
    std::vector<GCodeRecord> results;

    auto stmt = m_db.prepare(
        "SELECT * FROM gcode_files WHERE name LIKE ? ESCAPE '\\' ORDER BY imported_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, "%" + str::escapeLike(searchTerm) + "%")) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToGCode(stmt));
    }

    return results;
}

bool GCodeRepository::update(const GCodeRecord& record) {
    auto stmt = m_db.prepare(R"(
        UPDATE gcode_files SET
            name = ?,
            file_path = ?,
            file_size = ?,
            bounds_min_x = ?,
            bounds_min_y = ?,
            bounds_min_z = ?,
            bounds_max_x = ?,
            bounds_max_y = ?,
            bounds_max_z = ?,
            total_distance = ?,
            estimated_time = ?,
            feed_rates = ?,
            tool_numbers = ?,
            thumbnail_path = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, record.name) || !stmt.bindText(2, record.filePath.string()) ||
        !stmt.bindInt(3, static_cast<i64>(record.fileSize)) ||
        !stmt.bindDouble(4, static_cast<f64>(record.boundsMin.x)) ||
        !stmt.bindDouble(5, static_cast<f64>(record.boundsMin.y)) ||
        !stmt.bindDouble(6, static_cast<f64>(record.boundsMin.z)) ||
        !stmt.bindDouble(7, static_cast<f64>(record.boundsMax.x)) ||
        !stmt.bindDouble(8, static_cast<f64>(record.boundsMax.y)) ||
        !stmt.bindDouble(9, static_cast<f64>(record.boundsMax.z)) ||
        !stmt.bindDouble(10, static_cast<f64>(record.totalDistance)) ||
        !stmt.bindDouble(11, static_cast<f64>(record.estimatedTime)) ||
        !stmt.bindText(12, feedRatesToJson(record.feedRates)) ||
        !stmt.bindText(13, toolNumbersToJson(record.toolNumbers)) ||
        !stmt.bindText(14, record.thumbnailPath.string()) || !stmt.bindInt(15, record.id)) {
        return false;
    }

    return stmt.execute();
}

bool GCodeRepository::updateThumbnail(i64 id, const Path& thumbnailPath) {
    auto stmt = m_db.prepare("UPDATE gcode_files SET thumbnail_path = ? WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, thumbnailPath.string()) || !stmt.bindInt(2, id)) {
        return false;
    }

    return stmt.execute();
}

bool GCodeRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM gcode_files WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, id)) {
        return false;
    }
    return stmt.execute();
}

bool GCodeRepository::exists(std::string_view hash) {
    auto stmt = m_db.prepare("SELECT 1 FROM gcode_files WHERE hash = ? LIMIT 1");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, std::string(hash))) {
        return false;
    }
    return stmt.step();
}

i64 GCodeRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM gcode_files");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

// ===== Hierarchy operations =====

std::optional<i64> GCodeRepository::createGroup(i64 modelId,
                                                const std::string& name,
                                                int sortOrder) {
    auto stmt =
        m_db.prepare("INSERT INTO operation_groups (model_id, name, sort_order) VALUES (?, ?, ?)");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, modelId) || !stmt.bindText(2, name) || !stmt.bindInt(3, sortOrder)) {
        log::error("GCodeRepo", "Failed to bind createGroup parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("GCodeRepo", "Failed to create group: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::vector<OperationGroup> GCodeRepository::getGroups(i64 modelId) {
    std::vector<OperationGroup> results;

    auto stmt =
        m_db.prepare("SELECT * FROM operation_groups WHERE model_id = ? ORDER BY sort_order");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, modelId)) {
        return results;
    }

    while (stmt.step()) {
        OperationGroup group;
        group.id = stmt.getInt(0);
        group.modelId = stmt.getInt(1);
        group.name = stmt.getText(2);
        group.sortOrder = static_cast<int>(stmt.getInt(3));
        results.push_back(group);
    }

    return results;
}

bool GCodeRepository::addToGroup(i64 groupId, i64 gcodeId, int sortOrder) {
    auto stmt = m_db.prepare(
        "INSERT INTO gcode_group_members (group_id, gcode_id, sort_order) VALUES (?, ?, ?)");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, groupId) || !stmt.bindInt(2, gcodeId) || !stmt.bindInt(3, sortOrder)) {
        return false;
    }

    return stmt.execute();
}

bool GCodeRepository::removeFromGroup(i64 groupId, i64 gcodeId) {
    auto stmt = m_db.prepare("DELETE FROM gcode_group_members WHERE group_id = ? AND gcode_id = ?");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, groupId) || !stmt.bindInt(2, gcodeId)) {
        return false;
    }

    return stmt.execute();
}

std::vector<GCodeRecord> GCodeRepository::getGroupMembers(i64 groupId) {
    std::vector<GCodeRecord> results;

    auto stmt = m_db.prepare(R"(
        SELECT g.* FROM gcode_files g
        INNER JOIN gcode_group_members m ON g.id = m.gcode_id
        WHERE m.group_id = ?
        ORDER BY m.sort_order
    )");

    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, groupId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToGCode(stmt));
    }

    return results;
}

bool GCodeRepository::deleteGroup(i64 groupId) {
    // Cascade delete will handle gcode_group_members
    auto stmt = m_db.prepare("DELETE FROM operation_groups WHERE id = ?");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, groupId)) {
        return false;
    }

    return stmt.execute();
}

// ===== Template operations =====

std::vector<GCodeTemplate> GCodeRepository::getTemplates() {
    std::vector<GCodeTemplate> results;

    auto stmt = m_db.prepare("SELECT * FROM gcode_templates ORDER BY name");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        GCodeTemplate tmpl;
        tmpl.id = stmt.getInt(0);
        tmpl.name = stmt.getText(1);
        tmpl.groups = jsonToGroups(stmt.getText(2));
        results.push_back(tmpl);
    }

    return results;
}

bool GCodeRepository::applyTemplate(i64 modelId, const std::string& templateName) {
    // Find the template
    auto stmt = m_db.prepare("SELECT groups FROM gcode_templates WHERE name = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, templateName)) {
        return false;
    }

    if (!stmt.step()) {
        log::errorf("GCodeRepo", "Template not found: %s", templateName.c_str());
        return false;
    }

    auto groupNames = jsonToGroups(stmt.getText(0));

    // Create groups from template
    for (size_t i = 0; i < groupNames.size(); ++i) {
        auto groupId = createGroup(modelId, groupNames[i], static_cast<int>(i));
        if (!groupId) {
            log::errorf("GCodeRepo", "Failed to create group: %s", groupNames[i].c_str());
            return false;
        }
    }

    return true;
}

// ===== Private helpers =====

GCodeRecord GCodeRepository::rowToGCode(Statement& stmt) {
    GCodeRecord record;
    record.id = stmt.getInt(0);
    record.hash = stmt.getText(1);
    record.name = stmt.getText(2);
    record.filePath = stmt.getText(3);
    record.fileSize = static_cast<u64>(stmt.getInt(4));
    record.boundsMin.x = static_cast<f32>(stmt.getDouble(5));
    record.boundsMin.y = static_cast<f32>(stmt.getDouble(6));
    record.boundsMin.z = static_cast<f32>(stmt.getDouble(7));
    record.boundsMax.x = static_cast<f32>(stmt.getDouble(8));
    record.boundsMax.y = static_cast<f32>(stmt.getDouble(9));
    record.boundsMax.z = static_cast<f32>(stmt.getDouble(10));
    record.totalDistance = static_cast<f32>(stmt.getDouble(11));
    record.estimatedTime = static_cast<f32>(stmt.getDouble(12));
    record.feedRates = jsonToFeedRates(stmt.getText(13));
    record.toolNumbers = jsonToToolNumbers(stmt.getText(14));
    record.importedAt = stmt.getText(15);
    record.thumbnailPath = stmt.getText(16);
    return record;
}

std::string GCodeRepository::feedRatesToJson(const std::vector<f32>& feedRates) {
    if (feedRates.empty()) {
        return "[]";
    }

    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < feedRates.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << feedRates[i];
    }
    ss << "]";
    return ss.str();
}

std::vector<f32> GCodeRepository::jsonToFeedRates(const std::string& json) {
    std::vector<f32> feedRates;

    if (json.size() < 2 || json[0] != '[') {
        return feedRates;
    }

    size_t pos = 1;
    while (pos < json.size()) {
        // Skip whitespace and commas
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ',')) {
            ++pos;
        }

        if (pos >= json.size() || json[pos] == ']') {
            break;
        }

        // Parse number
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != ']' && json[end] != ' ') {
            ++end;
        }

        if (end > pos) {
            try {
                feedRates.push_back(std::stof(json.substr(pos, end - pos)));
            } catch (...) {
                // Skip invalid numbers
            }
        }

        pos = end;
    }

    return feedRates;
}

std::string GCodeRepository::toolNumbersToJson(const std::vector<int>& toolNumbers) {
    if (toolNumbers.empty()) {
        return "[]";
    }

    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < toolNumbers.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << toolNumbers[i];
    }
    ss << "]";
    return ss.str();
}

std::vector<int> GCodeRepository::jsonToToolNumbers(const std::string& json) {
    std::vector<int> toolNumbers;

    if (json.size() < 2 || json[0] != '[') {
        return toolNumbers;
    }

    size_t pos = 1;
    while (pos < json.size()) {
        // Skip whitespace and commas
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ',')) {
            ++pos;
        }

        if (pos >= json.size() || json[pos] == ']') {
            break;
        }

        // Parse number
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != ']' && json[end] != ' ') {
            ++end;
        }

        if (end > pos) {
            try {
                toolNumbers.push_back(std::stoi(json.substr(pos, end - pos)));
            } catch (...) {
                // Skip invalid numbers
            }
        }

        pos = end;
    }

    return toolNumbers;
}

std::vector<std::string> GCodeRepository::jsonToGroups(const std::string& json) {
    std::vector<std::string> groups;

    if (json.size() < 2 || json[0] != '[') {
        return groups;
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
        std::string group;
        for (size_t i = start + 1; i < end; ++i) {
            if (json[i] == '\\' && i + 1 < end) {
                char next = json[i + 1];
                if (next == '"' || next == '\\') {
                    group.push_back(next);
                    ++i;
                    continue;
                }
            }
            group.push_back(json[i]);
        }
        groups.push_back(std::move(group));
        pos = end + 1;
    }

    return groups;
}

} // namespace dw
