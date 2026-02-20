// Digital Workshop - File I/O Manager Implementation
// Extracted from Application: import, export, project operations,
// drag-and-drop, completed import processing, recent project opening.

#include "managers/file_io_manager.h"

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/events/event_bus.h"
#include "core/export/model_exporter.h"
#include "core/import/import_queue.h"
#include "core/import/import_task.h"
#include "core/library/library_manager.h"
#include "core/loaders/loader_factory.h"
#include "core/project/project.h"
#include "core/utils/file_utils.h"
#include "core/utils/log.h"
#include "render/thumbnail_generator.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/viewport_panel.h"
#include "ui/widgets/toast.h"

namespace dw {

FileIOManager::FileIOManager(EventBus* eventBus, Database* database, LibraryManager* libraryManager,
                             ProjectManager* projectManager, ImportQueue* importQueue,
                             Workspace* workspace, FileDialog* fileDialog,
                             ThumbnailGenerator* thumbnailGenerator)
    : m_eventBus(eventBus)
    , m_database(database)
    , m_libraryManager(libraryManager)
    , m_projectManager(projectManager)
    , m_importQueue(importQueue)
    , m_workspace(workspace)
    , m_fileDialog(fileDialog)
    , m_thumbnailGenerator(thumbnailGenerator) {}

FileIOManager::~FileIOManager() = default;

void FileIOManager::importModel() {
    if (m_fileDialog) {
        m_fileDialog->showOpenMulti("Import Models", FileDialog::modelFilters(),
                                    [this](const std::vector<std::string>& paths) {
                                        if (paths.empty())
                                            return;

                                        std::vector<Path> importPaths;
                                        importPaths.reserve(paths.size());
                                        for (const auto& p : paths) {
                                            importPaths.emplace_back(p);
                                        }

                                        if (m_importQueue) {
                                            m_importQueue->enqueue(importPaths);
                                        }
                                    });
    }
}

void FileIOManager::exportModel() {
    auto mesh = m_workspace->getFocusedMesh();
    if (!mesh) {
        MessageDialog::warning("No Model", "No model selected to export.");
        return;
    }

    if (m_fileDialog) {
        m_fileDialog->showSave("Export Model", FileDialog::modelFilters(), "model.stl",
                               [this, mesh](const std::string& path) {
                                   if (path.empty())
                                       return;

                                   ModelExporter exporter;
                                   auto result = exporter.exportMesh(*mesh, path);

                                   if (result.success) {
                                       MessageDialog::info("Export Complete",
                                                           "Model exported to:\n" + path);
                                   } else {
                                       MessageDialog::error("Export Failed", result.error);
                                   }
                               });
    }
}

void FileIOManager::collectSupportedFiles(const Path& directory, std::vector<Path>& outPaths) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            // Skip non-regular files (directories, symlinks, etc.)
            if (!entry.is_regular_file()) {
                continue;
            }

            // Get extension and check if supported
            auto ext = file::getExtension(entry.path());
            if (LoaderFactory::isSupported(ext)) {
                outPaths.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Log warning but continue - don't crash on permission denied or broken symlinks
        log::warningf("FileIO", "Failed to scan directory %s: %s", directory.string().c_str(),
                      e.what());
    }
}

void FileIOManager::onFilesDropped(const std::vector<std::string>& paths) {
    if (!m_importQueue)
        return;

    std::vector<Path> importPaths;
    for (const auto& p : paths) {
        Path path{p};

        // Check if this is a directory
        if (fs::is_directory(path)) {
            collectSupportedFiles(path, importPaths);
        } else {
            // Regular file - check extension
            auto ext = path.extension().string();
            if (!ext.empty() && ext[0] == '.')
                ext = ext.substr(1);

            if (LoaderFactory::isSupported(ext)) {
                importPaths.push_back(path);
            }
        }
    }

    if (!importPaths.empty()) {
        m_importQueue->enqueue(importPaths);
    }
}

void FileIOManager::processCompletedImports(ViewportPanel* viewport, PropertiesPanel* properties,
                                            LibraryPanel* library,
                                            std::function<void(bool)> setShowStartPage) {
    // viewport param kept for API compatibility; focus does not change on import (user decision)
    (void)viewport;

    if (!m_importQueue)
        return;

    // Poll for newly completed tasks and add to pending queue
    auto newlyCompleted = m_importQueue->pollCompleted();
    if (!newlyCompleted.empty()) {
        setShowStartPage(false);
        m_pendingCompletions.insert(m_pendingCompletions.end(),
                                    std::make_move_iterator(newlyCompleted.begin()),
                                    std::make_move_iterator(newlyCompleted.end()));
    }

    // Process at most ONE task per frame to avoid blocking the UI
    if (m_pendingCompletions.empty())
        return;

    auto task = std::move(m_pendingCompletions.front());
    m_pendingCompletions.erase(m_pendingCompletions.begin());

    // Generate thumbnail on main thread (needs GL context)
    // Use LibraryManager which writes the file AND updates the DB
    if (m_thumbnailGenerator && task.mesh && m_libraryManager) {
        m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator);
        bool thumbnailOk = m_libraryManager->generateThumbnail(task.modelId, *task.mesh);
        if (!thumbnailOk) {
            // Retry once - framebuffer creation can fail transiently
            thumbnailOk = m_libraryManager->generateThumbnail(task.modelId, *task.mesh);
        }
        if (!thumbnailOk) {
            ToastManager::instance().show(ToastType::Warning, "Thumbnail Failed",
                                          "Could not generate thumbnail for: " + task.record.name);
        }
    }

    // Refresh library to show the newly imported model
    if (library) {
        library->refresh();
    }

    // Set mesh on workspace for properties display (lightweight, not viewport render)
    if (task.mesh) {
        m_workspace->setFocusedMesh(task.mesh);
        if (properties) {
            properties->setMesh(task.mesh, task.record.name);
        }
    }
}

void FileIOManager::newProject(std::function<void(bool)> setShowStartPage) {
    auto current = m_projectManager->currentProject();
    if (current && current->isModified()) {
        MessageDialog::question(
            "Unsaved Changes",
            "Current project has unsaved changes. Save before creating a new project?",
            [this, setShowStartPage](DialogResult result) {
                if (result == DialogResult::Yes) {
                    saveProject();
                }
                if (result != DialogResult::No && result != DialogResult::Yes) {
                    return; // Dialog closed without answering
                }
                auto project = m_projectManager->create("New Project");
                m_projectManager->setCurrentProject(project);
                setShowStartPage(false);
            });
        return;
    }
    auto project = m_projectManager->create("New Project");
    m_projectManager->setCurrentProject(project);
    setShowStartPage(false);
}

void FileIOManager::openProject(std::function<void(bool)> setShowStartPage) {
    if (!m_fileDialog)
        return;

    auto current = m_projectManager->currentProject();
    if (current && current->isModified()) {
        MessageDialog::question(
            "Unsaved Changes",
            "Current project has unsaved changes. Save before opening another project?",
            [this, setShowStartPage](DialogResult result) {
                if (result == DialogResult::Yes) {
                    saveProject();
                }
                if (result != DialogResult::No && result != DialogResult::Yes) {
                    return;
                }
                openProject(setShowStartPage);
            });
        return;
    }

    m_fileDialog->showOpen("Open Project", FileDialog::projectFilters(),
                           [this, setShowStartPage](const std::string& path) {
                               if (path.empty())
                                   return;

                               // Search existing projects for one matching this file path
                               auto projects = m_projectManager->listProjects();
                               for (const auto& record : projects) {
                                   if (record.filePath == Path(path)) {
                                       auto project = m_projectManager->open(record.id);
                                       if (project) {
                                           m_projectManager->setCurrentProject(project);
                                           Config::instance().addRecentProject(Path(path));
                                           Config::instance().save();
                                           setShowStartPage(false);
                                           return;
                                       }
                                   }
                               }

                               // No existing project found at that path - create a new one
                               // and associate the file path
                               Path filePath(path);
                               std::string name = filePath.stem().string();
                               auto project = m_projectManager->create(name);
                               if (project) {
                                   project->setFilePath(filePath);
                                   m_projectManager->setCurrentProject(project);
                                   Config::instance().addRecentProject(filePath);
                                   Config::instance().save();
                                   setShowStartPage(false);
                               }
                           });
}

void FileIOManager::saveProject() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        MessageDialog::warning("No Project", "No project is currently open.");
        return;
    }

    // If project has no file path, show save dialog to pick one
    if (project->filePath().empty()) {
        if (m_fileDialog) {
            std::string defaultName = project->name() + ".dwp";
            m_fileDialog->showSave("Save Project", FileDialog::projectFilters(), defaultName,
                                   [this, project](const std::string& path) {
                                       if (path.empty())
                                           return;
                                       project->setFilePath(Path(path));
                                       m_projectManager->save(*project);
                                       Config::instance().addRecentProject(Path(path));
                                       Config::instance().save();
                                   });
        }
    } else {
        m_projectManager->save(*project);
        if (!project->filePath().empty()) {
            Config::instance().addRecentProject(project->filePath());
            Config::instance().save();
        }
    }
}

void FileIOManager::openRecentProject(const Path& path,
                                      std::function<void(bool)> setShowStartPage) {
    if (!m_projectManager)
        return;

    auto projects = m_projectManager->listProjects();
    for (const auto& record : projects) {
        if (record.filePath == path) {
            auto project = m_projectManager->open(record.id);
            if (project) {
                m_projectManager->setCurrentProject(project);
                Config::instance().addRecentProject(path);
                Config::instance().save();
                setShowStartPage(false);
                return;
            }
        }
    }

    // Project not found in DB -- create new from path
    std::string name = path.stem().string();
    auto project = m_projectManager->create(name);
    if (project) {
        project->setFilePath(path);
        m_projectManager->setCurrentProject(project);
        Config::instance().addRecentProject(path);
        Config::instance().save();
        setShowStartPage(false);
    }
}

} // namespace dw
