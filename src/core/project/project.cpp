#include "project.h"

#include "../database/database.h"
#include "../utils/log.h"

#include <algorithm>

namespace dw {

// Project implementation

void Project::addModel(i64 modelId) {
    if (!hasModel(modelId)) {
        m_modelIds.push_back(modelId);
        m_modified = true;
    }
}

void Project::removeModel(i64 modelId) {
    auto it = std::find(m_modelIds.begin(), m_modelIds.end(), modelId);
    if (it != m_modelIds.end()) {
        m_modelIds.erase(it);
        m_modified = true;
    }
}

void Project::reorderModel(i64 modelId, int newPosition) {
    auto it = std::find(m_modelIds.begin(), m_modelIds.end(), modelId);
    if (it == m_modelIds.end()) {
        return;
    }

    m_modelIds.erase(it);

    if (newPosition < 0) {
        newPosition = 0;
    }
    if (newPosition > static_cast<int>(m_modelIds.size())) {
        newPosition = static_cast<int>(m_modelIds.size());
    }

    m_modelIds.insert(m_modelIds.begin() + newPosition, modelId);
    m_modified = true;
}

bool Project::hasModel(i64 modelId) const {
    return std::find(m_modelIds.begin(), m_modelIds.end(), modelId) != m_modelIds.end();
}

// ProjectManager implementation

ProjectManager::ProjectManager(Database& db) : m_db(db), m_projectRepo(db) {}

std::shared_ptr<Project> ProjectManager::create(const std::string& name) {
    ProjectRecord record;
    record.name = name;

    auto id = m_projectRepo.insert(record);
    if (!id) {
        log::error("Failed to create project in database");
        return nullptr;
    }

    auto project = std::make_shared<Project>();
    project->record().id = *id;
    project->record().name = name;

    log::infof("Created project: %s (ID: %lld)", name.c_str(),
               static_cast<long long>(*id));

    return project;
}

std::shared_ptr<Project> ProjectManager::open(i64 projectId) {
    auto record = m_projectRepo.findById(projectId);
    if (!record) {
        log::errorf("Project not found: %lld", static_cast<long long>(projectId));
        return nullptr;
    }

    auto project = std::make_shared<Project>();
    project->record() = *record;

    // Load model IDs
    auto modelIds = m_projectRepo.getModelIds(projectId);
    for (i64 id : modelIds) {
        project->addModel(id);
    }
    project->clearModified();

    log::infof("Opened project: %s (ID: %lld)", record->name.c_str(),
               static_cast<long long>(projectId));

    return project;
}

bool ProjectManager::save(Project& project) {
    // Update project record
    if (!m_projectRepo.update(project.record())) {
        log::error("Failed to update project record");
        return false;
    }

    // Sync model associations
    auto currentDbIds = m_projectRepo.getModelIds(project.id());

    // Remove models no longer in project
    for (i64 dbId : currentDbIds) {
        if (!project.hasModel(dbId)) {
            m_projectRepo.removeModel(project.id(), dbId);
        }
    }

    // Add new models and update order
    const auto& projectIds = project.modelIds();
    for (int i = 0; i < static_cast<int>(projectIds.size()); ++i) {
        i64 modelId = projectIds[static_cast<usize>(i)];
        if (!m_projectRepo.hasModel(project.id(), modelId)) {
            m_projectRepo.addModel(project.id(), modelId, i);
        } else {
            m_projectRepo.updateModelOrder(project.id(), modelId, i);
        }
    }

    project.clearModified();
    log::infof("Saved project: %s", project.name().c_str());
    return true;
}

bool ProjectManager::close(Project& project) {
    if (project.isModified()) {
        // Caller should handle save confirmation
        log::warning("Closing modified project without saving");
    }

    log::infof("Closed project: %s", project.name().c_str());
    return true;
}

bool ProjectManager::remove(i64 projectId) {
    if (!m_projectRepo.remove(projectId)) {
        log::errorf("Failed to remove project: %lld", static_cast<long long>(projectId));
        return false;
    }

    log::infof("Removed project: %lld", static_cast<long long>(projectId));
    return true;
}

std::vector<ProjectRecord> ProjectManager::listProjects() {
    return m_projectRepo.findAll();
}

std::optional<ProjectRecord> ProjectManager::getProjectInfo(i64 projectId) {
    return m_projectRepo.findById(projectId);
}

bool ProjectManager::addModelToProject(i64 modelId) {
    if (!m_currentProject) {
        log::warning("No project open");
        return false;
    }

    m_currentProject->addModel(modelId);
    return true;
}

bool ProjectManager::removeModelFromProject(i64 modelId) {
    if (!m_currentProject) {
        log::warning("No project open");
        return false;
    }

    m_currentProject->removeModel(modelId);
    return true;
}

}  // namespace dw
