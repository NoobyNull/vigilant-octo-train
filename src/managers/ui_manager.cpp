// Digital Workshop - UI Manager Implementation
// Extracted from Application: panel ownership, visibility, menus,
// keyboard shortcuts, dialogs, import progress, about/restart popups,
// dock layout.

#include "managers/ui_manager.h"

#include <array>

#include <imgui.h>
#include <imgui_internal.h>

#include "core/config/config.h"
#include "core/import/import_queue.h"
#include "core/import/import_task.h"
#include "core/threading/loading_state.h"
#include "core/utils/thread_utils.h"
#include "ui/context_menu_manager.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/dialogs/import_options_dialog.h"
#include "ui/dialogs/import_summary_dialog.h"
#include "ui/dialogs/lighting_dialog.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/dialogs/progress_dialog.h"
#include "ui/panels/cost_panel.h"
#include "ui/panels/cut_optimizer_panel.h"
#include "ui/panels/gcode_panel.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/materials_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/start_page.h"
#include "ui/panels/viewport_panel.h"
#include "ui/widgets/status_bar.h"
#include "ui/widgets/toast.h"
#include "version.h"

namespace dw {

UIManager::UIManager() = default;

UIManager::~UIManager() {
    shutdown();
}

void UIManager::init(LibraryManager* libraryManager, ProjectManager* projectManager,
                     MaterialManager* materialManager, CostRepository* costRepo) {
    // Create panels
    m_viewportPanel = std::make_unique<ViewportPanel>();
    m_libraryPanel = std::make_unique<LibraryPanel>(libraryManager);
    m_propertiesPanel = std::make_unique<PropertiesPanel>();
    m_projectPanel = std::make_unique<ProjectPanel>(projectManager);
    m_gcodePanel = std::make_unique<GCodePanel>();
    m_cutOptimizerPanel = std::make_unique<CutOptimizerPanel>();
    m_materialsPanel = std::make_unique<MaterialsPanel>(materialManager);
    if (costRepo) {
        m_costPanel = std::make_unique<CostPanel>(costRepo);
    }
    m_startPage = std::make_unique<StartPage>();

    // Create dialogs
    m_fileDialog = std::make_unique<FileDialog>();
    m_lightingDialog = std::make_unique<LightingDialog>();
    m_importSummaryDialog = std::make_unique<ImportSummaryDialog>();
    m_importOptionsDialog = std::make_unique<ImportOptionsDialog>();
    m_progressDialog = std::make_unique<ProgressDialog>();

    // Create widgets
    m_statusBar = std::make_unique<StatusBar>();
    m_contextMenuManager = std::make_unique<ContextMenuManager>();

    // Connect context menu manager to library panel
    if (m_libraryPanel) {
        m_libraryPanel->setContextMenuManager(m_contextMenuManager.get());
    }

    // Connect context menu manager to materials panel
    if (m_materialsPanel) {
        m_materialsPanel->setContextMenuManager(m_contextMenuManager.get());
    }

    // Connect context menu manager to viewport panel
    if (m_viewportPanel) {
        m_viewportPanel->setContextMenuManager(m_contextMenuManager.get());
    }

    // Connect viewport render settings to lighting dialog
    if (m_viewportPanel) {
        m_lightingDialog->setSettings(&m_viewportPanel->renderSettings());
    }

    // Connect file dialog to G-code panel
    if (m_gcodePanel) {
        m_gcodePanel->setFileDialog(m_fileDialog.get());
    }

    // NOTE: StartPage callbacks are NOT wired here.
    // Application wires those after both UIManager and FileIOManager exist.
}

void UIManager::shutdown() {
    // Destroy dialogs
    m_fileDialog.reset();
    m_lightingDialog.reset();
    m_importSummaryDialog.reset();
    m_importOptionsDialog.reset();
    m_progressDialog.reset();

    // Destroy widgets
    m_statusBar.reset();

    // Destroy panels
    m_viewportPanel.reset();
    m_libraryPanel.reset();
    m_propertiesPanel.reset();
    m_projectPanel.reset();
    m_gcodePanel.reset();
    m_cutOptimizerPanel.reset();
    m_costPanel.reset();
    m_materialsPanel.reset();
    m_startPage.reset();
}

void UIManager::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        renderFileMenu();
        renderViewMenu();
        renderEditMenu();
        renderHelpMenu();
        ImGui::EndMainMenuBar();
    }
}

void UIManager::renderFileMenu() {
    if (!ImGui::BeginMenu("File")) {
        return;
    }

    if (ImGui::MenuItem("New Project", "Ctrl+N") && m_onNewProject) {
        m_onNewProject();
    }
    if (ImGui::MenuItem("Open Project", "Ctrl+O") && m_onOpenProject) {
        m_onOpenProject();
    }
    if (ImGui::MenuItem("Save Project", "Ctrl+S") && m_onSaveProject) {
        m_onSaveProject();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Import Model", "Ctrl+I") && m_onImportModel) {
        m_onImportModel();
    }
    if (ImGui::MenuItem("Export Model", "Ctrl+E") && m_onExportModel) {
        m_onExportModel();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4") && m_onQuit) {
        m_onQuit();
    }
    ImGui::EndMenu();
}

void UIManager::renderViewMenu() {
    if (!ImGui::BeginMenu("View")) {
        return;
    }

    ImGui::MenuItem("Start Page", nullptr, &m_showStartPage);
    ImGui::Separator();
    ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
    ImGui::MenuItem("Library", nullptr, &m_showLibrary);
    ImGui::MenuItem("Properties", nullptr, &m_showProperties);
    ImGui::MenuItem("Project", nullptr, &m_showProject);
    ImGui::Separator();
    ImGui::MenuItem("G-code Viewer", nullptr, &m_showGCode);
    ImGui::MenuItem("Cut Optimizer", nullptr, &m_showCutOptimizer);
    ImGui::MenuItem("Cost Estimator", nullptr, &m_showCostEstimator);
    ImGui::MenuItem("Materials", nullptr, &m_showMaterials);
    ImGui::Separator();
    if (ImGui::MenuItem("Lighting Settings", "Ctrl+L") && m_lightingDialog) {
        m_lightingDialog->open();
    }
    ImGui::EndMenu();
}

void UIManager::renderEditMenu() {
    if (!ImGui::BeginMenu("Edit")) {
        return;
    }

    if (ImGui::MenuItem("Settings", "Ctrl+,") && m_onSpawnSettings) {
        m_onSpawnSettings();
    }
    ImGui::EndMenu();
}

void UIManager::renderHelpMenu() {
    if (!ImGui::BeginMenu("Help")) {
        return;
    }

    if (ImGui::MenuItem("About Digital Workshop")) {
        ImGui::OpenPopup("About Digital Workshop");
    }
    ImGui::EndMenu();
}

void UIManager::renderPanels() {
    // Assert we're on the main thread (ImGui is not thread-safe)
    ASSERT_MAIN_THREAD();

    // Start page
    if (m_showStartPage && m_startPage) {
        m_startPage->render();
    }

    if (m_showViewport && m_viewportPanel) {
        m_viewportPanel->render();
    }

    if (m_showLibrary && m_libraryPanel) {
        m_libraryPanel->render();
    }

    if (m_showProperties && m_propertiesPanel) {
        m_propertiesPanel->render();
    }

    if (m_showProject && m_projectPanel) {
        m_projectPanel->render();
    }

    if (m_showGCode && m_gcodePanel) {
        m_gcodePanel->render();
    }

    if (m_showCutOptimizer && m_cutOptimizerPanel) {
        m_cutOptimizerPanel->render();
    }

    if (m_showCostEstimator && m_costPanel) {
        m_costPanel->render();
        // Sync: if user closed panel via X button, update menu checkbox state
        if (!m_costPanel->isOpen()) {
            m_showCostEstimator = false;
            m_costPanel->setOpen(true); // reset for next View menu toggle
        }
    }

    if (m_showMaterials && m_materialsPanel) {
        m_materialsPanel->render();
        // Sync: if user closed panel via X button, update menu checkbox state
        if (!m_materialsPanel->isOpen()) {
            m_showMaterials = false;
            m_materialsPanel->setOpen(true); // reset for next View menu toggle
        }
    }

    // Render dialogs
    if (m_fileDialog) {
        m_fileDialog->render();
    }

    if (m_lightingDialog) {
        m_lightingDialog->render();
    }

    if (m_importSummaryDialog) {
        m_importSummaryDialog->render();
    }

    if (m_importOptionsDialog) {
        m_importOptionsDialog->render();
    }
}

void UIManager::renderAboutDialog() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About Digital Workshop", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Digital Workshop");
        ImGui::Text("Version %s", VERSION);
        ImGui::Separator();
        ImGui::TextWrapped("A 3D model management application for CNC and 3D printing workflows.");
        ImGui::Spacing();
        ImGui::Text("Libraries:");
        ImGui::BulletText("SDL2 - Window management");
        ImGui::BulletText("Dear ImGui - User interface");
        ImGui::BulletText("OpenGL 3.3 - 3D rendering");
        ImGui::BulletText("SQLite3 - Database");
        ImGui::Separator();
        ImGui::TextDisabled("Built with C++17");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void UIManager::renderRestartPopup(const ActionCallback& onRelaunch) {
    if (!m_showRestartPopup) {
        return;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Restart Required", &m_showRestartPopup,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("UI Scale has been changed.");
        ImGui::Text("A restart is required to apply this setting.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Relaunch Now", ImVec2(140, 0))) {
            m_showRestartPopup = false;
            if (onRelaunch) {
                onRelaunch();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Later", ImVec2(140, 0))) {
            m_showRestartPopup = false;
        }
    }
    ImGui::End();
}

void UIManager::handleKeyboardShortcuts() {
    auto& io = ImGui::GetIO();

    // Only handle shortcuts when not typing in a text field
    if (io.WantTextInput) {
        return;
    }

    if (!io.KeyCtrl) {
        return;
    }

    // Ctrl+key shortcut dispatch
    struct Shortcut {
        ImGuiKey key;
        const ActionCallback* action;
    };
    const std::array<Shortcut, 6> kShortcuts = {{
        {ImGuiKey_N, &m_onNewProject},
        {ImGuiKey_O, &m_onOpenProject},
        {ImGuiKey_S, &m_onSaveProject},
        {ImGuiKey_I, &m_onImportModel},
        {ImGuiKey_E, &m_onExportModel},
        {ImGuiKey_Comma, &m_onSpawnSettings},
    }};

    for (const auto& [key, action] : kShortcuts) {
        if (ImGui::IsKeyPressed(key) && *action) {
            (*action)();
            return;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_L) && m_lightingDialog) {
        m_lightingDialog->open();
    }
}

void UIManager::setupDefaultDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    // Split: left sidebar (20%) | center+right
    ImGuiID dockLeft = 0;
    ImGuiID dockCenterRight = 0;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenterRight);

    // Split left sidebar: library (top 60%) | project (bottom 40%)
    ImGuiID dockLeftTop = 0;
    ImGuiID dockLeftBottom = 0;
    ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.40f, &dockLeftBottom, &dockLeftTop);

    // Split center+right: center | right sidebar (22%) for properties
    ImGuiID dockCenter = 0;
    ImGuiID dockRight = 0;
    ImGui::DockBuilderSplitNode(dockCenterRight, ImGuiDir_Right, 0.22f, &dockRight, &dockCenter);

    // Dock windows into their positions
    ImGui::DockBuilderDockWindow("Library", dockLeftTop);
    ImGui::DockBuilderDockWindow("Project", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    ImGui::DockBuilderDockWindow("Properties", dockRight);

    ImGui::DockBuilderFinish(dockspaceId);
}

void UIManager::restoreVisibilityFromConfig() {
    auto& cfg = Config::instance();
    m_showViewport = cfg.getShowViewport();
    m_showLibrary = cfg.getShowLibrary();
    m_showProperties = cfg.getShowProperties();
    m_showProject = cfg.getShowProject();
    m_showMaterials = cfg.getShowMaterials();
    m_showGCode = cfg.getShowGCode();
    m_showCutOptimizer = cfg.getShowCutOptimizer();
    m_showCostEstimator = cfg.getShowCostEstimator();
    m_showStartPage = cfg.getShowStartPage();
}

void UIManager::saveVisibilityToConfig() {
    auto& cfg = Config::instance();
    cfg.setShowViewport(m_showViewport);
    cfg.setShowLibrary(m_showLibrary);
    cfg.setShowProperties(m_showProperties);
    cfg.setShowProject(m_showProject);
    cfg.setShowMaterials(m_showMaterials);
    cfg.setShowGCode(m_showGCode);
    cfg.setShowCutOptimizer(m_showCutOptimizer);
    cfg.setShowCostEstimator(m_showCostEstimator);
    cfg.setShowStartPage(m_showStartPage);
}

void UIManager::applyRenderSettingsFromConfig() {
    if (!m_viewportPanel) {
        return;
    }

    auto& cfg = Config::instance();
    auto& rs = m_viewportPanel->renderSettings();
    rs.lightDir = cfg.getRenderLightDir();
    rs.lightColor = cfg.getRenderLightColor();
    rs.ambient = cfg.getRenderAmbient();
    rs.objectColor = cfg.getRenderObjectColor();
    rs.shininess = cfg.getRenderShininess();
    rs.showGrid = cfg.getShowGrid();
    rs.showAxis = cfg.getShowAxis();
}

int64_t UIManager::getSelectedModelId() const {
    if (m_libraryPanel) {
        return m_libraryPanel->selectedModelId();
    }
    return -1;
}

void UIManager::renderBackgroundUI(float deltaTime, const LoadingState* loadingState) {
    // Render status bar (replaces old renderStatusBar and renderImportProgress)
    if (m_statusBar) {
        m_statusBar->render(loadingState);
    }

    // Render progress dialog (batch operations)
    if (m_progressDialog) {
        m_progressDialog->render();
    }

    // Render global message dialog (singleton)
    MessageDialog::renderGlobal();

    // Render toast notifications
    ToastManager::instance().render(deltaTime);

    // Render import summary dialog
    if (m_importSummaryDialog) {
        m_importSummaryDialog->render();
    }
}

void UIManager::setImportProgress(const ImportProgress* progress) {
    if (m_statusBar) {
        m_statusBar->setImportProgress(progress);
    }
}

void UIManager::showImportSummary(const ImportBatchSummary& summary) {
    if (m_importSummaryDialog) {
        m_importSummaryDialog->open(summary);
    }
}

void UIManager::setImportCancelCallback(std::function<void()> callback) {
    if (m_statusBar) {
        m_statusBar->setOnCancel(std::move(callback));
    }
}

} // namespace dw
