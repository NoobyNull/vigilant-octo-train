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
struct ImportTask;
class Workspace;
class FileDialog;
class ThumbnailGenerator;
class Mesh;
class ViewportPanel;
class PropertiesPanel;
class LibraryPanel;
class ImportOptionsDialog;

class FileIOManager {
  public:
    FileIOManager(EventBus* eventBus, Database* database, LibraryManager* libraryManager,
                  ProjectManager* projectManager, ImportQueue* importQueue, Workspace* workspace,
                  FileDialog* fileDialog, ThumbnailGenerator* thumbnailGenerator);
    ~FileIOManager();

    // Optional callback for thumbnail generation with material support.
    // Signature: (modelId, mesh) -> bool. When set, replaces default thumbnail generation.
    using ThumbnailCallback = std::function<bool(int64_t modelId, Mesh& mesh)>;
    void setThumbnailCallback(ThumbnailCallback cb) { m_thumbnailCallback = std::move(cb); }

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
    // Recursive directory scanning helper
    void collectSupportedFiles(const Path& directory, std::vector<Path>& outPaths);

    EventBus* m_eventBus;
    Database* m_database;
    LibraryManager* m_libraryManager;
    ProjectManager* m_projectManager;
    ImportQueue* m_importQueue;
    Workspace* m_workspace;
    FileDialog* m_fileDialog;
    ThumbnailGenerator* m_thumbnailGenerator;

    // Pending completions queue for throttled processing (one per frame)
    std::vector<ImportTask> m_pendingCompletions;

    // Optional material-aware thumbnail callback
    ThumbnailCallback m_thumbnailCallback;

    // Import options dialog (owned by UIManager, nullable)
    ImportOptionsDialog* m_importOptionsDialog = nullptr;

  public:
    void setImportOptionsDialog(ImportOptionsDialog* dialog) { m_importOptionsDialog = dialog; }
};

} // namespace dw
