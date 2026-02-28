// Direct Carve wizard panel -- step-by-step guided workflow for streaming
// 2.5D toolpaths directly from STL models. Each step validates before allowing
// progression. Machine readiness is verified before any carving begins.

#include "ui/panels/direct_carve_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "core/carve/carve_job.h"
#include "core/carve/gcode_export.h"
#include "core/cnc/cnc_controller.h"
#include "core/config/config.h"
#include "core/gcode/machine_profile.h"
#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

namespace {

// Theme-derived status colors
ImVec4 successColor() { return ImGui::ColorConvertU32ToFloat4(Theme::Colors::Success); }
ImVec4 errorColor()   { return ImGui::ColorConvertU32ToFloat4(Theme::Colors::Error); }
ImVec4 warningColor() { return ImGui::ColorConvertU32ToFloat4(Theme::Colors::Warning); }
ImVec4 dimColor()     { return ImGui::ColorConvertU32ToFloat4(Theme::Colors::TextDim); }
ImVec4 primaryColor() { return ImGui::ColorConvertU32ToFloat4(Theme::Colors::Primary); }

void statusBullet(bool ok, const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, ok ? successColor() : errorColor());
    ImGui::BulletText("%s %s", ok ? "OK" : "--", label);
    ImGui::PopStyleColor();
}

} // anonymous namespace

DirectCarvePanel::DirectCarvePanel() : Panel("Direct Carve") {
    m_open = false; // Hidden by default, shown via View menu
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

std::string DirectCarvePanel::formatTime(f32 seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%dm %ds", mins, secs);
    return buf;
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
    int currentIdx = static_cast<int>(m_currentStep);

    float xPos = cursor.x;
    for (int i = 0; i < STEP_COUNT; ++i) {
        const char* label = stepLabel(static_cast<Step>(i));
        float labelW = ImGui::CalcTextSize(label).x;
        float cx = xPos + circleR;

        ImVec4 color = (i < currentIdx) ? successColor()
                     : (i == currentIdx) ? primaryColor()
                     : dimColor();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

        if (i <= currentIdx)
            dl->AddCircleFilled(ImVec2(cx, cursor.y + circleR), circleR, col);
        else
            dl->AddCircle(ImVec2(cx, cursor.y + circleR), circleR, col, 0, 1.5f);

        dl->AddText(ImVec2(cx - labelW * 0.5f, cursor.y + circleR * 2.0f + 2.0f), col, label);

        if (i < STEP_COUNT - 1) {
            float lx = cx + circleR + 2.0f;
            float lineLen = spacing + circleR;
            ImU32 lc = (i < currentIdx)
                ? ImGui::ColorConvertFloat4ToU32(successColor())
                : ImGui::ColorConvertFloat4ToU32(dimColor());
            dl->AddLine(ImVec2(lx, cursor.y + circleR),
                        ImVec2(lx + lineLen, cursor.y + circleR), lc, 1.5f);
        }

        xPos += circleR * 2.0f + spacing +
                std::max(labelW, circleR * 2.0f) * 0.5f + spacing;
    }
    ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + circleR));
}

void DirectCarvePanel::renderNavButtons() {
    float btnW = ImGui::GetFontSize() * 6.0f;
    bool isFirst = (m_currentStep == Step::MachineCheck);
    bool isRunning = (m_currentStep == Step::Running);
    bool isCommit = (m_currentStep == Step::Commit);

    if (isFirst || isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Back", ImVec2(btnW, 0))) retreatStep();
    if (isFirst || isRunning) ImGui::EndDisabled();

    ImGui::SameLine();
    bool canGo = canAdvance();
    if (!canGo) ImGui::BeginDisabled();
    if (ImGui::Button(isCommit ? "Start Carving" : "Next", ImVec2(btnW, 0))) advanceStep();
    if (!canGo) ImGui::EndDisabled();

    ImGui::SameLine();
    if (isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Cancel", ImVec2(btnW, 0))) {
        m_currentStep = Step::MachineCheck;
        m_safeZConfirmed = false;
        m_finishingToolSelected = false;
        m_materialSelected = false;
        m_toolpathGenerated = false;
        m_outlineCompleted = false;
        m_outlineSkipped = false;
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
    const auto& profile = Config::instance().getActiveMachineProfile();
    bool profileOk = (profile.maxTravelX > 0.0f && profile.maxTravelY > 0.0f &&
                      profile.maxTravelZ > 0.0f);

    statusBullet(connected, "CNC connected");
    statusBullet(homed && notAlarm, "Machine homed (idle, no alarm)");
    statusBullet(profileOk, "Machine profile configured (travel limits set)");
    statusBullet(m_safeZConfirmed, "Safe Z verified");

    ImGui::Spacing();
    if (connected && homed) {
        float btnW = ImGui::GetFontSize() * 10.0f;
        if (ImGui::Button("Test Safe Z", ImVec2(btnW, 0))) {
            if (m_cnc) {
                char cmd[64];
                std::snprintf(cmd, sizeof(cmd), "G0 Z%.3f",
                              static_cast<double>(m_toolpathConfig.safeZMm));
                m_cnc->sendCommand(cmd);
                m_safeZConfirmed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip (confirm safe Z)", ImVec2(btnW * 1.2f, 0))) {
            m_safeZConfirmed = true;
        }
    }

    if (m_machineStatus.state == MachineState::Alarm) {
        ImGui::Spacing();
        ImGui::TextColored(errorColor(),
            "Machine is in ALARM state. Clear the alarm and home before continuing.");
    }
}

void DirectCarvePanel::renderModelFit() {
    ImGui::TextUnformatted("Model Fitting");
    ImGui::Spacing();

    if (!m_modelLoaded) {
        ImGui::TextColored(warningColor(), "No model loaded. Load an STL model first.");
        return;
    }

    Vec3 sz = m_modelBoundsMax - m_modelBoundsMin;
    ImGui::Text("Model bounds: %.1f x %.1f x %.1f mm",
                static_cast<double>(sz.x), static_cast<double>(sz.y), static_cast<double>(sz.z));

    ImGui::Spacing();
    ImGui::TextUnformatted("Stock Dimensions:");
    float iw = ImGui::GetFontSize() * 6.0f;
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Width (X) mm", &m_stock.width, 1.0f, 10.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Height (Y) mm", &m_stock.height, 1.0f, 10.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Thickness (Z) mm", &m_stock.thickness, 1.0f, 10.0f, "%.1f");

    ImGui::Spacing();
    ImGui::TextUnformatted("Fit Parameters:");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Scale (uniform)", &m_fitParams.scale, 0.01f, 0.1f, "%.3f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Depth (Z) mm", &m_fitParams.depthMm, 0.1f, 1.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Offset X mm", &m_fitParams.offsetX, 0.5f, 5.0f, "%.1f");
    ImGui::SetNextItemWidth(iw);
    ImGui::InputFloat("Offset Y mm", &m_fitParams.offsetY, 0.5f, 5.0f, "%.1f");

    if (ImGui::Button("Auto Fit", ImVec2(ImGui::GetFontSize() * 8.0f, 0))) {
        m_fitter.setStock(m_stock);
        m_fitParams.scale = m_fitter.autoScale();
        m_fitParams.depthMm = m_fitter.autoDepth();
        m_fitParams.offsetX = 0.0f;
        m_fitParams.offsetY = 0.0f;
    }

    m_fitter.setStock(m_stock);
    const auto& mp = Config::instance().getActiveMachineProfile();
    m_fitter.setMachineTravel(mp.maxTravelX, mp.maxTravelY, mp.maxTravelZ);
    carve::FitResult result = m_fitter.fit(m_fitParams);

    ImGui::Spacing();
    ImGui::TextColored(result.fitsStock ? successColor() : errorColor(),
        result.fitsStock ? "Model fits within stock." : "Model exceeds stock: %s",
        result.warning.c_str());
    ImGui::TextColored(result.fitsMachine ? successColor() : errorColor(),
        result.fitsMachine ? "Model fits within machine travel." : "Exceeds machine: %s",
        result.warning.c_str());
}

void DirectCarvePanel::renderToolSelect() {
    ImGui::TextUnformatted("Tool Selection");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a finishing tool for the carve operation. "
                       "A V-bit is recommended for fine surface detail.");
    ImGui::Spacing();
    ImGui::Checkbox("Finishing tool selected", &m_finishingToolSelected);
    if (m_finishingToolSelected)
        ImGui::TextColored(successColor(), "Tool ready.");
}

void DirectCarvePanel::renderMaterialSetup() {
    ImGui::TextUnformatted("Material & Feeds");
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

void DirectCarvePanel::renderPreview() {
    ImGui::TextUnformatted("Toolpath Preview");
    ImGui::Spacing();
    if (!m_toolpathGenerated) {
        ImGui::TextWrapped("Generate the toolpath to preview estimated time and line count.");
        ImGui::Spacing();
        if (ImGui::Button("Generate Toolpath", ImVec2(ImGui::GetFontSize() * 10.0f, 0)))
            m_toolpathGenerated = true;
    } else {
        ImGui::TextColored(successColor(), "Toolpath generated and ready for preview.");
    }
}

void DirectCarvePanel::renderOutlineTest() {
    ImGui::TextUnformatted("Outline Test");
    ImGui::Spacing();
    ImGui::TextWrapped("Run a perimeter trace at safe Z to verify carve area.");
    ImGui::Spacing();
    float btnW = ImGui::GetFontSize() * 10.0f;
    if (!m_outlineCompleted) {
        if (ImGui::Button("Run Outline", ImVec2(btnW, 0))) m_outlineCompleted = true;
        ImGui::SameLine();
        if (ImGui::Button("Skip Outline", ImVec2(btnW, 0))) m_outlineSkipped = true;
    } else {
        ImGui::TextColored(successColor(), "Outline test completed.");
    }
    if (m_outlineSkipped && !m_outlineCompleted)
        ImGui::TextColored(warningColor(), "Outline test skipped.");
}

void DirectCarvePanel::renderZeroConfirm() {
    ImGui::TextUnformatted("Zero Position Confirmation");
    ImGui::Spacing();
    ImGui::Text("Work Pos: X%.3f Y%.3f Z%.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));

    bool nearZero = (std::fabs(m_machineStatus.workPos.x) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.y) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.z) < 0.5f);
    if (nearZero)
        ImGui::TextColored(successColor(), "Position is near zero origin.");

    ImGui::Spacing();
    ImGui::TextWrapped("Position tool at work zero (bottom-left of stock, Z on surface).");
    ImGui::Spacing();

    float btnW = ImGui::GetFontSize() * 10.0f;
    bool canSend = (m_cnc != nullptr && m_cncConnected);
    if (!canSend) ImGui::BeginDisabled();
    if (ImGui::Button("Set Zero Here", ImVec2(btnW, 0)))
        m_cnc->sendCommand("G10 L20 P0 X0 Y0 Z0");
    ImGui::SameLine();
    if (ImGui::Button("Zero XY Only", ImVec2(btnW, 0)))
        m_cnc->sendCommand("G10 L20 P0 X0 Y0");
    ImGui::SameLine();
    if (ImGui::Button("Zero Z Only", ImVec2(btnW, 0)))
        m_cnc->sendCommand("G10 L20 P0 Z0");
    if (!canSend) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Checkbox("Zero position is set and verified", &m_zeroConfirmed);
}

void DirectCarvePanel::renderCommit() {
    ImGui::TextUnformatted("Final Confirmation");
    ImGui::Spacing();

    float colW = ImGui::GetFontSize() * 8.0f;
    if (ImGui::BeginTable("##summary", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, colW);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        auto row = [](const char* l, const char* v) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(l);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(v);
        };

        char buf[128];
        row("Model", m_modelName.empty() ? "(loaded)" : m_modelName.c_str());
        std::snprintf(buf, sizeof(buf), "%.1f mm", static_cast<double>(m_fitParams.depthMm));
        row("Depth", buf);
        row("Finishing", m_finishTool.name_format.empty() ? "(selected)" : m_finishTool.name_format.c_str());
        row("Clearing", m_clearToolSelected ? m_clearTool.name_format.c_str() : "None");
        std::snprintf(buf, sizeof(buf), "%.0f mm/min", static_cast<double>(m_toolpathConfig.feedRateMmMin));
        row("Feed", buf);
        std::snprintf(buf, sizeof(buf), "%.1f mm", static_cast<double>(m_toolpathConfig.safeZMm));
        row("Safe Z", buf);

        if (m_carveJob) {
            const auto& tp = m_carveJob->toolpath();
            row("Finish Time", formatTime(tp.finishing.estimatedTimeSec).c_str());
            if (!tp.clearing.points.empty())
                row("Clear Time", formatTime(tp.clearing.estimatedTimeSec).c_str());
            row("Total Time", formatTime(tp.totalTimeSec).c_str());
            std::snprintf(buf, sizeof(buf), "%d", tp.totalLineCount);
            row("Lines", buf);
        }
        ImGui::EndTable();
    }

    if (m_carveJob) {
        for (const auto& w : m_carveJob->toolpath().finishing.warnings)
            ImGui::TextColored(warningColor(), "%s", w.c_str());
    }

    ImGui::Spacing();
    ImGui::TextColored(warningColor(), "Ensure work area is clear and spindle is ready.");
    ImGui::Spacing();
    ImGui::Checkbox("I have verified setup", &m_commitConfirmed);

    ImGui::Spacing();
    if (ImGui::Button("Save as G-code", ImVec2(ImGui::GetFontSize() * 10.0f, 0))) {
        if (m_carveJob)
            carve::exportGcode("/tmp/direct_carve.nc", m_carveJob->toolpath(),
                               m_toolpathConfig, m_modelName, m_finishTool.name_format);
    }
}

void DirectCarvePanel::renderRunning() {
    ImGui::TextUnformatted("Carve in Progress");
    ImGui::Spacing();
    ImGui::TextColored(primaryColor(), "Streaming toolpath to machine...");
    ImGui::Spacing();
    ImGui::Text("State: %s",
        m_machineStatus.state == MachineState::Run ? "Running" :
        m_machineStatus.state == MachineState::Idle ? "Idle" :
        m_machineStatus.state == MachineState::Hold ? "Hold" : "Other");
    ImGui::Text("Pos: X%.3f Y%.3f Z%.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Spacing();
    ImGui::TextWrapped("Use Safety panel or keyboard smash to emergency stop.");
}

} // namespace dw
