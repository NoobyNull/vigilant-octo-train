#include "project_repository.h"

#include "../utils/log.h"

namespace dw {

ProjectRepository::ProjectRepository(Database& db) : m_db(db) {}

std::optional<i64> ProjectRepository::insert(const ProjectRecord& project) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO projects (name, description, file_path)
        VALUES (?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return std::nullopt;
    }

    stmt.bindText(1, project.name);
    stmt.bindText(2, project.description);
    stmt.bindText(3, project.filePath.string());

    if (!stmt.execute()) {
        log::errorf("ProjectRepo", "Failed to insert project: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

std::optional<ProjectRecord> ProjectRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM projects WHERE id = ?");
    if (!stmt.isValid()) {
        return std::nullopt;
    }

    stmt.bindInt(1, id);

    if (stmt.step()) {
        return rowToProject(stmt);
    }

    return std::nullopt;
}

std::vector<ProjectRecord> ProjectRepository::findAll() {
    std::vector<ProjectRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM projects ORDER BY modified_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    while (stmt.step()) {
        results.push_back(rowToProject(stmt));
    }

    return results;
}

std::vector<ProjectRecord> ProjectRepository::findByName(const std::string& searchTerm) {
    std::vector<ProjectRecord> results;

    auto stmt = m_db.prepare(
        "SELECT * FROM projects WHERE name LIKE ? ORDER BY modified_at DESC");
    if (!stmt.isValid()) {
        return results;
    }

    stmt.bindText(1, "%" + searchTerm + "%");

    while (stmt.step()) {
        results.push_back(rowToProject(stmt));
    }

    return results;
}

bool ProjectRepository::update(const ProjectRecord& project) {
    auto stmt = m_db.prepare(R"(
        UPDATE projects SET
            name = ?,
            description = ?,
            file_path = ?,
            modified_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindText(1, project.name);
    stmt.bindText(2, project.description);
    stmt.bindText(3, project.filePath.string());
    stmt.bindInt(4, project.id);

    return stmt.execute();
}

bool ProjectRepository::updateModifiedTime(i64 id) {
    auto stmt =
        m_db.prepare("UPDATE projects SET modified_at = CURRENT_TIMESTAMP WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, id);
    return stmt.execute();
}

bool ProjectRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM projects WHERE id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, id);
    return stmt.execute();
}

bool ProjectRepository::addModel(i64 projectId, i64 modelId, int sortOrder) {
    auto stmt = m_db.prepare(R"(
        INSERT OR REPLACE INTO project_models (project_id, model_id, sort_order)
        VALUES (?, ?, ?)
    )");

    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, projectId);
    stmt.bindInt(2, modelId);
    stmt.bindInt(3, sortOrder);

    bool result = stmt.execute();
    if (result) {
        updateModifiedTime(projectId);
    }
    return result;
}

bool ProjectRepository::removeModel(i64 projectId, i64 modelId) {
    auto stmt =
        m_db.prepare("DELETE FROM project_models WHERE project_id = ? AND model_id = ?");
    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, projectId);
    stmt.bindInt(2, modelId);

    bool result = stmt.execute();
    if (result) {
        updateModifiedTime(projectId);
    }
    return result;
}

bool ProjectRepository::updateModelOrder(i64 projectId, i64 modelId, int sortOrder) {
    auto stmt = m_db.prepare(R"(
        UPDATE project_models SET sort_order = ?
        WHERE project_id = ? AND model_id = ?
    )");

    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, sortOrder);
    stmt.bindInt(2, projectId);
    stmt.bindInt(3, modelId);

    return stmt.execute();
}

std::vector<i64> ProjectRepository::getModelIds(i64 projectId) {
    std::vector<i64> ids;

    auto stmt = m_db.prepare(
        "SELECT model_id FROM project_models WHERE project_id = ? ORDER BY sort_order");
    if (!stmt.isValid()) {
        return ids;
    }

    stmt.bindInt(1, projectId);

    while (stmt.step()) {
        ids.push_back(stmt.getInt(0));
    }

    return ids;
}

std::vector<i64> ProjectRepository::getProjectsForModel(i64 modelId) {
    std::vector<i64> ids;

    auto stmt = m_db.prepare("SELECT project_id FROM project_models WHERE model_id = ?");
    if (!stmt.isValid()) {
        return ids;
    }

    stmt.bindInt(1, modelId);

    while (stmt.step()) {
        ids.push_back(stmt.getInt(0));
    }

    return ids;
}

bool ProjectRepository::hasModel(i64 projectId, i64 modelId) {
    auto stmt = m_db.prepare(
        "SELECT 1 FROM project_models WHERE project_id = ? AND model_id = ? LIMIT 1");
    if (!stmt.isValid()) {
        return false;
    }

    stmt.bindInt(1, projectId);
    stmt.bindInt(2, modelId);

    return stmt.step();
}

i64 ProjectRepository::count() {
    auto stmt = m_db.prepare("SELECT COUNT(*) FROM projects");
    if (!stmt.isValid()) {
        return 0;
    }

    if (stmt.step()) {
        return stmt.getInt(0);
    }

    return 0;
}

ProjectRecord ProjectRepository::rowToProject(Statement& stmt) {
    ProjectRecord project;
    project.id = stmt.getInt(0);
    project.name = stmt.getText(1);
    project.description = stmt.getText(2);
    project.filePath = stmt.getText(3);
    project.createdAt = stmt.getText(4);
    project.modifiedAt = stmt.getText(5);
    return project;
}

}  // namespace dw
