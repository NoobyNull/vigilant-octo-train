// Digital Workshop - UI Manager Implementation
// Extracted from Application: panel ownership, visibility, menus,
// keyboard shortcuts, dialogs, import progress, about/restart popups,
// dock layout.

#include "managers/ui_manager.h"

#include <algorithm>
#include <array>
#include <cstring>

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
#include "ui/dialogs/maintenance_dialog.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/dialogs/progress_dialog.h"
#include "ui/dialogs/tag_image_dialog.h"
#include "ui/dialogs/tagger_shutdown_dialog.h"
#include "ui/panels/cost_panel.h"
#include "ui/panels/cut_optimizer_panel.h"
#include "ui/panels/gcode_panel.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/materials_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/start_page.h"
#include "ui/panels/cnc_status_panel.h"
#include "ui/panels/cnc_jog_panel.h"
#include "ui/panels/cnc_console_panel.h"
#include "ui/panels/cnc_wcs_panel.h"
#include "ui/panels/cnc_tool_panel.h"
#include "ui/panels/cnc_job_panel.h"
#include "ui/panels/cnc_safety_panel.h"
#include "ui/panels/cnc_settings_panel.h"
#include "ui/panels/cnc_macro_panel.h"
#include "ui/panels/direct_carve_panel.h"
#include "ui/panels/tool_browser_panel.h"
#include "ui/panels/viewport_panel.h"
#include "ui/widgets/status_bar.h"
#include "ui/widgets/toast.h"
#include "version.h"

namespace dw {

UIManager::UIManager() = default;

UIManager::~UIManager() {
    shutdown();
}

void UIManager::init(LibraryManager* libraryManager,
                     ProjectManager* projectManager,
                     MaterialManager* materialManager,
                     CostRepository* costRepo,
                     ModelRepository* modelRepo,
                     GCodeRepository* gcodeRepo,
                     CutPlanRepository* cutPlanRepo) {
    // Create panels
    m_viewportPanel = std::make_unique<ViewportPanel>();
    m_libraryPanel = std::make_unique<LibraryPanel>(libraryManager);
    m_propertiesPanel = std::make_unique<PropertiesPanel>();
    m_projectPanel = std::make_unique<ProjectPanel>(
        projectManager, modelRepo, gcodeRepo, cutPlanRepo, costRepo);
    m_gcodePanel = std::make_unique<GCodePanel>();
    m_cutOptimizerPanel = std::make_unique<CutOptimizerPanel>();
    m_materialsPanel = std::make_unique<MaterialsPanel>(materialManager);
    if (costRepo) {
        m_costPanel = std::make_unique<CostPanel>(costRepo);
    }
    m_startPage = std::make_unique<StartPage>();
    m_toolBrowserPanel = std::make_unique<ToolBrowserPanel>();
    m_cncStatusPanel = std::make_unique<CncStatusPanel>();
    m_cncJogPanel = std::make_unique<CncJogPanel>();
    m_cncConsolePanel = std::make_unique<CncConsolePanel>();
    m_cncWcsPanel = std::make_unique<CncWcsPanel>();
    m_cncToolPanel = std::make_unique<CncToolPanel>();
    m_cncJobPanel = std::make_unique<CncJobPanel>();
    m_cncSafetyPanel = std::make_unique<CncSafetyPanel>();
    m_cncSettingsPanel = std::make_unique<CncSettingsPanel>();
    m_cncMacroPanel = std::make_unique<CncMacroPanel>();
    m_directCarvePanel = std::make_unique<DirectCarvePanel>();

    // Create dialogs
    m_fileDialog = std::make_unique<FileDialog>();
    m_lightingDialog = std::make_unique<LightingDialog>();
    m_importSummaryDialog = std::make_unique<ImportSummaryDialog>();
    m_importOptionsDialog = std::make_unique<ImportOptionsDialog>();
    m_progressDialog = std::make_unique<ProgressDialog>();
    m_tagImageDialog = std::make_unique<TagImageDialog>();
    m_maintenanceDialog = std::make_unique<MaintenanceDialog>();
    m_taggerShutdownDialog = std::make_unique<TaggerShutdownDialog>();

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

    // Build panel registry for layout preset system
    buildPanelRegistry();

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
    m_tagImageDialog.reset();
    m_taggerShutdownDialog.reset();
    m_maintenanceDialog.reset();

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
    m_toolBrowserPanel.reset();
    m_cncStatusPanel.reset();
    m_cncJogPanel.reset();
    m_cncConsolePanel.reset();
    m_cncWcsPanel.reset();
    m_cncToolPanel.reset();
    m_cncJobPanel.reset();
    m_cncSafetyPanel.reset();
    m_cncSettingsPanel.reset();
    m_cncMacroPanel.reset();
    m_directCarvePanel.reset();
    m_startPage.reset();
}

void UIManager::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        renderFileMenu();
        renderViewMenu();
        renderEditMenu();
        renderToolsMenu();
        renderHelpMenu();

        // Layout preset selector (between menus and CNC status)
        renderPresetSelector();

        // Right-aligned CNC connection status
        {
            float barWidth = ImGui::GetWindowWidth();
            float cursorX = barWidth;

            if (m_cncConnected && !m_cncSimulating) {
                // Connected to real hardware — show Disconnect button
                const char* label = "Disconnect";
                const auto& style = ImGui::GetStyle();
                float btnWidth = ImGui::CalcTextSize(label).x + style.FramePadding.x * 4.0f;
                cursorX -= btnWidth + style.FramePadding.x;
                ImGui::SetCursorPosX(cursorX);
                if (ImGui::SmallButton(label) && m_onDisconnect) {
                    m_onDisconnect();
                }
            } else if (m_cncSimulating) {
                // Simulator mode — show Connect dropdown if ports available, then "Virtual CNC" label
                if (!m_availablePorts.empty()) {
                    const char* connLabel = "Connect";
                    const auto& style = ImGui::GetStyle();
                    float connWidth = ImGui::CalcTextSize(connLabel).x + style.FramePadding.x * 4.0f;
                    cursorX -= connWidth + style.FramePadding.x;
                    ImGui::SetCursorPosX(cursorX);
                    if (ImGui::SmallButton(connLabel)) {
                        if (m_availablePorts.size() == 1 && m_onConnect) {
                            m_onConnect(m_availablePorts.front());
                        } else {
                            ImGui::OpenPopup("##PortSelect");
                        }
                    }
                    if (ImGui::BeginPopup("##PortSelect")) {
                        for (const auto& port : m_availablePorts) {
                            if (ImGui::MenuItem(port.c_str()) && m_onConnect) {
                                m_onConnect(port);
                            }
                        }
                        ImGui::EndPopup();
                    }
                }

                const char* label = "Virtual CNC";
                float textWidth = ImGui::CalcTextSize(label).x;
                cursorX -= textWidth + ImGui::GetStyle().ItemSpacing.x * 2;
                ImGui::SetCursorPosX(cursorX);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::TextUnformatted(label);
                ImGui::PopStyleColor();
            }
        }

        ImGui::EndMainMenuBar();
    }

    // Save preset popup (must be outside BeginMainMenuBar)
    renderSavePresetPopup();
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
    if (ImGui::MenuItem("Import .dwproj...") && m_onImportProjectArchive) {
        m_onImportProjectArchive();
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
    ImGui::MenuItem("Cut Optimizer", nullptr, &m_showCutOptimizer);
    ImGui::MenuItem("Cost Estimator", nullptr, &m_showCostEstimator);
    ImGui::MenuItem("Materials", nullptr, &m_showMaterials);
    ImGui::MenuItem("Tool Browser", nullptr, &m_showToolBrowser);
    ImGui::Separator();
    if (ImGui::BeginMenu("Sender")) {
        ImGui::MenuItem("G-code Viewer", nullptr, &m_showGCode);
        ImGui::Separator();
        ImGui::MenuItem("Status", nullptr, &m_showCncStatus);
        ImGui::MenuItem("Jog Control", nullptr, &m_showCncJog);
        ImGui::MenuItem("MDI Console", nullptr, &m_showCncConsole);
        ImGui::MenuItem("Work Zero / WCS", nullptr, &m_showCncWcs);
        ImGui::Separator();
        ImGui::MenuItem("Tool & Material", nullptr, &m_showCncTool);
        ImGui::MenuItem("Job Progress", nullptr, &m_showCncJob);
        ImGui::MenuItem("Safety Controls", nullptr, &m_showCncSafety);
        ImGui::Separator();
        ImGui::MenuItem("Firmware Settings", nullptr, &m_showCncSettings);
        ImGui::MenuItem("Macros", nullptr, &m_showCncMacros);
        ImGui::Separator();
        ImGui::MenuItem("Direct Carve", nullptr, &m_showDirectCarve);
        ImGui::Separator();
        if (ImGui::BeginMenu("Live Overlay")) {
            auto& cfg = Config::instance();
            bool dot = cfg.getCncShowToolDot();
            if (ImGui::MenuItem("Tool Position", nullptr, &dot)) cfg.setCncShowToolDot(dot);
            bool env = cfg.getCncShowWorkEnvelope();
            if (ImGui::MenuItem("Work Envelope", nullptr, &env)) cfg.setCncShowWorkEnvelope(env);
            bool dro = cfg.getCncShowDroOverlay();
            if (ImGui::MenuItem("Position Readout", nullptr, &dro)) cfg.setCncShowDroOverlay(dro);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show All")) {
            showCncPanels(true);
        }
        if (ImGui::MenuItem("Hide All")) {
            showCncPanels(false);
        }
        ImGui::EndMenu();
    }
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

void UIManager::renderToolsMenu() {
    if (!ImGui::BeginMenu("Tools")) {
        return;
    }

    if (ImGui::MenuItem("Library Maintenance...") && m_onLibraryMaintenance) {
        m_onLibraryMaintenance();
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

    // Reset auto-context guard each frame
    m_suppressAutoContext = false;

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

    if (m_showToolBrowser && m_toolBrowserPanel) {
        m_toolBrowserPanel->render();
        if (!m_toolBrowserPanel->isOpen()) {
            m_showToolBrowser = false;
            m_toolBrowserPanel->setOpen(true);
        }
    }

    if (m_showCncStatus && m_cncStatusPanel) {
        m_cncStatusPanel->render();
        if (!m_cncStatusPanel->isOpen()) {
            m_showCncStatus = false;
            m_cncStatusPanel->setOpen(true);
        }
    }

    if (m_showCncJog && m_cncJogPanel) {
        m_cncJogPanel->render();
        if (!m_cncJogPanel->isOpen()) {
            m_showCncJog = false;
            m_cncJogPanel->setOpen(true);
        }
    }

    if (m_showCncConsole && m_cncConsolePanel) {
        m_cncConsolePanel->render();
        if (!m_cncConsolePanel->isOpen()) {
            m_showCncConsole = false;
            m_cncConsolePanel->setOpen(true);
        }
    }

    if (m_showCncWcs && m_cncWcsPanel) {
        m_cncWcsPanel->render();
        if (!m_cncWcsPanel->isOpen()) {
            m_showCncWcs = false;
            m_cncWcsPanel->setOpen(true);
        }
    }

    if (m_showCncTool && m_cncToolPanel) {
        m_cncToolPanel->render();
        if (!m_cncToolPanel->isOpen()) {
            m_showCncTool = false;
            m_cncToolPanel->setOpen(true);
        }
    }

    if (m_showCncJob && m_cncJobPanel) {
        m_cncJobPanel->render();
        if (!m_cncJobPanel->isOpen()) {
            m_showCncJob = false;
            m_cncJobPanel->setOpen(true);
        }
    }

    if (m_showCncSafety && m_cncSafetyPanel) {
        m_cncSafetyPanel->render();
        if (!m_cncSafetyPanel->isOpen()) {
            m_showCncSafety = false;
            m_cncSafetyPanel->setOpen(true);
        }
    }

    if (m_showCncSettings && m_cncSettingsPanel) {
        m_cncSettingsPanel->render();
        if (!m_cncSettingsPanel->isOpen()) {
            m_showCncSettings = false;
            m_cncSettingsPanel->setOpen(true);
        }
    }

    if (m_showCncMacros && m_cncMacroPanel) {
        m_cncMacroPanel->render();
        if (!m_cncMacroPanel->isOpen()) {
            m_showCncMacros = false;
            m_cncMacroPanel->setOpen(true);
        }
    }

    if (m_showDirectCarve && m_directCarvePanel) {
        m_directCarvePanel->render();
        if (!m_directCarvePanel->isOpen()) {
            m_showDirectCarve = false;
            m_directCarvePanel->setOpen(true);
        }
    }

    // Auto-context: detect focused panel and trigger preset switch
    if (!m_suppressAutoContext) {
        ImGuiWindow* navWin = GImGui->NavWindow;
        if (navWin) {
            const char* focusedName = navWin->Name;
            for (const auto& entry : m_panelRegistry) {
                if (*entry.showFlag && std::strcmp(focusedName, entry.windowTitle) == 0) {
                    checkAutoContextTrigger(entry.key);
                    break;
                }
            }
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

    if (m_tagImageDialog) {
        m_tagImageDialog->render();
    }

    if (m_taggerShutdownDialog) {
        m_taggerShutdownDialog->render();
    }
    if (m_maintenanceDialog) {
        m_maintenanceDialog->render();
    }
}

void UIManager::renderAboutDialog() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(
            "About Digital Workshop", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
        float okBtnW = ImGui::CalcTextSize("OK").x + ImGui::GetStyle().FramePadding.x * 6;
        if (ImGui::Button("OK", ImVec2(okBtnW, 0))) {
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

    if (ImGui::Begin("Restart Required",
                     &m_showRestartPopup,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("UI Scale has been changed.");
        ImGui::Text("A restart is required to apply this setting.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const auto& style = ImGui::GetStyle();
        float restartBtnW = ImGui::CalcTextSize("Relaunch Now").x + style.FramePadding.x * 4;
        float laterBtnW = ImGui::CalcTextSize("Later").x + style.FramePadding.x * 4;
        if (ImGui::Button("Relaunch Now", ImVec2(restartBtnW, 0))) {
            m_showRestartPopup = false;
            if (onRelaunch) {
                onRelaunch();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Later", ImVec2(laterBtnW, 0))) {
            m_showRestartPopup = false;
        }
    }
    ImGui::End();
}

void UIManager::handleKeyboardShortcuts() {
    auto& io = ImGui::GetIO();

    // Keyboard smash panic stop — runs BEFORE WantTextInput check
    // so it works even when a text field (e.g. MDI console) is focused.
    // Only active during real hardware streaming.
    if (m_cncStreaming && m_cncConnected && !m_cncSimulating) {
        // Count any key-down event this frame
        for (int k = static_cast<int>(ImGuiKey_NamedKey_BEGIN);
             k < static_cast<int>(ImGuiKey_NamedKey_END); ++k) {
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) {
                float now = static_cast<float>(ImGui::GetTime());
                m_panicKeyTimes[m_panicKeyHead] = now;
                m_panicKeyHead = (m_panicKeyHead + 1) % PANIC_KEY_COUNT;
                // Check if oldest entry in ring is within window
                float oldest = m_panicKeyTimes[m_panicKeyHead];
                if (oldest > 0.0f && (now - oldest) <= PANIC_WINDOW_SEC) {
                    // Panic detected — fire callback and reset
                    if (m_onPanicStop) m_onPanicStop();
                    std::fill(std::begin(m_panicKeyTimes), std::end(m_panicKeyTimes), 0.0f);
                    return;
                }
                break; // only record one key per frame
            }
        }
    }

    // Only handle shortcuts when not typing in a text field
    if (io.WantTextInput) {
        return;
    }

    // CNC keyboard jog (arrow keys, Page Up/Down) — no modifier required
    if (m_showCncJog && m_cncJogPanel) {
        m_cncJogPanel->handleKeyboardJog();
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

    // Workspace mode switching
    if (ImGui::IsKeyPressed(ImGuiKey_1)) {
        setWorkspaceMode(WorkspaceMode::Model);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_2)) {
        setWorkspaceMode(WorkspaceMode::CNC);
    }
}

void UIManager::setupDefaultDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    // Split: left sidebar (20%) | center+right
    ImGuiID dockLeft = 0;
    ImGuiID dockCenterRight = 0;
    ImGui::DockBuilderSplitNode(
        dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenterRight);

    // Split left sidebar: library (top 60%) | project (bottom 40%)
    ImGuiID dockLeftTop = 0;
    ImGuiID dockLeftBottom = 0;
    ImGui::DockBuilderSplitNode(
        dockLeft, ImGuiDir_Down, 0.40f, &dockLeftBottom, &dockLeftTop);

    // Split center+right: center | right sidebar (20%) for properties
    ImGuiID dockCenter = 0;
    ImGuiID dockRight = 0;
    ImGui::DockBuilderSplitNode(
        dockCenterRight, ImGuiDir_Right, 0.20f, &dockRight, &dockCenter);

    // --- Core visible panels ---
    ImGui::DockBuilderDockWindow("Library", dockLeftTop);
    ImGui::DockBuilderDockWindow("Project", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    ImGui::DockBuilderDockWindow("Properties", dockRight);

    // --- Hidden panels — docked as tabs in existing areas (no empty splits) ---
    // Center tabs (behind Viewport)
    ImGui::DockBuilderDockWindow("Start Page", dockCenter);
    ImGui::DockBuilderDockWindow("G-code", dockCenter);
    ImGui::DockBuilderDockWindow("Cut Optimizer", dockCenter);

    // Left-bottom tabs (behind Project)
    ImGui::DockBuilderDockWindow("Cost Estimator", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Materials", dockLeftBottom);

    // Right tabs (behind Properties)
    ImGui::DockBuilderDockWindow("Tool & Material", dockRight);
    ImGui::DockBuilderDockWindow("Tool Browser", dockRight);

    // CNC panels — docked as center tabs (appear when CNC mode activated)
    ImGui::DockBuilderDockWindow("CNC Status", dockCenter);
    ImGui::DockBuilderDockWindow("Jog Control", dockCenter);
    ImGui::DockBuilderDockWindow("MDI Console", dockCenter);
    ImGui::DockBuilderDockWindow("WCS", dockCenter);
    ImGui::DockBuilderDockWindow("Safety", dockCenter);
    ImGui::DockBuilderDockWindow("Firmware", dockCenter);
    ImGui::DockBuilderDockWindow("Macros", dockCenter);
    ImGui::DockBuilderDockWindow("Job Progress", dockCenter);
    ImGui::DockBuilderDockWindow("Direct Carve", dockCenter);

    ImGui::DockBuilderFinish(dockspaceId);
}

void UIManager::restoreVisibilityFromConfig() {
    auto& cfg = Config::instance();

    // Restore individual panel visibility (legacy + overrides)
    m_showViewport = cfg.getShowViewport();
    m_showLibrary = cfg.getShowLibrary();
    m_showProperties = cfg.getShowProperties();
    m_showProject = cfg.getShowProject();
    m_showMaterials = cfg.getShowMaterials();
    m_showGCode = cfg.getShowGCode();
    m_showCutOptimizer = cfg.getShowCutOptimizer();
    m_showCostEstimator = cfg.getShowCostEstimator();
    m_showToolBrowser = cfg.getShowToolBrowser();
    m_showStartPage = cfg.getShowStartPage();

    // CNC panels
    m_showCncStatus = cfg.getShowCncStatus();
    m_showCncJog = cfg.getShowCncJog();
    m_showCncConsole = cfg.getShowCncConsole();
    m_showCncWcs = cfg.getShowCncWcs();
    m_showCncTool = cfg.getShowCncTool();
    m_showCncJob = cfg.getShowCncJob();
    m_showCncSafety = cfg.getShowCncSafety();
    m_showCncSettings = cfg.getShowCncSettings();
    m_showCncMacros = cfg.getShowCncMacros();
    m_showDirectCarve = cfg.getShowDirectCarve();

    // Restore active layout preset index
    m_activePresetIndex = cfg.getActiveLayoutPresetIndex();
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
    cfg.setShowToolBrowser(m_showToolBrowser);
    cfg.setShowStartPage(m_showStartPage);

    // CNC panels
    cfg.setShowCncStatus(m_showCncStatus);
    cfg.setShowCncJog(m_showCncJog);
    cfg.setShowCncConsole(m_showCncConsole);
    cfg.setShowCncWcs(m_showCncWcs);
    cfg.setShowCncTool(m_showCncTool);
    cfg.setShowCncJob(m_showCncJob);
    cfg.setShowCncSafety(m_showCncSafety);
    cfg.setShowCncSettings(m_showCncSettings);
    cfg.setShowCncMacros(m_showCncMacros);
    cfg.setShowDirectCarve(m_showDirectCarve);

    cfg.setActiveLayoutPresetIndex(m_activePresetIndex);
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

void UIManager::showTaggerShutdownDialog(const TaggerProgress* progress) {
    if (m_taggerShutdownDialog)
        m_taggerShutdownDialog->open(progress);
}

void UIManager::showCncPanels(bool show) {
    m_showCncStatus = show;
    m_showCncJog = show;
    m_showCncConsole = show;
    m_showCncWcs = show;
    m_showCncTool = show;
    m_showCncJob = show;
    m_showCncSafety = show;
    m_showCncSettings = show;
    m_showCncMacros = show;
    m_showGCode = show;
}

void UIManager::setWorkspaceMode(WorkspaceMode mode) {
    m_workspaceMode = mode;
    // Delegate to layout presets: index 0 = Modeling, index 1 = CNC Sender
    applyLayoutPreset(mode == WorkspaceMode::CNC ? 1 : 0);
}

void UIManager::buildPanelRegistry() {
    // key, showFlag, menuLabel, windowTitle (ImGui::Begin title)
    m_panelRegistry = {
        {"viewport",       &m_showViewport,       "Viewport",          "Viewport"},
        {"library",        &m_showLibrary,        "Library",           "Library"},
        {"properties",     &m_showProperties,     "Properties",        "Properties"},
        {"project",        &m_showProject,        "Project",           "Project"},
        {"start_page",     &m_showStartPage,      "Start Page",        "Start Page"},
        {"gcode",          &m_showGCode,          "G-code Viewer",     "G-code"},
        {"cut_optimizer",  &m_showCutOptimizer,   "Cut Optimizer",     "Cut Optimizer"},
        {"cost_estimator", &m_showCostEstimator,  "Cost Estimator",    "Cost Estimator"},
        {"materials",      &m_showMaterials,      "Materials",         "Materials"},
        {"tool_browser",   &m_showToolBrowser,    "Tool Browser",      "Tool Browser"},
        {"cnc_status",     &m_showCncStatus,      "Status",            "CNC Status"},
        {"cnc_jog",        &m_showCncJog,         "Jog Control",       "Jog Control"},
        {"cnc_console",    &m_showCncConsole,     "MDI Console",       "MDI Console"},
        {"cnc_wcs",        &m_showCncWcs,         "Work Zero / WCS",   "WCS"},
        {"cnc_tool",       &m_showCncTool,        "Tool & Material",   "Tool & Material"},
        {"cnc_job",        &m_showCncJob,         "Job Progress",      "Job Progress"},
        {"cnc_safety",     &m_showCncSafety,      "Safety Controls",   "Safety"},
        {"cnc_settings",   &m_showCncSettings,    "Firmware Settings",  "Firmware"},
        {"cnc_macros",     &m_showCncMacros,      "Macros",            "Macros"},
        {"direct_carve",   &m_showDirectCarve,    "Direct Carve",      "Direct Carve"},
    };
}

void UIManager::applyLayoutPreset(int presetIndex) {
    auto& cfg = Config::instance();
    const auto& presets = cfg.getLayoutPresets();
    if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size()))
        return;

    const auto& preset = presets[static_cast<size_t>(presetIndex)];

    for (const auto& entry : m_panelRegistry) {
        auto it = preset.visibility.find(entry.key);
        if (it != preset.visibility.end()) {
            *entry.showFlag = it->second;
        }
    }

    m_activePresetIndex = presetIndex;
    cfg.setActiveLayoutPresetIndex(presetIndex);
    m_suppressAutoContext = true;

    // Keep workspace mode in sync with built-in presets
    if (presetIndex == 0) m_workspaceMode = WorkspaceMode::Model;
    else if (presetIndex == 1) m_workspaceMode = WorkspaceMode::CNC;
}

LayoutPreset UIManager::captureCurrentLayout(const std::string& name) const {
    LayoutPreset preset;
    preset.name = name;

    for (const auto& entry : m_panelRegistry) {
        preset.visibility[entry.key] = *entry.showFlag;
    }
    return preset;
}

void UIManager::saveCurrentAsPreset(const std::string& name) {
    auto& cfg = Config::instance();
    auto& presets = const_cast<std::vector<LayoutPreset>&>(cfg.getLayoutPresets());

    // Check for existing custom preset with same name — overwrite it
    for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
        if (!presets[static_cast<size_t>(i)].builtIn && presets[static_cast<size_t>(i)].name == name) {
            auto captured = captureCurrentLayout(name);
            cfg.updateLayoutPreset(i, captured);
            m_activePresetIndex = i;
            cfg.setActiveLayoutPresetIndex(i);
            cfg.save();
            return;
        }
    }

    // New preset
    auto captured = captureCurrentLayout(name);
    cfg.addLayoutPreset(captured);
    m_activePresetIndex = static_cast<int>(cfg.getLayoutPresets().size()) - 1;
    cfg.setActiveLayoutPresetIndex(m_activePresetIndex);
    cfg.save();
}

void UIManager::deletePreset(int index) {
    auto& cfg = Config::instance();
    cfg.removeLayoutPreset(index);

    // Adjust active index
    if (m_activePresetIndex >= static_cast<int>(cfg.getLayoutPresets().size())) {
        m_activePresetIndex = static_cast<int>(cfg.getLayoutPresets().size()) - 1;
    }
    cfg.setActiveLayoutPresetIndex(m_activePresetIndex);
    cfg.save();
}

void UIManager::checkAutoContextTrigger(const std::string& focusedPanelKey) {
    if (m_suppressAutoContext) return;

    auto& cfg = Config::instance();
    const auto& presets = cfg.getLayoutPresets();

    for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
        if (i == m_activePresetIndex) continue;
        if (presets[static_cast<size_t>(i)].autoTriggerPanelKey == focusedPanelKey) {
            applyLayoutPreset(i);
            return;
        }
    }
}

void UIManager::renderPresetSelector() {
    auto& cfg = Config::instance();
    const auto& presets = cfg.getLayoutPresets();

    const char* activeLabel = "Custom";
    if (m_activePresetIndex >= 0 &&
        m_activePresetIndex < static_cast<int>(presets.size())) {
        activeLabel = presets[static_cast<size_t>(m_activePresetIndex)].name.c_str();
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    const auto& style = ImGui::GetStyle();
    float comboWidth = ImGui::CalcTextSize(activeLabel).x + style.FramePadding.x * 4.0f +
                       ImGui::GetFrameHeight();
    ImGui::SetNextItemWidth(comboWidth);

    if (ImGui::BeginCombo("##LayoutPreset", activeLabel, ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
            bool selected = (i == m_activePresetIndex);
            if (ImGui::Selectable(presets[static_cast<size_t>(i)].name.c_str(), selected)) {
                applyLayoutPreset(i);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }

        ImGui::Separator();
        if (ImGui::Selectable("Save Current Layout...")) {
            m_showSavePresetPopup = true;
        }
        if (m_activePresetIndex >= 0 &&
            m_activePresetIndex < static_cast<int>(presets.size()) &&
            !presets[static_cast<size_t>(m_activePresetIndex)].builtIn) {
            if (ImGui::Selectable("Delete Current Preset")) {
                deletePreset(m_activePresetIndex);
            }
        }
        ImGui::EndCombo();
    }
}

void UIManager::renderSavePresetPopup() {
    if (m_showSavePresetPopup) {
        ImGui::OpenPopup("Save Layout Preset");
        m_showSavePresetPopup = false;
    }
    if (ImGui::BeginPopupModal("Save Layout Preset", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Preset name:");
        float inputWidth = ImGui::CalcTextSize("M").x * 30.0f;
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##PresetName", m_presetNameBuf, sizeof(m_presetNameBuf));
        ImGui::Spacing();

        bool nameValid = m_presetNameBuf[0] != '\0';
        if (!nameValid) ImGui::BeginDisabled();
        if (ImGui::Button("Save")) {
            saveCurrentAsPreset(m_presetNameBuf);
            m_presetNameBuf[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        if (!nameValid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_presetNameBuf[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace dw
