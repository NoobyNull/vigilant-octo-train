// Direct Carve wizard panel -- step-by-step guided workflow for streaming
// 2.5D toolpaths directly from STL models. Each step validates before allowing
// progression. Machine readiness is verified before any carving begins.

#include "ui/panels/direct_carve_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "core/carve/analysis_overlay.h"
#include "core/carve/carve_job.h"
#include "core/cnc/cnc_controller.h"
#include "core/config/config.h"
#include "core/gcode/machine_profile.h"
#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

namespace {

// Status indicator colors
constexpr ImVec4 kGreen{0.3f, 0.8f, 0.3f, 1.0f};
constexpr ImVec4 kRed{1.0f, 0.3f, 0.3f, 1.0f};
constexpr ImVec4 kYellow{1.0f, 0.8f, 0.2f, 1.0f};
constexpr ImVec4 kDimmed{0.5f, 0.5f, 0.5f, 1.0f};
constexpr ImVec4 kBright{0.4f, 0.7f, 1.0f, 1.0f};

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

DirectCarvePanel::DirectCarvePanel() : Panel("Direct Carve") {
    m_open = false;
}

DirectCarvePanel::~DirectCarvePanel() = default;

void DirectCarvePanel::setCncController(CncController* cnc) { m_cnc = cnc; }
void DirectCarvePanel::setToolDatabase(ToolDatabase* db) { m_toolDb = db; }
void DirectCarvePanel::setCarveJob(carve::CarveJob* job) { m_carveJob = job; }

void DirectCarvePanel::onConnectionChanged(bool connected) { m_cncConnected = connected; }
void DirectCarvePanel::onStatusUpdate(const MachineStatus& status) { m_machineStatus = status; }

void DirectCarvePanel::onModelLoaded(const std::vector<Vertex>& vertices,
                                      const std::vector<u32>& indices,
                                      const Vec3& boundsMin,
                                      const Vec3& boundsMax) {
    m_vertices = vertices;
    m_indices = indices;
    m_modelLoaded = true;
    m_modelBoundsMin = boundsMin;
    m_modelBoundsMax = boundsMax;
    m_fitter.setModelBounds(boundsMin, boundsMax);
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
    float spacing = fontSize * 0.5f;
    float circleR = fontSize * 0.35f;
    float lineH = ImGui::GetTextLineHeightWithSpacing();
    int curIdx = static_cast<int>(m_currentStep);
    float xPos = cursor.x;

    for (int i = 0; i < STEP_COUNT; ++i) {
        const char* label = stepLabel(static_cast<Step>(i));
        float labelW = ImGui::CalcTextSize(label).x;
        float cx = xPos + circleR;
        ImVec4 color = (i < curIdx) ? kGreen : (i == curIdx) ? kBright : kDimmed;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

        if (i <= curIdx)
            dl->AddCircleFilled(ImVec2(cx, cursor.y + circleR), circleR, col);
        else
            dl->AddCircle(ImVec2(cx, cursor.y + circleR), circleR, col, 0, 1.5f);

        dl->AddText(ImVec2(cx - labelW * 0.5f, cursor.y + circleR * 2.0f + 2.0f), col, label);

        if (i < STEP_COUNT - 1) {
            float lx = cx + circleR + 2.0f;
            float lineLen = spacing + circleR;
            ImU32 lc = (i < curIdx) ?
                ImGui::ColorConvertFloat4ToU32(kGreen) :
                ImGui::ColorConvertFloat4ToU32(kDimmed);
            dl->AddLine(ImVec2(lx, cursor.y + circleR),
                        ImVec2(lx + lineLen, cursor.y + circleR), lc, 1.5f);
        }
        xPos += circleR * 2.0f + spacing +
                std::max(labelW, circleR * 2.0f) * 0.5f + spacing;
    }
    ImGui::Dummy(ImVec2(0, lineH + circleR));
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
    ImGui::DragFloat("Width (X) mm", &m_stock.width, 1.0f, 1.0f, 2000.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::DragFloat("Height (Y) mm", &m_stock.height, 1.0f, 1.0f, 2000.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::DragFloat("Thickness (Z) mm", &m_stock.thickness, 0.5f, 0.5f, 200.0f, "%.1f");

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

    // Heightmap generation
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (m_carveJob) {
        auto st = m_carveJob->state();
        if (st == carve::CarveJobState::Computing)
            ImGui::ProgressBar(m_carveJob->progress(), ImVec2(-1, 0), "Computing heightmap...");
        else if (st == carve::CarveJobState::Ready)
            ImGui::TextColored(kGreen, "Heightmap ready.");
        else if (st == carve::CarveJobState::Error)
            ImGui::TextColored(kRed, "Error: %s", m_carveJob->errorMessage().c_str());

        bool canGen = (st == carve::CarveJobState::Idle ||
                       st == carve::CarveJobState::Ready ||
                       st == carve::CarveJobState::Error);
        if (!canGen) ImGui::BeginDisabled();
        if (ImGui::Button("Generate Heightmap", ImVec2(bw, 0))) {
            carve::HeightmapConfig hmCfg;
            m_carveJob->startHeightmap(m_vertices, m_indices, m_fitter,
                                        m_fitParams, hmCfg);
        }
        if (!canGen) ImGui::EndDisabled();
    }
}

void DirectCarvePanel::renderToolSelect() {
    ImGui::TextUnformatted("Tool Selection");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a finishing tool for the carve operation. "
                       "A V-bit is recommended for fine surface detail.");
    ImGui::Spacing();
    ImGui::Checkbox("Finishing tool selected", &m_finishingToolSelected);
    if (m_finishingToolSelected) ImGui::TextColored(kGreen, "Tool ready.");
}

void DirectCarvePanel::renderMaterialSetup() {
    ImGui::TextUnformatted("Material & Feeds");
    ImGui::Spacing();
    ImGui::TextWrapped("Select the workpiece material and confirm feed rates.");
    ImGui::Spacing();

    float iw = ImGui::GetFontSize() * 6.0f;
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Feed Rate (mm/min)", &m_toolpathConfig.feedRateMmMin, 50.0f, 200.0f, "%.0f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Plunge Rate (mm/min)", &m_toolpathConfig.plungeRateMmMin, 10.0f, 50.0f, "%.0f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Safe Z (mm)", &m_toolpathConfig.safeZMm, 0.5f, 2.0f, "%.1f");
    ImGui::Spacing();
    ImGui::Checkbox("Material confirmed", &m_materialSelected);
}

// --- renderPreview (18-02): Heightmap overlay, toolpath lines, stats, controls ---

void DirectCarvePanel::renderPreview() {
    ImGui::TextUnformatted("Toolpath Preview");
    ImGui::Spacing();

    if (!m_carveJob || m_carveJob->state() != carve::CarveJobState::Ready) {
        ImGui::TextColored(kYellow, "Generate a heightmap in the Model step first.");
        return;
    }

    if (!m_toolpathGenerated) {
        float bw = ImGui::GetFontSize() * 12.0f;
        if (ImGui::Button("Generate Toolpath", ImVec2(bw, 0))) {
            f32 toolAngle = static_cast<f32>(m_finishTool.included_angle);
            if (toolAngle <= 0.0f) toolAngle = 90.0f;
            m_carveJob->analyzeHeightmap(toolAngle);
            const VtdbToolGeometry* clrPtr =
                m_clearToolSelected ? &m_clearTool : nullptr;
            m_carveJob->generateToolpath(m_toolpathConfig, m_finishTool, clrPtr);
            m_toolpathGenerated = true;
        }
        return;
    }

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
    ImGui::PushStyleColor(ImGuiCol_Text, kYellow);
    ImGui::TextWrapped("This will begin streaming G-code to the machine. "
                       "Ensure the work area is clear and the spindle is ready.");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Checkbox("I confirm the above and am ready to carve", &m_commitConfirmed);
}

void DirectCarvePanel::renderRunning() {
    ImGui::TextUnformatted("Carve in Progress");
    ImGui::Spacing();
    ImGui::TextColored(kBright, "Streaming toolpath to machine...");
    ImGui::Spacing();

    ImGui::Text("Machine state: %s",
        m_machineStatus.state == MachineState::Run ? "Running" :
        m_machineStatus.state == MachineState::Idle ? "Idle" :
        m_machineStatus.state == MachineState::Hold ? "Hold" : "Other");
    ImGui::Text("Position: X%.3f Y%.3f Z%.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Spacing();
    ImGui::TextWrapped("Use the Safety panel or keyboard smash to emergency stop.");
}

} // namespace dw
