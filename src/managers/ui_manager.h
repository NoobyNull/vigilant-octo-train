#pragma once

// Digital Workshop - UI Manager
// Owns all UI panels, dialogs, visibility state, menu bar,
// keyboard shortcuts, import progress overlay, about/restart popups,
// and dock layout. Extracted from Application (god class decomposition).

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../core/types.h"

using ImGuiID = unsigned int;

namespace dw {

// Forward declarations - panels
class ViewportPanel;
class LibraryPanel;
class PropertiesPanel;
class ProjectPanel;
class GCodePanel;
class CutOptimizerPanel;
class MaterialsPanel;
class StartPage;

// Forward declarations - dialogs
class FileDialog;
class LightingDialog;
class ImportSummaryDialog;

// Forward declarations - core
struct LoadingState;
struct ImportBatchSummary;
struct ImportProgress;
class LibraryManager;
class ProjectManager;
class MaterialManager;
class ImportQueue;
class Workspace;

// Forward declarations - widgets
class StatusBar;

// Callback types for actions that remain in Application
using ActionCallback = std::function<void()>;
using ModelIdCallback = std::function<void(int64_t)>;
using PathCallback = std::function<void(const Path&)>;
using PathsCallback = std::function<void(const std::vector<std::string>&)>;

class UIManager {
  public:
    UIManager();
    ~UIManager();

    // Disable copy/move
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) = delete;
    UIManager& operator=(UIManager&&) = delete;

    // Initialize all panels and dialogs.
    // Does NOT wire StartPage callbacks (Application does that after
    // both UIManager and FileIOManager exist).
    void init(LibraryManager* libraryManager, ProjectManager* projectManager,
              MaterialManager* materialManager);

    // Shutdown and destroy all UI resources
    void shutdown();

    // --- Per-frame rendering ---
    void renderMenuBar();
    void renderPanels();
    void renderAboutDialog();
    void renderRestartPopup(const ActionCallback& onRelaunch);
    void setupDefaultDockLayout(ImGuiID dockspaceId);
    void handleKeyboardShortcuts();

    // --- New background UI methods (Plan 02-05) ---
    void renderBackgroundUI(float deltaTime, const LoadingState* loadingState);  // Renders StatusBar, ToastManager, ImportSummaryDialog

    // --- Dock layout first-frame logic ---
    [[nodiscard]] bool isFirstFrame() const { return m_firstFrame; }
    void clearFirstFrame() { m_firstFrame = false; }

    // --- Panel accessors (Application needs these for wiring callbacks) ---
    ViewportPanel* viewportPanel() { return m_viewportPanel.get(); }
    LibraryPanel* libraryPanel() { return m_libraryPanel.get(); }
    PropertiesPanel* propertiesPanel() { return m_propertiesPanel.get(); }
    ProjectPanel* projectPanel() { return m_projectPanel.get(); }
    GCodePanel* gcodePanel() { return m_gcodePanel.get(); }
    CutOptimizerPanel* cutOptimizerPanel() { return m_cutOptimizerPanel.get(); }
    MaterialsPanel* materialsPanel() { return m_materialsPanel.get(); }
    StartPage* startPage() { return m_startPage.get(); }
    FileDialog* fileDialog() { return m_fileDialog.get(); }
    LightingDialog* lightingDialog() { return m_lightingDialog.get(); }
    ImportSummaryDialog* importSummaryDialog() { return m_importSummaryDialog.get(); }

    // --- Visibility state ---
    bool& showViewport() { return m_showViewport; }
    bool& showLibrary() { return m_showLibrary; }
    bool& showProperties() { return m_showProperties; }
    bool& showProject() { return m_showProject; }
    bool& showGCode() { return m_showGCode; }
    bool& showCutOptimizer() { return m_showCutOptimizer; }
    bool& showMaterials() { return m_showMaterials; }
    bool& showStartPage() { return m_showStartPage; }
    bool& showRestartPopup() { return m_showRestartPopup; }

    // --- Action callbacks (set by Application) ---
    void setOnNewProject(ActionCallback cb) { m_onNewProject = std::move(cb); }
    void setOnOpenProject(ActionCallback cb) { m_onOpenProject = std::move(cb); }
    void setOnSaveProject(ActionCallback cb) { m_onSaveProject = std::move(cb); }
    void setOnImportModel(ActionCallback cb) { m_onImportModel = std::move(cb); }
    void setOnExportModel(ActionCallback cb) { m_onExportModel = std::move(cb); }
    void setOnQuit(ActionCallback cb) { m_onQuit = std::move(cb); }
    void setOnSpawnSettings(ActionCallback cb) { m_onSpawnSettings = std::move(cb); }
    void setOnShowAbout(ActionCallback cb) { m_onShowAbout = std::move(cb); }

    // --- Import progress callbacks (Plan 02-05) ---
    void setImportProgress(const ImportProgress* progress);
    void showImportSummary(const ImportBatchSummary& summary);
    void setImportCancelCallback(std::function<void()> callback);

    // --- Workspace state save/restore helpers ---
    void restoreVisibilityFromConfig();
    void saveVisibilityToConfig();
    void applyRenderSettingsFromConfig();
    [[nodiscard]] int64_t getSelectedModelId() const;

  private:
    // UI Panels
    std::unique_ptr<ViewportPanel> m_viewportPanel;
    std::unique_ptr<LibraryPanel> m_libraryPanel;
    std::unique_ptr<PropertiesPanel> m_propertiesPanel;
    std::unique_ptr<ProjectPanel> m_projectPanel;
    std::unique_ptr<GCodePanel> m_gcodePanel;
    std::unique_ptr<CutOptimizerPanel> m_cutOptimizerPanel;
    std::unique_ptr<MaterialsPanel> m_materialsPanel;
    std::unique_ptr<StartPage> m_startPage;

    // Panel visibility
    bool m_showViewport = true;
    bool m_showLibrary = true;
    bool m_showProperties = true;
    bool m_showProject = true;
    bool m_showGCode = false;
    bool m_showCutOptimizer = false;
    bool m_showMaterials = false;
    bool m_showStartPage = true;

    // Dialogs
    std::unique_ptr<FileDialog> m_fileDialog;
    std::unique_ptr<LightingDialog> m_lightingDialog;
    std::unique_ptr<ImportSummaryDialog> m_importSummaryDialog;

    // Widgets (Plan 02-05)
    std::unique_ptr<StatusBar> m_statusBar;

    // Restart popup state
    bool m_showRestartPopup = false;

    // First frame flag for dock layout
    bool m_firstFrame = true;

    // Menu rendering helpers (extracted from renderMenuBar for complexity)
    void renderFileMenu();
    void renderViewMenu();
    void renderEditMenu();
    void renderHelpMenu();

    // Action callbacks (delegated to Application)
    ActionCallback m_onNewProject;
    ActionCallback m_onOpenProject;
    ActionCallback m_onSaveProject;
    ActionCallback m_onImportModel;
    ActionCallback m_onExportModel;
    ActionCallback m_onQuit;
    ActionCallback m_onSpawnSettings;
    ActionCallback m_onShowAbout;
};

} // namespace dw
