#include "project.h"

#include <algorithm>

#include "project_directory.h"
#include "../config/config.h"
#include "../database/database.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

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
        log::error("Project", "Failed to create in database");
        return nullptr;
    }

    auto project = std::make_shared<Project>();
    project->record().id = *id;
    project->record().name = name;

    log::infof("Project", "Created: %s (ID: %lld)", name.c_str(), static_cast<long long>(*id));

    return project;
}

std::shared_ptr<Project> ProjectManager::open(i64 projectId) {
    auto record = m_projectRepo.findById(projectId);
    if (!record) {
        log::errorf("Project", "Not found: %lld", static_cast<long long>(projectId));
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

    log::infof("Project",
               "Opened: %s (ID: %lld)",
               record->name.c_str(),
               static_cast<long long>(projectId));

    return project;
}

bool ProjectManager::save(Project& project) {
    // Update project record
    if (!m_projectRepo.update(project.record())) {
        log::error("Project", "Failed to update record");
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
    log::infof("Project", "Saved: %s", project.name().c_str());
    return true;
}

bool ProjectManager::close(Project& project) {
    if (project.isModified()) {
        // Caller should handle save confirmation
        log::warning("Project", "Closing modified project without saving");
    }

    log::infof("Project", "Closed: %s", project.name().c_str());
    return true;
}

bool ProjectManager::remove(i64 projectId) {
    if (!m_projectRepo.remove(projectId)) {
        log::errorf("Project", "Failed to remove: %lld", static_cast<long long>(projectId));
        return false;
    }

    log::infof("Project", "Removed: %lld", static_cast<long long>(projectId));
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
        log::warning("Project", "No project open");
        return false;
    }

    m_currentProject->addModel(modelId);
    return true;
}

bool ProjectManager::removeModelFromProject(i64 modelId) {
    if (!m_currentProject) {
        log::warning("Project", "No project open");
        return false;
    }

    m_currentProject->removeModel(modelId);
    return true;
}

std::shared_ptr<ProjectDirectory> ProjectManager::ensureProjectForModel(
    const std::string& modelName, const Path& modelSourcePath) {

    std::string dirName = ProjectDirectory::sanitizeName(modelName);
    Path projectRoot = Config::instance().getProjectsDir() / dirName;

    auto dir = std::make_shared<ProjectDirectory>();

    if (file::isDirectory(projectRoot) && file::exists(projectRoot / "project.json")) {
        if (!dir->open(projectRoot)) {
            log::errorf("Project", "Failed to open existing project dir: %s",
                        projectRoot.c_str());
            return nullptr;
        }
    } else {
        if (!dir->create(projectRoot, modelName)) {
            log::errorf("Project", "Failed to create project dir: %s",
                        projectRoot.c_str());
            return nullptr;
        }
        if (!modelSourcePath.empty() && file::isFile(modelSourcePath)) {
            dir->addModelFile(modelSourcePath);
            dir->save();
        }
    }

    // Sync with SQLite — find or create a ProjectRecord with matching filePath
    auto records = m_projectRepo.findAll();
    std::shared_ptr<Project> project;
    for (const auto& rec : records) {
        if (rec.filePath == projectRoot) {
            project = open(rec.id);
            break;
        }
    }
    if (!project) {
        project = create(modelName);
        if (project) {
            project->setFilePath(projectRoot);
            save(*project);
        }
    }

    if (project) {
        m_currentProject = project;
    }
    m_currentDir = dir;

    Config::instance().addRecentProject(projectRoot);
    return dir;
}

} // namespace dw
