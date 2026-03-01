// Direct Carve wizard panel -- step-by-step guided workflow for streaming
// 2.5D toolpaths directly from STL models. Each step validates before allowing
// progression. Machine readiness is verified before any carving begins.

#include "ui/panels/direct_carve_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

#include <imgui.h>

#include "core/carve/analysis_overlay.h"
#include "core/carve/carve_job.h"
#include "core/carve/gcode_export.h"
#include "core/carve/tool_recommender.h"
#include "core/project/project.h"
#include "core/project/project_directory.h"
#include "core/cnc/cnc_controller.h"
#include "core/config/config.h"
#include "core/database/tool_database.h"
#include "core/database/toolbox_repository.h"
#include "core/gcode/machine_profile.h"
#include "core/materials/material_manager.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/icons.h"
#include "ui/panels/gcode_panel.h"
#include "ui/theme.h"
#include "ui/widgets/toast.h"

namespace dw {

namespace {

// Status indicator colors
constexpr ImVec4 kGreen{0.3f, 0.8f, 0.3f, 1.0f};
constexpr ImVec4 kRed{1.0f, 0.3f, 0.3f, 1.0f};
constexpr ImVec4 kYellow{1.0f, 0.8f, 0.2f, 1.0f};
constexpr ImVec4 kDimmed{0.5f, 0.5f, 0.5f, 1.0f};
constexpr ImVec4 kBright{0.4f, 0.7f, 1.0f, 1.0f};

// ProgressBar with centered overlay text (ImGui default left-aligns within filled area)
void CenteredProgressBar(float fraction, const ImVec2& size, const char* overlay) {
    ImGui::ProgressBar(fraction, size, "");
    if (overlay && overlay[0]) {
        ImVec2 textSize = ImGui::CalcTextSize(overlay);
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        float cx = barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f;
        float cy = barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(ImVec2(cx, cy), IM_COL32(255, 255, 255, 255), overlay);
    }
}

// Toolpath overlay colors
constexpr ImU32 kFinishColor = IM_COL32(80, 120, 255, 200);   // blue
constexpr ImU32 kClearColor = IM_COL32(255, 80, 80, 200);     // red
constexpr ImU32 kRapidColor = IM_COL32(80, 220, 80, 150);     // green

void statusBullet(bool ok, const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, ok ? kGreen : kRed);
    ImGui::BulletText("%s %s", ok ? "OK" : "--", label);
    ImGui::PopStyleColor();
}

} // anonymous namespace

DirectCarvePanel::DirectCarvePanel() : Panel("Direct Carve") {}

DirectCarvePanel::~DirectCarvePanel() = default;

void DirectCarvePanel::setCncController(CncController* cnc) { m_cnc = cnc; }
void DirectCarvePanel::setToolDatabase(ToolDatabase* db) { m_toolDb = db; }
void DirectCarvePanel::setToolboxRepository(ToolboxRepository* repo) { m_toolboxRepo = repo; }
void DirectCarvePanel::setCarveJob(carve::CarveJob* job) { m_carveJob = job; }
void DirectCarvePanel::setFileDialog(FileDialog* dlg) { m_fileDialog = dlg; }
void DirectCarvePanel::setGCodePanel(GCodePanel* gcp) { m_gcodePanel = gcp; }
void DirectCarvePanel::setMaterialManager(MaterialManager* mgr) { m_materialMgr = mgr; }
void DirectCarvePanel::setProjectManager(ProjectManager* pm) { m_projectManager = pm; }
void DirectCarvePanel::setOpenToolBrowserCallback(std::function<void()> cb) { m_openToolBrowser = std::move(cb); }

void DirectCarvePanel::onConnectionChanged(bool connected) { m_cncConnected = connected; }
void DirectCarvePanel::onStatusUpdate(const MachineStatus& status) { m_machineStatus = status; }

void DirectCarvePanel::onModelLoaded(const std::vector<Vertex>& vertices,
                                      const std::vector<u32>& indices,
                                      const Vec3& boundsMin,
                                      const Vec3& boundsMax,
                                      const std::string& modelName,
                                      const Path& modelSourcePath) {
    m_vertices = vertices;
    m_indices = indices;
    m_modelLoaded = true;
    m_modelBoundsMin = boundsMin;
    m_modelBoundsMax = boundsMax;
    m_fitter.setModelBounds(boundsMin, boundsMax);
    if (!modelName.empty()) m_modelName = modelName;
    if (!modelSourcePath.empty()) m_modelSourcePath = modelSourcePath;

    // Reset heightmap cache state for new model
    m_hmInitAttempted = false;
    m_hmFileMissing = false;
    m_heightmapSaved = false;
    m_hmRegenConfirm = false;
    m_hmMissingPath.clear();
}

std::string DirectCarvePanel::formatTime(f32 seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%dm %ds", mins, secs);
    return buf;
}

const char* DirectCarvePanel::stepLabel(Step step) {
    switch (step) {
    case Step::MachineCheck: return "Machine";
    case Step::ModelFit:     return "Model";
    case Step::ToolSelect:   return "Tool";
    case Step::MaterialSetup:return "Material";
    case Step::Preview:      return "Preview";
    case Step::OutlineTest:  return "Outline";
    case Step::ZeroConfirm:  return "Zero";
    case Step::Commit:       return "Confirm";
    case Step::Running:      return "Running";
    }
    return "???";
}

void DirectCarvePanel::render() {
    if (!m_open) return;
    applyMinSize(40.0f, 25.0f);
    if (!ImGui::Begin(m_title.c_str(), &m_open)) { ImGui::End(); return; }

    renderStepIndicator();
    ImGui::Separator();
    ImGui::Spacing();

    switch (m_currentStep) {
    case Step::MachineCheck:  renderMachineCheck();  break;
    case Step::ModelFit:      renderModelFit();      break;
    case Step::ToolSelect:    renderToolSelect();    break;
    case Step::MaterialSetup: renderMaterialSetup(); break;
    case Step::Preview:       renderPreview();       break;
    case Step::OutlineTest:   renderOutlineTest();   break;
    case Step::ZeroConfirm:   renderZeroConfirm();   break;
    case Step::Commit:        renderCommit();        break;
    case Step::Running:       renderRunning();       break;
    }

    ImGui::Spacing();
    ImGui::Separator();
    renderNavButtons();
    ImGui::End();
}

void DirectCarvePanel::renderStepIndicator() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float fontSize = ImGui::GetFontSize();
    float circleR = fontSize * 0.35f;
    int curIdx = static_cast<int>(m_currentStep);

    // Evenly space steps across the full available width
    float availW = ImGui::GetContentRegionAvail().x;
    float stepSpacing = (STEP_COUNT > 1) ? availW / static_cast<float>(STEP_COUNT) : availW;
    float totalH = circleR * 2.0f + fontSize + 4.0f; // circle + gap + label

    for (int i = 0; i < STEP_COUNT; ++i) {
        const char* label = stepLabel(static_cast<Step>(i));
        float labelW = ImGui::CalcTextSize(label).x;
        float cx = cursor.x + stepSpacing * (static_cast<float>(i) + 0.5f);
        float cy = cursor.y + circleR;
        ImVec4 color = (i < curIdx) ? kGreen : (i == curIdx) ? kBright : kDimmed;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

        // Clickable invisible button over step region
        ImVec2 hitMin{cx - stepSpacing * 0.5f, cursor.y};
        ImVec2 hitMax{cx + stepSpacing * 0.5f, cursor.y + totalH};
        ImGui::SetCursorScreenPos(hitMin);
        char btnId[32];
        std::snprintf(btnId, sizeof(btnId), "##step%d", i);
        if (ImGui::InvisibleButton(btnId, ImVec2(hitMax.x - hitMin.x, hitMax.y - hitMin.y))) {
            // Allow clicking to any completed step or the current step
            if (i <= curIdx)
                m_currentStep = static_cast<Step>(i);
        }
        bool hovered = ImGui::IsItemHovered();
        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // Circle
        if (i <= curIdx)
            dl->AddCircleFilled(ImVec2(cx, cy), circleR, col);
        else
            dl->AddCircle(ImVec2(cx, cy), circleR, col, 0, 1.5f);

        // Hover highlight ring
        if (hovered && i <= curIdx)
            dl->AddCircle(ImVec2(cx, cy), circleR + 2.0f, col, 0, 1.5f);

        // Label centered below circle
        dl->AddText(ImVec2(cx - labelW * 0.5f, cy + circleR + 2.0f), col, label);

        // Connecting line to next step
        if (i < STEP_COUNT - 1) {
            float nextCx = cursor.x + stepSpacing * (static_cast<float>(i) + 1.5f);
            float lx0 = cx + circleR + 2.0f;
            float lx1 = nextCx - circleR - 2.0f;
            ImU32 lc = (i < curIdx) ?
                ImGui::ColorConvertFloat4ToU32(kGreen) :
                ImGui::ColorConvertFloat4ToU32(kDimmed);
            dl->AddLine(ImVec2(lx0, cy), ImVec2(lx1, cy), lc, 1.5f);
        }
    }
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + totalH + 2.0f));
    ImGui::Dummy(ImVec2(0, 0)); // advance layout cursor
}

void DirectCarvePanel::renderNavButtons() {
    float fontSize = ImGui::GetFontSize();
    float bw = fontSize * 6.0f;
    bool isFirst = (m_currentStep == Step::MachineCheck);
    bool isRunning = (m_currentStep == Step::Running);
    bool isCommit = (m_currentStep == Step::Commit);

    if (isFirst || isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Back", ImVec2(bw, 0))) retreatStep();
    if (isFirst || isRunning) ImGui::EndDisabled();

    ImGui::SameLine();
    bool canGo = canAdvance();
    if (!canGo) ImGui::BeginDisabled();
    if (ImGui::Button(isCommit ? "Start Carving" : "Next", ImVec2(bw, 0))) advanceStep();
    if (!canGo) ImGui::EndDisabled();

    ImGui::SameLine();
    if (isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Cancel", ImVec2(bw, 0))) {
        m_currentStep = Step::MachineCheck;
        m_safeZConfirmed = false;
        m_finishingToolSelected = false;
        m_materialSelected = false;
        m_toolpathGenerated = false;
        m_outlineCompleted = false;
        m_outlineSkipped = false;
        m_outlineRunning = false;
        m_zeroConfirmed = false;
        m_commitConfirmed = false;
    }
    if (isRunning) ImGui::EndDisabled();
}

bool DirectCarvePanel::canAdvance() const {
    switch (m_currentStep) {
    case Step::MachineCheck:  return validateMachineReady();
    case Step::ModelFit:      return m_modelLoaded;
    case Step::ToolSelect:    return m_finishingToolSelected;
    case Step::MaterialSetup: return m_materialSelected;
    case Step::Preview:       return m_toolpathGenerated;
    case Step::OutlineTest:   return m_outlineCompleted || m_outlineSkipped;
    case Step::ZeroConfirm:   return m_zeroConfirmed;
    case Step::Commit:        return m_commitConfirmed;
    case Step::Running:       return false;
    }
    return false;
}

void DirectCarvePanel::advanceStep() {
    int idx = static_cast<int>(m_currentStep);
    if (idx < STEP_COUNT - 1) m_currentStep = static_cast<Step>(idx + 1);
}

void DirectCarvePanel::retreatStep() {
    int idx = static_cast<int>(m_currentStep);
    if (idx > 0) m_currentStep = static_cast<Step>(idx - 1);
}

bool DirectCarvePanel::validateMachineReady() const {
    if (!m_cncConnected) return false;
    if (m_machineStatus.state == MachineState::Alarm) return false;
    if (m_machineStatus.state == MachineState::Unknown) return false;
    if (!m_safeZConfirmed) return false;
    return true;
}

void DirectCarvePanel::renderMachineCheck() {
    ImGui::TextUnformatted("Machine Readiness Check");
    ImGui::Spacing();

    bool connected = m_cncConnected;
    bool homed = (m_machineStatus.state == MachineState::Idle);
    bool notAlarm = (m_machineStatus.state != MachineState::Alarm &&
                     m_machineStatus.state != MachineState::Unknown);
    auto& cfg = Config::instance();
    const auto& profile = cfg.getActiveMachineProfile();
    bool profileOk = (profile.maxTravelX > 0.0f && profile.maxTravelY > 0.0f &&
                      profile.maxTravelZ > 0.0f);

    statusBullet(connected, "CNC connected");
    statusBullet(homed && notAlarm, "Machine homed (idle, no alarm)");
    statusBullet(profileOk, "Machine profile configured (travel limits set)");
    statusBullet(m_safeZConfirmed, "Safe Z verified");
    ImGui::Spacing();

    if (connected && homed) {
        float fs = ImGui::GetFontSize();
        float bw = fs * 10.0f;
        if (ImGui::Button("Test Safe Z", ImVec2(bw, 0))) {
            if (m_cnc) {
                char cmd[64];
                std::snprintf(cmd, sizeof(cmd), "G0 Z%.3f",
                              static_cast<double>(m_toolpathConfig.safeZMm));
                m_cnc->sendCommand(cmd);
                m_safeZConfirmed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip (confirm safe Z)", ImVec2(bw * 1.2f, 0)))
            m_safeZConfirmed = true;
    }

    if (m_machineStatus.state == MachineState::Alarm) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, kRed);
        ImGui::TextWrapped("Machine is in ALARM state. Clear the alarm and home.");
        ImGui::PopStyleColor();
    }
}

// --- renderModelFit (18-02): Stock dims, scale/depth/position, live fit, heightmap gen ---

void DirectCarvePanel::renderModelFit() {
    ImGui::TextUnformatted("Model Fitting");
    ImGui::Spacing();

    if (!m_modelLoaded) {
        ImGui::TextColored(kYellow, "No model loaded. Load an STL model first.");
        return;
    }

    // Stock dimensions
    float iw = ImGui::GetFontSize() * 8.0f;
    ImGui::Text("Stock Dimensions:");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Width (X) mm", &m_stock.width, 1.0f, 10.0f, "%.1f");
    m_stock.width = std::clamp(m_stock.width, 1.0f, 2000.0f);
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Height (Y) mm", &m_stock.height, 1.0f, 10.0f, "%.1f");
    m_stock.height = std::clamp(m_stock.height, 1.0f, 2000.0f);
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Thickness (Z) mm", &m_stock.thickness, 0.5f, 5.0f, "%.1f");
    m_stock.thickness = std::clamp(m_stock.thickness, 0.5f, 200.0f);

    float fs = ImGui::GetFontSize();
    float bw = fs * 12.0f;
    if (ImGui::Button("From Machine Profile", ImVec2(bw, 0))) {
        const auto& prof = Config::instance().getActiveMachineProfile();
        m_stock.width = prof.maxTravelX;
        m_stock.height = prof.maxTravelY;
        m_stock.thickness = prof.maxTravelZ;
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Scale slider with dynamic max derived from stock/model ratio
    f32 extX = std::max(1.0f, m_modelBoundsMax.x - m_modelBoundsMin.x);
    f32 maxScale = std::max(1.0f, m_stock.width / extX) * 2.0f;
    ImGui::SetNextItemWidth(iw);
    ImGui::SliderFloat("Scale", &m_fitParams.scale, 0.1f, maxScale, "%.3f");
    ImGui::SameLine();
    if (ImGui::Button("Auto Fit")) {
        m_fitter.setStock(m_stock);
        m_fitParams.scale = m_fitter.autoScale();
    }

    f32 depthMax = std::max(m_stock.thickness, 1.0f);
    ImGui::SetNextItemWidth(iw);
    ImGui::DragFloat("Depth (Z) mm", &m_fitParams.depthMm, 0.1f, 0.0f, depthMax, "%.1f");
    ImGui::SameLine();
    if (ImGui::Button("Full Depth")) {
        m_fitter.setStock(m_stock);
        m_fitParams.depthMm = m_fitter.autoDepth() * m_fitParams.scale;
    }

    ImGui::SetNextItemWidth(iw);
    ImGui::DragFloat2("Position (XY)", &m_fitParams.offsetX, 0.5f);

    // Live fit result
    m_fitter.setStock(m_stock);
    const auto& mp = Config::instance().getActiveMachineProfile();
    m_fitter.setMachineTravel(mp.maxTravelX, mp.maxTravelY, mp.maxTravelZ);
    carve::FitResult result = m_fitter.fit(m_fitParams);

    ImGui::Spacing();
    Vec3 dim = result.modelMax - result.modelMin;
    ImGui::Text("After transform: %.1f x %.1f x %.1f mm",
                static_cast<double>(dim.x), static_cast<double>(dim.y),
                static_cast<double>(dim.z));
    ImGui::TextColored(result.fitsStock ? kGreen : kRed,
                       result.fitsStock ? "Fits stock" : "Exceeds stock");
    ImGui::SameLine();
    ImGui::TextColored(result.fitsMachine ? kGreen : kRed,
                       result.fitsMachine ? "Fits machine" : "Exceeds machine");
    if (!result.warning.empty())
        ImGui::TextColored(kYellow, "%s", result.warning.c_str());

}

void DirectCarvePanel::renderToolSelect() {
    ImGui::TextUnformatted("Tool Selection");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a finishing tool for the carve operation. "
                       "A ball-nose end mill is recommended for smooth 3D relief surfaces. "
                       "Smaller diameters capture finer detail but take longer.");
    ImGui::Spacing();

    // Load tools from database on first visit
    if (!m_toolLibraryLoaded && m_toolDb) {
        m_toolLibraryLoaded = true;
        m_toolboxTools.clear();
        m_allTools.clear();

        auto isCarveType = [](VtdbToolType t) {
            return t == VtdbToolType::BallNose || t == VtdbToolType::TaperedBallNose ||
                   t == VtdbToolType::VBit || t == VtdbToolType::EndMill;
        };

        // Load toolbox subset
        if (m_toolboxRepo) {
            auto ids = m_toolboxRepo->getAllGeometryIds();
            for (const auto& id : ids) {
                auto geom = m_toolDb->findGeometryById(id);
                if (geom && isCarveType(geom->tool_type))
                    m_toolboxTools.push_back(std::move(*geom));
            }
        }

        // Load full library
        auto allGeoms = m_toolDb->findAllGeometries();
        for (auto& g : allGeoms) {
            if (isCarveType(g.tool_type))
                m_allTools.push_back(std::move(g));
        }

        // Default to toolbox if it has tools, otherwise show all
        m_showAllTools = m_toolboxTools.empty();
        m_libraryTools = m_showAllTools ? m_allTools : m_toolboxTools;
    }

    // Source tabs: Library vs Manual
    bool hasLibrary = !m_libraryTools.empty();

    if (hasLibrary || !m_allTools.empty()) {
        if (ImGui::BeginTabBar("##toolSource")) {
            if (ImGui::BeginTabItem("Tool Library")) {
                m_useManualTool = false;
                ImGui::Spacing();

                // Filter toggle + Edit Toolbox button
                bool hasToolboxTools = !m_toolboxTools.empty();
                if (hasToolboxTools) {
                    bool showAll = m_showAllTools;
                    if (ImGui::RadioButton("My Toolbox", !showAll)) {
                        m_showAllTools = false;
                        m_libraryTools = m_toolboxTools;
                        m_selectedLibToolIdx = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("All Tools", showAll)) {
                        m_showAllTools = true;
                        m_libraryTools = m_allTools;
                        m_selectedLibToolIdx = -1;
                    }
                } else {
                    ImGui::TextDisabled("Showing all tools (My Toolbox is empty)");
                }

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Edit Toolbox").x
                                - ImGui::GetStyle().FramePadding.x * 2);
                if (ImGui::SmallButton("Edit Toolbox")) {
                    if (m_openToolBrowser) m_openToolBrowser();
                    ImGui::SetWindowFocus("Tool Browser");
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Open Tool Browser to manage My Toolbox");

                ImGui::Spacing();
                renderToolLibraryPicker();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Manual Entry")) {
                m_useManualTool = true;
                ImGui::Spacing();
                renderManualToolEntry();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    } else {
        if (!m_toolDb) {
            ImGui::TextColored(kYellow, "No tool database connected.");
        } else {
            ImGui::TextDisabled("Tool library is empty. Import tools via the Tool Browser panel.");
        }
        ImGui::Spacing();
        m_useManualTool = true;
        renderManualToolEntry();
    }

    // Show summary of selected tool
    if (m_finishingToolSelected) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        const char* typeStr = "Tool";
        switch (m_finishTool.tool_type) {
        case VtdbToolType::BallNose:        typeStr = "Ball Nose"; break;
        case VtdbToolType::TaperedBallNose: typeStr = "Tapered Ball Nose"; break;
        case VtdbToolType::VBit:            typeStr = "V-Bit"; break;
        case VtdbToolType::EndMill:         typeStr = "End Mill"; break;
        default: break;
        }
        ImGui::TextColored(kGreen, "%s Finishing: %s  %.3gmm  %d flute%s",
                           Icons::Check, typeStr, m_finishTool.diameter,
                           m_finishTool.num_flutes,
                           m_finishTool.num_flutes != 1 ? "s" : "");

        // Run recommender when heightmap analysis is available
        if (m_carveJob && m_carveJob->state() == carve::CarveJobState::Ready
            && m_toolDb && !m_recommendationRun) {
            // Analyze heightmap for islands using selected finishing tool angle
            f32 toolAngle = static_cast<f32>(m_finishTool.included_angle);
            if (toolAngle <= 0.0f) toolAngle = 90.0f;
            m_carveJob->analyzeHeightmap(toolAngle);

            // Populate recommender with toolbox tools
            carve::ToolRecommender recommender;
            VtdbCuttingData emptyData{};
            for (const auto& g : m_libraryTools) {
                auto entities = m_toolDb->findEntitiesForGeometry(g.id);
                if (!entities.empty()) {
                    auto cd = m_toolDb->findCuttingDataById(entities[0].tool_cutting_data_id);
                    recommender.addCandidate(g, cd.value_or(emptyData));
                } else {
                    recommender.addCandidate(g, emptyData);
                }
            }

            const auto& hm = m_carveJob->heightmap();
            carve::RecommendationInput input;
            input.curvature = m_carveJob->curvatureResult();
            input.islands = m_carveJob->islandResult();
            input.modelDepthMm = hm.maxZ() - hm.minZ();
            input.stockThicknessMm = m_stock.thickness;
            m_recommendation = recommender.recommend(input);
            m_recommendationRun = true;

            // Auto-select top clearing tool
            if (m_recommendation.needsClearing && !m_recommendation.clearing.empty()) {
                m_clearTool = m_recommendation.clearing[0].geometry;
                m_clearToolSelected = true;
                m_selectedClearIdx = 0;
            }
        }

        // Show clearing tool section when recommendation has been run
        if (m_recommendationRun) {
            ImGui::Spacing();

            if (m_recommendation.needsClearing) {
                const auto& islands = m_carveJob->islandResult().islands;
                f32 maxDepth = 0.0f;
                for (const auto& island : islands)
                    maxDepth = std::max(maxDepth, island.depth);

                ImGui::TextColored(kYellow, "%s %d island%s detected (max depth: %.1fmm)",
                                   Icons::Warning,
                                   static_cast<int>(islands.size()),
                                   islands.size() != 1 ? "s" : "",
                                   static_cast<double>(maxDepth));
                ImGui::TextWrapped("A roughing pass will clear material around islands before "
                                   "the finishing pass, preventing deep plunges with the finishing tool.");

                ImGui::Spacing();

                if (!m_recommendation.clearing.empty()) {
                    ImGui::Text("Clearing Tool:");
                    ImGui::Indent();
                    for (int i = 0; i < static_cast<int>(m_recommendation.clearing.size()); ++i) {
                        const auto& tc = m_recommendation.clearing[static_cast<size_t>(i)];
                        bool selected = (i == m_selectedClearIdx);

                        char label[192];
                        const char* clearTypeStr = (tc.geometry.tool_type == VtdbToolType::EndMill)
                            ? "End Mill" : "Ball Nose";
                        std::snprintf(label, sizeof(label), "%s %.3gmm  (score: %.0f%%)",
                                      clearTypeStr,
                                      tc.geometry.diameter,
                                      static_cast<double>(tc.score * 100.0f));

                        if (ImGui::Selectable(label, selected)) {
                            if (selected) {
                                // Deselect
                                m_selectedClearIdx = -1;
                                m_clearToolSelected = false;
                            } else {
                                m_selectedClearIdx = i;
                                m_clearTool = tc.geometry;
                                m_clearToolSelected = true;
                            }
                            // Invalidate toolpath so it regenerates with new clearing choice
                            m_toolpathGenerated = false;
                        }
                        // Show reasoning as tooltip
                        if (ImGui::IsItemHovered() && !tc.reasoning.empty())
                            ImGui::SetTooltip("%s", tc.reasoning.c_str());
                    }
                    ImGui::Unindent();
                } else {
                    ImGui::TextColored(kYellow,
                        "No suitable clearing tools in your toolbox. "
                        "Add an end mill or ball nose to My Toolbox.");
                }

                if (m_clearToolSelected) {
                    const char* clrType = (m_clearTool.tool_type == VtdbToolType::EndMill)
                        ? "End Mill" : "Ball Nose";
                    ImGui::TextColored(kGreen, "%s Clearing: %s  %.3gmm",
                                       Icons::Check, clrType, m_clearTool.diameter);
                }
            } else {
                ImGui::TextColored(kGreen, "%s No islands detected - no roughing pass needed.",
                                   Icons::Check);
            }
        }
    }
}

void DirectCarvePanel::renderToolLibraryPicker() {
    float availH = ImGui::GetContentRegionAvail().y
                   - ImGui::GetFrameHeightWithSpacing() * 3.0f; // room for nav buttons
    if (availH < ImGui::GetFrameHeightWithSpacing() * 3.0f)
        availH = ImGui::GetFrameHeightWithSpacing() * 3.0f;

    ImGui::BeginChild("##toolList", ImVec2(0, availH), ImGuiChildFlags_Borders);
    for (int i = 0; i < static_cast<int>(m_libraryTools.size()); ++i) {
        const auto& g = m_libraryTools[static_cast<size_t>(i)];

        // Type badge color
        ImVec4 badgeColor = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Secondary);
        const char* typeLabel = "Tool";
        switch (g.tool_type) {
        case VtdbToolType::BallNose:
            badgeColor = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Primary);
            typeLabel = "Ball Nose";
            break;
        case VtdbToolType::TaperedBallNose:
            badgeColor = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Warning);
            typeLabel = "TBN";
            break;
        case VtdbToolType::VBit:
            badgeColor = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Success);
            typeLabel = "V-Bit";
            break;
        case VtdbToolType::EndMill:
            badgeColor = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Error);
            typeLabel = "End Mill";
            break;
        default: break;
        }

        ImGui::PushID(i);
        bool selected = (i == m_selectedLibToolIdx);

        // Selectable row
        std::string resolved = resolveToolNameFormat(g);
        char label[128];
        std::snprintf(label, sizeof(label), "%s", resolved.c_str());

        if (ImGui::Selectable(label, selected, 0, ImVec2(0, 0))) {
            m_selectedLibToolIdx = i;
            m_finishTool = g;
            m_finishingToolSelected = true;
            m_recommendationRun = false; // Re-run with new finishing tool
            m_toolpathGenerated = false;
        }

        // Inline specs on same line
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.55f);
        ImGui::PushStyleColor(ImGuiCol_Button, badgeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, badgeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, badgeColor);
        ImGui::SmallButton(typeLabel);
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::TextDisabled("%.3gmm", g.diameter);

        if (g.tool_type == VtdbToolType::VBit && g.included_angle > 0.0) {
            ImGui::SameLine();
            ImGui::TextDisabled("%.0f deg", g.included_angle);
        }

        ImGui::PopID();
    }
    ImGui::EndChild();
}

void DirectCarvePanel::renderManualToolEntry() {
    float iw = ImGui::GetFontSize() * 8.0f;

    const char* typeNames[] = {"Ball Nose", "V-Bit", "End Mill", "Tapered Ball Nose"};
    ImGui::SetNextItemWidth(iw);
    ImGui::Combo("Tool Type", &m_manualToolType, typeNames, 4);

    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Diameter (mm)", &m_manualDiameter, 0.1f, 1.0f, "%.3f");
    m_manualDiameter = std::clamp(m_manualDiameter, 0.1f, 50.0f);

    ImGui::SetNextItemWidth(iw);
    ImGui::InputInt("Flutes", &m_manualFlutes);
    m_manualFlutes = std::clamp(m_manualFlutes, 1, 8);

    // Type-specific fields
    if (m_manualToolType == 1) { // V-Bit
        ImGui::SetNextItemWidth(iw);
        ImGui::InputFloat("Included Angle (deg)", &m_manualAngle, 1.0f, 10.0f, "%.1f");
        m_manualAngle = std::clamp(m_manualAngle, 10.0f, 180.0f);
    }
    if (m_manualToolType == 0 || m_manualToolType == 3) { // Ball Nose / TBN
        ImGui::SetNextItemWidth(iw);
        float halfDia = m_manualDiameter * 0.5f;
        ImGui::InputFloat("Tip Radius (mm)", &m_manualTipRadius, 0.1f, 0.5f, "%.3f");
        m_manualTipRadius = std::clamp(m_manualTipRadius, 0.05f, halfDia);
    }

    ImGui::Spacing();
    bool canAccept = m_manualDiameter > 0.0f;
    if (!canAccept) ImGui::BeginDisabled();
    if (ImGui::Button("Use This Tool")) {
        VtdbToolType types[] = {
            VtdbToolType::BallNose, VtdbToolType::VBit,
            VtdbToolType::EndMill, VtdbToolType::TaperedBallNose
        };
        m_finishTool = VtdbToolGeometry{};
        m_finishTool.tool_type = types[m_manualToolType];
        m_finishTool.diameter = static_cast<f64>(m_manualDiameter);
        m_finishTool.num_flutes = m_manualFlutes;
        m_finishTool.included_angle = static_cast<f64>(m_manualAngle);
        m_finishTool.tip_radius = static_cast<f64>(m_manualTipRadius);
        m_finishTool.units = VtdbUnits::Metric;
        m_finishingToolSelected = true;
        m_recommendationRun = false; // Re-run with new finishing tool
        m_toolpathGenerated = false;
    }
    if (!canAccept) ImGui::EndDisabled();
}

void DirectCarvePanel::renderMaterialSetup() {
    ImGui::TextUnformatted("Material & Feeds");
    ImGui::Spacing();
    ImGui::TextWrapped("Select the workpiece material and confirm feed rates.");
    ImGui::Spacing();

    // Load materials from database on first visit
    if (!m_materialListLoaded && m_materialMgr) {
        m_materialListLoaded = true;
        m_materialList = m_materialMgr->getAllMaterials();
    }

    float iw = ImGui::GetContentRegionAvail().x * 0.45f;

    // --- Material selector ---
    if (!m_materialList.empty()) {
        ImGui::SetNextItemWidth(iw);
        const char* preview = (m_selectedMaterialIdx >= 0)
            ? m_materialList[static_cast<size_t>(m_selectedMaterialIdx)].name.c_str()
            : "Select material...";
        if (ImGui::BeginCombo("Material", preview)) {
            // Group by category
            const char* catLabels[] = {"Hardwood", "Softwood", "Domestic", "Composite"};
            MaterialCategory cats[] = {
                MaterialCategory::Hardwood, MaterialCategory::Softwood,
                MaterialCategory::Domestic, MaterialCategory::Composite
            };
            for (int c = 0; c < 4; ++c) {
                bool hasItems = false;
                for (int i = 0; i < static_cast<int>(m_materialList.size()); ++i) {
                    if (m_materialList[static_cast<size_t>(i)].category != cats[c])
                        continue;
                    if (!hasItems) {
                        ImGui::SeparatorText(catLabels[c]);
                        hasItems = true;
                    }
                    const auto& mat = m_materialList[static_cast<size_t>(i)];
                    bool selected = (i == m_selectedMaterialIdx);
                    if (ImGui::Selectable(mat.name.c_str(), selected)) {
                        m_selectedMaterialIdx = i;
                        m_materialName = mat.name;
                        m_materialSelected = true;

                        // Auto-calculate feed rates from Janka hardness + tool diameter
                        if (mat.jankaHardness > 0.0f) {
                            // Reference point: 1000 lbf Janka → 1000 mm/min base feed
                            // with a 6mm tool. Scale inversely with hardness, proportionally
                            // with tool diameter (larger tools handle more load).
                            f32 janka = mat.jankaHardness;
                            f32 toolDia = static_cast<f32>(m_finishTool.diameter);
                            if (toolDia <= 0.0f) toolDia = 3.175f; // 1/8" fallback
                            f32 refDia = 6.0f;
                            f32 refJanka = 1000.0f;
                            f32 refFeed = 1000.0f;

                            // Softer wood → higher feed; harder → lower
                            // Larger tool → higher feed; smaller → lower
                            f32 hardnessRatio = refJanka / janka;
                            f32 diameterRatio = toolDia / refDia;
                            f32 feed = refFeed * hardnessRatio * diameterRatio;

                            // Clamp to sane range
                            feed = std::clamp(feed, 200.0f, 5000.0f);
                            // Round to nearest 50
                            feed = std::round(feed / 50.0f) * 50.0f;
                            m_toolpathConfig.feedRateMmMin = feed;

                            // Plunge rate: typically 30-50% of feed rate
                            f32 plunge = feed * 0.3f;
                            plunge = std::clamp(plunge, 100.0f, 1500.0f);
                            plunge = std::round(plunge / 50.0f) * 50.0f;
                            m_toolpathConfig.plungeRateMmMin = plunge;
                        }
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        if (!m_materialMgr) {
            ImGui::TextColored(kYellow, "No material database available.");
        } else {
            ImGui::TextDisabled("No materials in library.");
        }
    }

    // Show Janka hardness if selected
    if (m_selectedMaterialIdx >= 0) {
        const auto& mat = m_materialList[static_cast<size_t>(m_selectedMaterialIdx)];
        if (mat.jankaHardness > 0.0f) {
            ImGui::SameLine();
            ImGui::TextDisabled("Janka: %.0f lbf", mat.jankaHardness);
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Feed Rates");

    // --- Feed rate inputs (wider, typeable) ---
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Feed Rate (mm/min)", &m_toolpathConfig.feedRateMmMin, 50.0f, 200.0f, "%.0f");
    m_toolpathConfig.feedRateMmMin = std::clamp(m_toolpathConfig.feedRateMmMin, 10.0f, 20000.0f);

    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Plunge Rate (mm/min)", &m_toolpathConfig.plungeRateMmMin, 10.0f, 50.0f, "%.0f");
    m_toolpathConfig.plungeRateMmMin = std::clamp(m_toolpathConfig.plungeRateMmMin, 5.0f, 5000.0f);

    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Safe Z (mm)", &m_toolpathConfig.safeZMm, 0.5f, 2.0f, "%.1f");
    m_toolpathConfig.safeZMm = std::clamp(m_toolpathConfig.safeZMm, 1.0f, 50.0f);

    ImGui::SetNextItemWidth(iw);
    const char* stepoverLabels[] = {"Ultra Fine (1%)", "Fine (8%)", "Basic (12%)", "Rough (25%)", "Roughing (40%)"};
    int stepIdx = static_cast<int>(m_toolpathConfig.stepoverPreset);
    if (ImGui::Combo("Stepover", &stepIdx, stepoverLabels, 5))
        m_toolpathConfig.stepoverPreset = static_cast<carve::StepoverPreset>(stepIdx);

    ImGui::Spacing();
    ImGui::SeparatorText("Scan Pattern");

    ImGui::SetNextItemWidth(iw);
    const char* axisLabels[] = {"X Only", "Y Only", "X then Y", "Y then X"};
    int axisIdx = static_cast<int>(m_toolpathConfig.axis);
    if (ImGui::Combo("Scan Axis", &axisIdx, axisLabels, 4))
        m_toolpathConfig.axis = static_cast<carve::ScanAxis>(axisIdx);

    ImGui::SetNextItemWidth(iw);
    const char* dirLabels[] = {"Climb", "Conventional", "Alternating (Zigzag)"};
    int dirIdx = static_cast<int>(m_toolpathConfig.direction);
    if (ImGui::Combo("Mill Direction", &dirIdx, dirLabels, 3))
        m_toolpathConfig.direction = static_cast<carve::MillDirection>(dirIdx);

    // Auto-confirm when material is selected
    if (!m_materialSelected && m_materialList.empty()) {
        ImGui::Spacing();
        ImGui::Checkbox("Confirm settings", &m_materialSelected);
    }
}

// --- renderPreview (18-02): Heightmap overlay, toolpath lines, stats, controls ---

void DirectCarvePanel::renderPreview() {
    ImGui::TextUnformatted("Toolpath Preview");
    ImGui::Spacing();

    if (!m_carveJob) {
        ImGui::TextColored(kRed, "Carve job not initialized.");
        return;
    }

    if (!m_modelLoaded) {
        ImGui::TextColored(kYellow, "Load an STL model first (go back to Model step).");
        return;
    }

    auto jobState = m_carveJob->state();
    float bw = ImGui::GetFontSize() * 14.0f;
    bool hmReady = (jobState == carve::CarveJobState::Ready);
    bool hmComputing = (jobState == carve::CarveJobState::Computing);

    // Auto-load or auto-compute heightmap on first entry to Preview
    if (jobState == carve::CarveJobState::Idle && !m_hmInitAttempted && !m_hmFileMissing) {
        m_hmInitAttempted = true;
        m_fitter.setStock(m_stock);

        // Check project manifest for cached heightmap
        bool loaded = false;
        if (m_projectManager) {
            auto dir = m_projectManager->ensureProjectForModel(m_modelName, m_modelSourcePath);
            if (dir && !dir->heightmaps().empty()) {
                const auto& entry = dir->heightmaps().front();
                Path fullPath = dir->heightmapsDir() / entry.filename;
                if (fs::exists(fullPath)) {
                    if (m_carveJob->loadHeightmap(fullPath.string())) {
                        loaded = true;
                        m_heightmapSaved = true; // Already on disk
                        ToastManager::instance().show(ToastType::Success,
                            "Heightmap Loaded", "Loaded cached heightmap from project");
                    }
                } else {
                    m_hmFileMissing = true;
                    m_hmMissingPath = fullPath.string();
                }
            }
        }

        // First time — no cache found, auto-compute
        if (!loaded && !m_hmFileMissing) {
            carve::HeightmapConfig hmCfg;
            m_carveJob->startHeightmap(m_vertices, m_indices, m_fitter,
                                        m_fitParams, hmCfg);
            hmComputing = true;
        }

        // Refresh state after potential load
        jobState = m_carveJob->state();
        hmReady = (jobState == carve::CarveJobState::Ready);
    }

    // Missing file recovery UI
    if (m_hmFileMissing) {
        ImGui::TextColored(kRed, "Cached heightmap file not found:");
        ImGui::TextWrapped("%s", m_hmMissingPath.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Regenerate", ImVec2(bw, 0))) {
            ImGui::OpenPopup("Confirm Regenerate##missing");
        }
        ImGui::SameLine();
        if (ImGui::Button("Locate...", ImVec2(bw, 0))) {
            if (m_fileDialog) {
                m_fileDialog->showOpen(
                    "Locate Heightmap",
                    {{"Heightmap", "*.dwhm"}},
                    [this](const std::string& path) {
                        if (m_carveJob->loadHeightmap(path)) {
                            m_hmFileMissing = false;
                            m_heightmapSaved = true;
                            ToastManager::instance().show(ToastType::Success,
                                "Heightmap Loaded", path);
                        } else {
                            ToastManager::instance().show(ToastType::Error,
                                "Load Failed", "Could not read " + path);
                        }
                    });
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(bw, 0))) {
            m_hmFileMissing = false;
        }

        if (ImGui::BeginPopupModal("Confirm Regenerate##missing", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("This will recompute the heightmap from the mesh. This may take a while.");
            ImGui::Spacing();
            if (ImGui::Button("Continue", ImVec2(bw, 0))) {
                m_hmFileMissing = false;
                m_heightmapSaved = false;
                m_fitter.setStock(m_stock);
                carve::HeightmapConfig hmCfg;
                m_carveJob->startHeightmap(m_vertices, m_indices, m_fitter,
                                            m_fitParams, hmCfg);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##regen", ImVec2(bw, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        return;
    }

    // Step 1: Heightmap
    {
        if (hmComputing) {
            ImGui::TextColored(kYellow, "1. Computing heightmap...");
            CenteredProgressBar(m_carveJob->progress(), ImVec2(-1, 0), "Computing heightmap...");
        } else if (jobState == carve::CarveJobState::Error) {
            ImGui::TextColored(kRed, "1. Heightmap error: %s", m_carveJob->errorMessage().c_str());
            if (ImGui::Button("Retry", ImVec2(bw, 0))) {
                m_fitter.setStock(m_stock);
                carve::HeightmapConfig hmCfg;
                m_carveJob->startHeightmap(m_vertices, m_indices, m_fitter,
                                            m_fitParams, hmCfg);
            }
        } else if (hmReady) {
            // Auto-save after first computation
            if (!m_heightmapSaved) {
                saveHeightmapToProject();
                m_heightmapSaved = true;
            }

            const auto& hm = m_carveJob->heightmap();
            ImGui::TextColored(kGreen, "1. Heightmap: Ready");
            ImGui::SameLine();
            ImGui::TextDisabled("(%dx%d, %.2f mm/px)", hm.cols(), hm.rows(), hm.resolution());

            // Export + Regenerate buttons
            if (ImGui::Button("Export Image")) saveImageToProject();
            ImGui::SameLine();
            if (ImGui::Button("Regenerate Heightmap")) {
                ImGui::OpenPopup("Confirm Regenerate");
            }

            if (ImGui::BeginPopupModal("Confirm Regenerate", nullptr,
                                        ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextWrapped(
                    "This will recompute the heightmap and overwrite the cached version. "
                    "This may take a while. Continue?");
                ImGui::Spacing();
                if (ImGui::Button("Continue", ImVec2(bw, 0))) {
                    m_heightmapSaved = false;
                    m_toolpathGenerated = false;
                    m_fitter.setStock(m_stock);
                    carve::HeightmapConfig hmCfg;
                    m_carveJob->startHeightmap(m_vertices, m_indices, m_fitter,
                                                m_fitParams, hmCfg);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel##regen2", ImVec2(bw, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    // Step 2: Toolpath (only after heightmap)
    ImGui::Spacing();
    {
        ImVec4 tpColor = m_toolpathGenerated ? kGreen : (hmReady ? kYellow : kDimmed);
        ImGui::TextColored(tpColor, m_toolpathGenerated
            ? "2. Toolpath: Generated" : "2. Toolpath: Not generated");

        if (hmReady && !m_toolpathGenerated) {
            if (ImGui::Button("Generate Toolpath", ImVec2(bw, 0))) {
                f32 toolAngle = static_cast<f32>(m_finishTool.included_angle);
                if (toolAngle <= 0.0f) toolAngle = 90.0f;
                // Analyze if not already done in tool selection step
                if (!m_recommendationRun)
                    m_carveJob->analyzeHeightmap(toolAngle);

                const VtdbToolGeometry* clrPtr =
                    m_clearToolSelected ? &m_clearTool : nullptr;
                m_carveJob->generateToolpath(m_toolpathConfig, m_finishTool, clrPtr);
                m_toolpathGenerated = true;
            }
        }
    }

    if (!m_toolpathGenerated) return;

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    const auto& tp = m_carveJob->toolpath();
    const auto& hm = m_carveJob->heightmap();

    // Preview area sized to heightmap aspect ratio
    float panelW = ImGui::GetContentRegionAvail().x;
    float aspect = (hm.cols() > 0 && hm.rows() > 0)
        ? static_cast<f32>(hm.cols()) / static_cast<f32>(hm.rows()) : 1.0f;
    float imgW = panelW * m_previewZoom;
    float imgH = imgW / aspect;

    ImVec2 imgPos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(imgW, imgH));

    // Toolpath overlay via ImDrawList
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Vec3 hmMin = hm.boundsMin();
    Vec3 hmMax = hm.boundsMax();
    f32 rx = hmMax.x - hmMin.x;
    f32 ry = hmMax.y - hmMin.y;

    auto toScreen = [&](const Vec3& p) -> ImVec2 {
        f32 nx = (rx > 0.0f) ? (p.x - hmMin.x) / rx : 0.5f;
        f32 ny = (ry > 0.0f) ? (p.y - hmMin.y) / ry : 0.5f;
        return ImVec2(imgPos.x + nx * imgW, imgPos.y + (1.0f - ny) * imgH);
    };

    if (m_showFinishing && tp.finishing.points.size() > 1) {
        for (size_t i = 1; i < tp.finishing.points.size(); ++i) {
            ImU32 c = tp.finishing.points[i].rapid ? kRapidColor : kFinishColor;
            dl->AddLine(toScreen(tp.finishing.points[i-1].position),
                        toScreen(tp.finishing.points[i].position), c, 1.0f);
        }
    }
    if (m_showClearing && tp.clearing.points.size() > 1) {
        for (size_t i = 1; i < tp.clearing.points.size(); ++i) {
            ImU32 c = tp.clearing.points[i].rapid ? kRapidColor : kClearColor;
            dl->AddLine(toScreen(tp.clearing.points[i-1].position),
                        toScreen(tp.clearing.points[i].position), c, 1.0f);
        }
    }

    // Statistics
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::Text("Finishing: %d scan lines, %s, %.0f mm",
                tp.finishing.lineCount,
                formatTime(tp.finishing.estimatedTimeSec).c_str(),
                static_cast<double>(tp.finishing.totalDistanceMm));
    if (!tp.clearing.points.empty())
        ImGui::Text("Clearing:  %d lines, %s, %.0f mm",
                    tp.clearing.lineCount,
                    formatTime(tp.clearing.estimatedTimeSec).c_str(),
                    static_cast<double>(tp.clearing.totalDistanceMm));
    ImGui::Text("Total estimated time: %s", formatTime(tp.totalTimeSec).c_str());

    // Controls
    ImGui::Spacing();
    ImGui::Checkbox("Show finishing", &m_showFinishing);
    ImGui::SameLine();
    ImGui::Checkbox("Show clearing", &m_showClearing);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
    ImGui::SliderFloat("Zoom", &m_previewZoom, 0.25f, 4.0f, "%.1fx");

    // G-code export
    ImGui::Spacing();
    if (ImGui::Button("Save G-code", ImVec2(bw, 0)))
        saveGCodeToProject();
}

// --- renderOutlineTest (18-02): Bounding box, G-code outline, skip ---

void DirectCarvePanel::renderOutlineTest() {
    ImGui::TextUnformatted("Outline Test");
    ImGui::Spacing();
    ImGui::TextWrapped("Traces the job perimeter at safe Z to verify work area.");
    ImGui::Spacing();

    if (!m_carveJob || !m_toolpathGenerated) {
        ImGui::TextColored(kYellow, "Generate a toolpath first.");
        return;
    }

    const auto& tp = m_carveJob->toolpath();
    f32 minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (const auto& pt : tp.finishing.points) {
        if (!pt.rapid) {
            minX = std::min(minX, pt.position.x);
            maxX = std::max(maxX, pt.position.x);
            minY = std::min(minY, pt.position.y);
            maxY = std::max(maxY, pt.position.y);
        }
    }

    ImGui::Text("Bounding box: X[%.1f .. %.1f]  Y[%.1f .. %.1f]",
                static_cast<double>(minX), static_cast<double>(maxX),
                static_cast<double>(minY), static_cast<double>(maxY));
    ImGui::Text("Size: %.1f x %.1f mm",
                static_cast<double>(maxX - minX), static_cast<double>(maxY - minY));
    ImGui::Spacing();

    float bw = ImGui::GetFontSize() * 10.0f;

    if (!m_outlineCompleted && !m_outlineRunning) {
        if (ImGui::Button("Run Outline", ImVec2(bw, 0))) {
            if (m_cnc && m_cncConnected) {
                f32 safeZ = m_toolpathConfig.safeZMm;
                char cmd[128];
                std::snprintf(cmd, sizeof(cmd), "G90 G0 Z%.3f", static_cast<double>(safeZ));
                m_cnc->sendCommand(cmd);
                std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                              static_cast<double>(minX), static_cast<double>(minY));
                m_cnc->sendCommand(cmd);
                std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                              static_cast<double>(maxX), static_cast<double>(minY));
                m_cnc->sendCommand(cmd);
                std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                              static_cast<double>(maxX), static_cast<double>(maxY));
                m_cnc->sendCommand(cmd);
                std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                              static_cast<double>(minX), static_cast<double>(maxY));
                m_cnc->sendCommand(cmd);
                std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                              static_cast<double>(minX), static_cast<double>(minY));
                m_cnc->sendCommand(cmd);
                m_outlineCompleted = true;
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Skip Outline", &m_outlineSkipped);
    }

    if (m_outlineCompleted)
        ImGui::TextColored(kGreen, "Outline complete -- verify work area before proceeding.");
    if (m_outlineSkipped && !m_outlineCompleted)
        ImGui::TextColored(kYellow, "Outline test skipped.");
}

void DirectCarvePanel::renderZeroConfirm() {
    ImGui::TextUnformatted("Zero Position Confirmation");
    ImGui::Spacing();

    ImGui::Text("Current Work Position:");
    ImGui::Indent();
    ImGui::Text("X: %.3f  Y: %.3f  Z: %.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Unindent();

    bool nearZero = (std::fabs(m_machineStatus.workPos.x) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.y) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.z) < 0.5f);
    if (nearZero) ImGui::TextColored(kGreen, "Position is near zero origin.");

    ImGui::Spacing();
    ImGui::TextWrapped("Position the tool at the work zero point "
                       "(bottom-left of stock, Z on top surface).");
    ImGui::Spacing();

    float bw = ImGui::GetFontSize() * 10.0f;
    bool canSend = (m_cnc != nullptr && m_cncConnected);
    if (!canSend) ImGui::BeginDisabled();
    if (ImGui::Button("Set Zero Here", ImVec2(bw, 0)))
        m_cnc->sendCommand("G10 L20 P0 X0 Y0 Z0");
    ImGui::SameLine();
    if (ImGui::Button("Zero XY Only", ImVec2(bw, 0)))
        m_cnc->sendCommand("G10 L20 P0 X0 Y0");
    ImGui::SameLine();
    if (ImGui::Button("Zero Z Only", ImVec2(bw, 0)))
        m_cnc->sendCommand("G10 L20 P0 Z0");
    if (!canSend) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Checkbox("Zero position is set and verified", &m_zeroConfirmed);
}

void DirectCarvePanel::renderCommit() {
    ImGui::TextUnformatted("Final Confirmation");
    ImGui::Spacing();
    ImGui::TextWrapped("Review the carve job parameters before starting:");
    ImGui::Spacing();

    ImGui::BulletText("Machine: %s", m_cncConnected ? "Connected" : "DISCONNECTED");
    ImGui::BulletText("Stock: %.0f x %.0f x %.0f mm",
                      static_cast<double>(m_stock.width),
                      static_cast<double>(m_stock.height),
                      static_cast<double>(m_stock.thickness));
    ImGui::BulletText("Feed: %.0f mm/min, Plunge: %.0f mm/min",
                      static_cast<double>(m_toolpathConfig.feedRateMmMin),
                      static_cast<double>(m_toolpathConfig.plungeRateMmMin));
    ImGui::BulletText("Safe Z: %.1f mm", static_cast<double>(m_toolpathConfig.safeZMm));

    if (m_carveJob) {
        const auto& tp = m_carveJob->toolpath();
        ImGui::BulletText("Estimated time: %s", formatTime(tp.totalTimeSec).c_str());
        ImGui::BulletText("G-code lines: %d", tp.totalLineCount);
    }

    ImGui::Spacing();

    // G-code export button
    float bw = ImGui::GetFontSize() * 10.0f;
    if (ImGui::Button("Save as G-code", ImVec2(bw, 0)))
        saveGCodeToProject();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kYellow);
    ImGui::TextWrapped("This will begin streaming G-code to the machine. "
                       "Ensure the work area is clear and the spindle is ready.");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Checkbox("I confirm the above and am ready to carve", &m_commitConfirmed);
}

void DirectCarvePanel::renderRunning() {
    float fontSize = ImGui::GetFontSize();
    float bw = fontSize * 8.0f;

    // State label
    const char* stateLabel = "Unknown";
    ImVec4 stateColor = kDimmed;
    switch (m_runState) {
    case RunState::Active:
        stateLabel = "Streaming";
        stateColor = kGreen;
        break;
    case RunState::Paused:
        stateLabel = "Paused (Feed Hold)";
        stateColor = kYellow;
        break;
    case RunState::Completed:
        stateLabel = "Complete";
        stateColor = kGreen;
        break;
    case RunState::Aborted:
        stateLabel = "Aborted";
        stateColor = kRed;
        break;
    }

    ImGui::TextColored(stateColor, "%s", stateLabel);
    ImGui::Spacing();

    // Progress bar
    if (m_runTotalLines > 0) {
        float fraction = static_cast<float>(m_runCurrentLine) /
                         static_cast<float>(m_runTotalLines);
        float etaSec = 0.0f;
        if (m_runCurrentLine > 0 && fraction < 1.0f && m_runElapsedSec > 0.0f) {
            float rate = static_cast<float>(m_runCurrentLine) / m_runElapsedSec;
            float remaining = static_cast<float>(m_runTotalLines - m_runCurrentLine);
            etaSec = remaining / rate;
        }
        char overlay[128];
        int etaMin = static_cast<int>(etaSec / 60.0f);
        int etaS = static_cast<int>(etaSec) % 60;
        std::snprintf(overlay, sizeof(overlay), "Line %d / %d  (%.0f%%)  ETA: %d:%02d",
                      m_runCurrentLine, m_runTotalLines,
                      static_cast<double>(fraction * 100.0f), etaMin, etaS);
        CenteredProgressBar(fraction, ImVec2(-1, 0), overlay);
    }

    // Pass indicator and elapsed time
    if (!m_runCurrentPass.empty())
        ImGui::Text("Pass: %s", m_runCurrentPass.c_str());
    ImGui::Text("Elapsed: %s", formatTime(m_runElapsedSec).c_str());

    // Machine position
    ImGui::Text("Position: X%.3f Y%.3f Z%.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Spacing();

    // Control buttons (active job only)
    if (m_runState == RunState::Active || m_runState == RunState::Paused) {
        // Pause / Resume
        if (m_runState == RunState::Active) {
            if (ImGui::Button("Pause", ImVec2(bw, 0))) {
                if (m_cnc) m_cnc->feedHold();
                m_runState = RunState::Paused;
            }
        } else {
            if (ImGui::Button("Resume", ImVec2(bw, 0))) {
                if (m_cnc) m_cnc->cycleStart();
                m_runState = RunState::Active;
            }
        }

        // Abort — long-press for safety
        ImGui::SameLine();
        ImGui::Button("Hold to Abort", ImVec2(bw * 1.2f, 0));
        bool isHeld = ImGui::IsItemActive();
        float requiredMs = 1500.0f;
        if (isHeld) {
            m_abortHoldTime += ImGui::GetIO().DeltaTime * 1000.0f;
            float progress = std::min(m_abortHoldTime / requiredMs, 1.0f);
            ImVec2 rmin = ImGui::GetItemRectMin();
            ImVec2 rmax = ImGui::GetItemRectMax();
            ImVec2 fillMax = {rmin.x + (rmax.x - rmin.x) * progress, rmax.y};
            ImGui::GetWindowDrawList()->AddRectFilled(
                rmin, fillMax, IM_COL32(255, 80, 80, 60), 3.0f);
            m_abortHolding = true;
            if (m_abortHoldTime >= requiredMs) {
                // Execute abort sequence
                if (m_cnc) {
                    m_cnc->feedHold();
                    m_cnc->softReset();
                }
                m_runState = RunState::Aborted;
                if (m_gcodePanel) m_gcodePanel->onCarveStreamAborted();
                m_abortHoldTime = 0.0f;
                m_abortHolding = false;
            }
        } else {
            if (m_abortHolding) { m_abortHolding = false; m_abortHoldTime = 0.0f; }
        }
    }

    // Post-completion / post-abort UI
    if (m_runState == RunState::Completed) {
        ImGui::Spacing();
        ImGui::TextColored(kGreen, "Carve completed successfully.");
        if (ImGui::Button("Save G-code", ImVec2(bw, 0)))
            saveGCodeToProject();
    }

    if (m_runState == RunState::Aborted) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, kRed);
        ImGui::TextWrapped("Job aborted. Tool may be in workpiece -- "
                           "jog Z up before moving XY.");
        ImGui::PopStyleColor();
        if (ImGui::Button("Save G-code", ImVec2(bw, 0)))
            saveGCodeToProject();
    }
}

void DirectCarvePanel::saveHeightmapToProject() {
    if (!m_projectManager || !m_carveJob) {
        // Fallback to FileDialog
        if (m_fileDialog) {
            m_fileDialog->showSave(
                "Save Heightmap",
                {{"Heightmap", "*.dwhm"}},
                "heightmap.dwhm",
                [this](const std::string& path) {
                    m_carveJob->heightmap().save(path);
                });
        }
        return;
    }

    auto dir = m_projectManager->ensureProjectForModel(m_modelName, m_modelSourcePath);
    if (!dir) {
        ToastManager::instance().show(ToastType::Error,
            "Project Error", "Failed to create project directory");
        return;
    }

    std::string baseName = ProjectDirectory::sanitizeName(m_modelName);
    Path destPath = dir->heightmapsDir() / (baseName + ".dwhm");
    const auto& hm = m_carveJob->heightmap();

    if (hm.save(destPath.string())) {
        dir->addHeightmap(baseName + ".dwhm", hm.resolution());
        dir->save();
        ToastManager::instance().show(ToastType::Success,
            "Heightmap Saved", destPath.string());
    } else {
        ToastManager::instance().show(ToastType::Error,
            "Save Failed", "Could not write " + destPath.string());
    }
}

void DirectCarvePanel::saveImageToProject() {
    if (!m_projectManager || !m_carveJob) {
        // Fallback to FileDialog
        if (m_fileDialog) {
            m_fileDialog->showSave(
                "Export Heightmap Image",
                {{"PGM Image (16-bit)", "*.pgm"}},
                "heightmap.pgm",
                [this](const std::string& path) {
                    m_carveJob->heightmap().exportPng(path);
                });
        }
        return;
    }

    auto dir = m_projectManager->ensureProjectForModel(m_modelName, m_modelSourcePath);
    if (!dir) {
        ToastManager::instance().show(ToastType::Error,
            "Project Error", "Failed to create project directory");
        return;
    }

    std::string baseName = ProjectDirectory::sanitizeName(m_modelName);
    Path destPath = dir->imagesDir() / (baseName + ".pgm");

    if (m_carveJob->heightmap().exportPng(destPath.string())) {
        dir->save();
        ToastManager::instance().show(ToastType::Success,
            "Image Exported", destPath.string());
    } else {
        ToastManager::instance().show(ToastType::Error,
            "Export Failed", "Could not write " + destPath.string());
    }
}

void DirectCarvePanel::saveGCodeToProject() {
    if (!m_projectManager || !m_carveJob) {
        showExportDialog();
        return;
    }

    auto dir = m_projectManager->ensureProjectForModel(m_modelName, m_modelSourcePath);
    if (!dir) {
        ToastManager::instance().show(ToastType::Error,
            "Project Error", "Failed to create project directory");
        showExportDialog();
        return;
    }

    std::string baseName = ProjectDirectory::sanitizeName(m_modelName);
    Path destPath = dir->gcodeDir() / (baseName + ".nc");
    const auto& tp = m_carveJob->toolpath();
    std::string toolName = m_finishTool.name_format;

    if (carve::exportGcode(destPath.string(), tp, m_toolpathConfig, m_modelName, toolName)) {
        dir->addGCode(baseName + ".nc", toolName);
        dir->save();
        ToastManager::instance().show(ToastType::Success,
            "G-code Saved", destPath.string());
    } else {
        ToastManager::instance().show(ToastType::Error,
            "Export Failed", "Could not write " + destPath.string());
    }
}

void DirectCarvePanel::showExportDialog() {
    if (!m_fileDialog || !m_carveJob) return;
    const auto& tp = m_carveJob->toolpath();
    std::string modelName = m_modelName;
    std::string toolName = m_finishTool.name_format;
    carve::ToolpathConfig config = m_toolpathConfig;
    m_fileDialog->showSave("Save G-code",
        {{  "G-code Files", "*.nc;*.gcode;*.ngc"}},
        "carve.nc",
        [tp, config, modelName, toolName](const std::string& path) {
            bool ok = carve::exportGcode(path, tp, config, modelName, toolName);
            if (ok)
                ToastManager::instance().show(ToastType::Success,
                    "G-code Saved", path);
            else
                ToastManager::instance().show(ToastType::Error,
                    "Export Failed", "Could not write " + path);
        });
}

} // namespace dw
