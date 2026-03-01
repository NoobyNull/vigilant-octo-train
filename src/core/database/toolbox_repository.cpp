#include "toolbox_repository.h"

namespace dw {

ToolboxRepository::ToolboxRepository(Database& db) : m_db(db) {}

bool ToolboxRepository::addTool(const std::string& geometryId, const std::string& displayName) {
    auto stmt = m_db.prepare(
        "INSERT OR IGNORE INTO toolbox_tools (geometry_id, display_name) VALUES (?, ?)");
    if (!stmt.bindText(1, geometryId)) return false;
    if (!stmt.bindText(2, displayName)) return false;
    return stmt.execute();
}

bool ToolboxRepository::removeTool(const std::string& geometryId) {
    auto stmt = m_db.prepare("DELETE FROM toolbox_tools WHERE geometry_id = ?");
    if (!stmt.bindText(1, geometryId)) return false;
    return stmt.execute();
}

bool ToolboxRepository::hasTool(const std::string& geometryId) const {
    auto stmt = m_db.prepare("SELECT 1 FROM toolbox_tools WHERE geometry_id = ?");
    if (!stmt.bindText(1, geometryId)) return false;
    return stmt.step();
}

std::vector<std::string> ToolboxRepository::getAllGeometryIds() const {
    std::vector<std::string> ids;
    auto stmt = m_db.prepare("SELECT geometry_id FROM toolbox_tools ORDER BY added_at");
    while (stmt.step()) {
        ids.push_back(stmt.getText(0));
    }
    return ids;
}

std::string ToolboxRepository::getDisplayName(const std::string& geometryId) const {
    auto stmt = m_db.prepare("SELECT display_name FROM toolbox_tools WHERE geometry_id = ?");
    if (!stmt.bindText(1, geometryId)) return "";
    if (stmt.step()) {
        return stmt.getText(0);
    }
    return "";
}

bool ToolboxRepository::setDisplayName(const std::string& geometryId, const std::string& name) {
    auto stmt = m_db.prepare("UPDATE toolbox_tools SET display_name = ? WHERE geometry_id = ?");
    if (!stmt.bindText(1, name)) return false;
    if (!stmt.bindText(2, geometryId)) return false;
    return stmt.execute();
}

} // namespace dw
