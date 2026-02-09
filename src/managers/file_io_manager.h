#pragma once

// Digital Workshop - File I/O Manager
// Orchestrates all file I/O operations: import, export, project
// new/open/save, drag-and-drop, completed import processing,
// recent project opening. Extracted from Application (god class
// decomposition Wave 2).

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../core/types.h"

namespace dw {

// Forward declarations
class EventBus;
class Database;
class LibraryManager;
class ProjectManager;
class ImportQueue;
class Workspace;
class FileDialog;
class ThumbnailGenerator;
class ViewportPanel;
class PropertiesPanel;
class LibraryPanel;

class FileIOManager {
  public:
    FileIOManager(EventBus* eventBus, Database* database, LibraryManager* libraryManager,
                  ProjectManager* projectManager, ImportQueue* importQueue, Workspace* workspace,
                  FileDialog* fileDialog, ThumbnailGenerator* thumbnailGenerator);
    ~FileIOManager();

    // Import/Export
    void importModel();
    void exportModel();
    void onFilesDropped(const std::vector<std::string>& paths);
    void processCompletedImports(ViewportPanel* viewport, PropertiesPanel* properties,
                                 LibraryPanel* library, std::function<void(bool)> setShowStartPage);

    // Project operations
    void newProject(std::function<void(bool)> setShowStartPage);
    void openProject(std::function<void(bool)> setShowStartPage);
    void saveProject();
    void openRecentProject(const Path& path, std::function<void(bool)> setShowStartPage);

  private:
    EventBus* m_eventBus;
    Database* m_database;
    LibraryManager* m_libraryManager;
    ProjectManager* m_projectManager;
    ImportQueue* m_importQueue;
    Workspace* m_workspace;
    FileDialog* m_fileDialog;
    ThumbnailGenerator* m_thumbnailGenerator;
};

} // namespace dw
