#include "cnc_tool_repository.h"

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

CncToolRepository::CncToolRepository(Database& db) : m_db(db) {}

std::optional<i64> CncToolRepository::insert(const CncToolRecord& tool) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO cnc_tools (
            name, type, diameter, flute_count, max_rpm,
            max_doc, shank_diameter, notes
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindText(1, tool.name) ||
        !stmt.bindText(2, cncToolTypeToString(tool.type)) ||
        !stmt.bindDouble(3, tool.diameter) ||
        !stmt.bindInt(4, tool.fluteCount) ||
        !stmt.bindDouble(5, tool.maxRPM) ||
        !stmt.bindDouble(6, tool.maxDOC) ||
        !stmt.bindDouble(7, tool.shankDiameter) ||
        !stmt.bindText(8, tool.notes)) {
        log::error("CncToolRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("CncToolRepo", "Failed to insert tool: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<CncToolRecord> CncToolRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM cnc_tools WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, id)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToTool(stmt);
    }

    return std::nullopt;
}

std::vector<CncToolRecord> CncToolRepository::findAll() {
    std::vector<CncToolRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM cnc_tools ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToTool(stmt));
    }

    return results;
}

std::vector<CncToolRecord> CncToolRepository::findByType(CncToolType type) {
    std::vector<CncToolRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM cnc_tools WHERE type = ? ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, cncToolTypeToString(type))) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToTool(stmt));
    }

    return results;
}

std::vector<CncToolRecord> CncToolRepository::findByName(std::string_view searchTerm) {
    std::vector<CncToolRecord> results;

    auto stmt =
        m_db.prepare("SELECT * FROM cnc_tools WHERE name LIKE ? ESCAPE '\\' ORDER BY name ASC");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindText(1, "%" + str::escapeLike(searchTerm) + "%")) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToTool(stmt));
    }

    return results;
}

bool CncToolRepository::update(const CncToolRecord& tool) {
    auto stmt = m_db.prepare(R"(
        UPDATE cnc_tools SET
            name = ?,
            type = ?,
            diameter = ?,
            flute_count = ?,
            max_rpm = ?,
            max_doc = ?,
            shank_diameter = ?,
            notes = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindText(1, tool.name) ||
        !stmt.bindText(2, cncToolTypeToString(tool.type)) ||
        !stmt.bindDouble(3, tool.diameter) ||
        !stmt.bindInt(4, tool.fluteCount) ||
        !stmt.bindDouble(5, tool.maxRPM) ||
        !stmt.bindDouble(6, tool.maxDOC) ||
        !stmt.bindDouble(7, tool.shankDiameter) ||
        !stmt.bindText(8, tool.notes) || !stmt.bindInt(9, tool.id)) {
        log::error("CncToolRepo", "Failed to bind update parameters");
        return false;
    }

    return stmt.execute();
}

bool CncToolRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM cnc_tools WHERE id = ?");
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

i64 CncToolRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM cnc_tools");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

// --- Junction CRUD ---

std::optional<i64> CncToolRepository::insertParams(const ToolMaterialParams& params) {
    auto stmt = m_db.prepare(R"(
        INSERT OR REPLACE INTO tool_material_params (
            tool_id, material_id, feed_rate, spindle_speed, depth_of_cut, chip_load
        ) VALUES (?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, params.toolId) ||
        !stmt.bindInt(2, params.materialId) ||
        !stmt.bindDouble(3, params.feedRate) ||
        !stmt.bindDouble(4, params.spindleSpeed) ||
        !stmt.bindDouble(5, params.depthOfCut) ||
        !stmt.bindDouble(6, params.chipLoad)) {
        log::error("CncToolRepo", "Failed to bind insertParams parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("CncToolRepo", "Failed to insert params: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<ToolMaterialParams> CncToolRepository::findParams(i64 toolId, i64 materialId) {
    auto stmt = m_db.prepare(
        "SELECT * FROM tool_material_params WHERE tool_id = ? AND material_id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    if (!stmt.bindInt(1, toolId) || !stmt.bindInt(2, materialId)) {
        return std::nullopt;
    }

    if (stmt.step()) {
        return rowToParams(stmt);
    }

    return std::nullopt;
}

std::vector<ToolMaterialParams> CncToolRepository::findParamsForTool(i64 toolId) {
    std::vector<ToolMaterialParams> results;

    auto stmt = m_db.prepare("SELECT * FROM tool_material_params WHERE tool_id = ?");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, toolId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToParams(stmt));
    }

    return results;
}

std::vector<ToolMaterialParams> CncToolRepository::findParamsForMaterial(i64 materialId) {
    std::vector<ToolMaterialParams> results;

    auto stmt = m_db.prepare("SELECT * FROM tool_material_params WHERE material_id = ?");
    if (!stmt.isValid()) {
        return results;
    }

    if (!stmt.bindInt(1, materialId)) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToParams(stmt));
    }

    return results;
}

bool CncToolRepository::updateParams(const ToolMaterialParams& params) {
    auto stmt = m_db.prepare(R"(
        UPDATE tool_material_params SET
            feed_rate = ?,
            spindle_speed = ?,
            depth_of_cut = ?,
            chip_load = ?
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindDouble(1, params.feedRate) ||
        !stmt.bindDouble(2, params.spindleSpeed) ||
        !stmt.bindDouble(3, params.depthOfCut) ||
        !stmt.bindDouble(4, params.chipLoad) ||
        !stmt.bindInt(5, params.id)) {
        log::error("CncToolRepo", "Failed to bind updateParams parameters");
        return false;
    }

    return stmt.execute();
}

bool CncToolRepository::removeParams(i64 toolId, i64 materialId) {
    auto stmt = m_db.prepare(
        "DELETE FROM tool_material_params WHERE tool_id = ? AND material_id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    if (!stmt.bindInt(1, toolId) || !stmt.bindInt(2, materialId)) {
        return false;
    }

    if (!stmt.execute()) {
        return false;
    }

    return m_db.changesCount() > 0;
}

CncToolRecord CncToolRepository::rowToTool(Statement& stmt) {
    CncToolRecord tool;
    tool.id = stmt.getInt(0);
    tool.name = stmt.getText(1);
    tool.type = stringToCncToolType(stmt.getText(2));
    tool.diameter = stmt.getDouble(3);
    tool.fluteCount = static_cast<int>(stmt.getInt(4));
    tool.maxRPM = stmt.getDouble(5);
    tool.maxDOC = stmt.getDouble(6);
    tool.shankDiameter = stmt.getDouble(7);
    tool.notes = stmt.getText(8);
    tool.createdAt = stmt.getText(9);
    return tool;
}

ToolMaterialParams CncToolRepository::rowToParams(Statement& stmt) {
    ToolMaterialParams params;
    params.id = stmt.getInt(0);
    params.toolId = stmt.getInt(1);
    params.materialId = stmt.getInt(2);
    params.feedRate = stmt.getDouble(3);
    params.spindleSpeed = stmt.getDouble(4);
    params.depthOfCut = stmt.getDouble(5);
    params.chipLoad = stmt.getDouble(6);
    return params;
}

} // namespace dw
