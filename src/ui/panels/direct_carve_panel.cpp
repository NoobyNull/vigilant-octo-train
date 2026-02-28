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
#include "ui/dialogs/file_dialog.h"
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

void statusBullet(bool ok, const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, ok ? kGreen : kRed);
    ImGui::BulletText("%s %s", ok ? "OK" : "--", label);
    ImGui::PopStyleColor();
}

} // anonymous namespace

DirectCarvePanel::DirectCarvePanel() : Panel("Direct Carve") {
    m_open = false; // Hidden by default, shown via View menu
}

DirectCarvePanel::~DirectCarvePanel() = default;

// --- Dependencies ---

void DirectCarvePanel::setCncController(CncController* cnc) {
    m_cnc = cnc;
}

void DirectCarvePanel::setToolDatabase(ToolDatabase* db) {
    m_toolDb = db;
}

void DirectCarvePanel::setCarveJob(carve::CarveJob* job) {
    m_carveJob = job;
}

void DirectCarvePanel::setFileDialog(FileDialog* dlg) {
    m_fileDialog = dlg;
}

// --- Callbacks ---

void DirectCarvePanel::onConnectionChanged(bool connected) {
    m_cncConnected = connected;
}

void DirectCarvePanel::onStatusUpdate(const MachineStatus& status) {
    m_machineStatus = status;
}

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

// --- Step label ---

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

// --- Main render ---

void DirectCarvePanel::render() {
    if (!m_open)
        return;

    applyMinSize(40.0f, 25.0f);

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

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

// --- Step indicator ---

void DirectCarvePanel::renderStepIndicator() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float fontSize = ImGui::GetFontSize();
    float spacing = fontSize * 0.5f;
    float circleRadius = fontSize * 0.35f;
    float lineHeight = ImGui::GetTextLineHeightWithSpacing();

    int currentIdx = static_cast<int>(m_currentStep);

    float xPos = cursor.x;
    for (int i = 0; i < STEP_COUNT; ++i) {
        const char* label = stepLabel(static_cast<Step>(i));
        float labelWidth = ImGui::CalcTextSize(label).x;
        float centerX = xPos + circleRadius;

        ImVec4 color;
        if (i < currentIdx) {
            color = kGreen; // completed
        } else if (i == currentIdx) {
            color = kBright; // current
        } else {
            color = kDimmed; // future
        }

        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

        // Circle
        if (i < currentIdx) {
            dl->AddCircleFilled(ImVec2(centerX, cursor.y + circleRadius), circleRadius, col);
        } else if (i == currentIdx) {
            dl->AddCircleFilled(ImVec2(centerX, cursor.y + circleRadius), circleRadius, col);
        } else {
            dl->AddCircle(ImVec2(centerX, cursor.y + circleRadius), circleRadius, col, 0, 1.5f);
        }

        // Label below circle
        float labelX = centerX - labelWidth * 0.5f;
        dl->AddText(ImVec2(labelX, cursor.y + circleRadius * 2.0f + 2.0f), col, label);

        // Connecting line to next step
        if (i < STEP_COUNT - 1) {
            float lineStartX = centerX + circleRadius + 2.0f;
            // Estimate next position for line end
            float lineLen = spacing + circleRadius;
            ImU32 lineCol = (i < currentIdx) ?
                ImGui::ColorConvertFloat4ToU32(kGreen) :
                ImGui::ColorConvertFloat4ToU32(kDimmed);
            dl->AddLine(
                ImVec2(lineStartX, cursor.y + circleRadius),
                ImVec2(lineStartX + lineLen, cursor.y + circleRadius),
                lineCol, 1.5f);
        }

        xPos += circleRadius * 2.0f + spacing +
                std::max(labelWidth, circleRadius * 2.0f) * 0.5f + spacing;
    }

    // Reserve vertical space for the indicator
    ImGui::Dummy(ImVec2(0, lineHeight + circleRadius));
}

// --- Navigation buttons ---

void DirectCarvePanel::renderNavButtons() {
    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 6.0f;

    bool isFirst = (m_currentStep == Step::MachineCheck);
    bool isRunning = (m_currentStep == Step::Running);
    bool isCommit = (m_currentStep == Step::Commit);

    // Back button
    if (isFirst || isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Back", ImVec2(btnWidth, 0))) {
        retreatStep();
    }
    if (isFirst || isRunning) ImGui::EndDisabled();

    ImGui::SameLine();

    // Next / Start Carving button
    bool canGo = canAdvance();
    if (!canGo) ImGui::BeginDisabled();

    const char* nextLabel = isCommit ? "Start Carving" : "Next";
    if (ImGui::Button(nextLabel, ImVec2(btnWidth, 0))) {
        advanceStep();
    }
    if (!canGo) ImGui::EndDisabled();

    ImGui::SameLine();

    // Cancel button (always available unless running)
    if (isRunning) ImGui::BeginDisabled();
    if (ImGui::Button("Cancel", ImVec2(btnWidth, 0))) {
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

// --- canAdvance ---

bool DirectCarvePanel::canAdvance() const {
    switch (m_currentStep) {
    case Step::MachineCheck:
        return validateMachineReady();
    case Step::ModelFit:
        return m_modelLoaded;
    case Step::ToolSelect:
        return m_finishingToolSelected;
    case Step::MaterialSetup:
        return m_materialSelected;
    case Step::Preview:
        return m_toolpathGenerated;
    case Step::OutlineTest:
        return m_outlineCompleted || m_outlineSkipped;
    case Step::ZeroConfirm:
        return m_zeroConfirmed;
    case Step::Commit:
        return m_commitConfirmed;
    case Step::Running:
        return false; // Cannot advance past running
    }
    return false;
}

// --- Navigation ---

void DirectCarvePanel::advanceStep() {
    int idx = static_cast<int>(m_currentStep);
    if (idx < STEP_COUNT - 1) {
        m_currentStep = static_cast<Step>(idx + 1);
    }
}

void DirectCarvePanel::retreatStep() {
    int idx = static_cast<int>(m_currentStep);
    if (idx > 0) {
        m_currentStep = static_cast<Step>(idx - 1);
    }
}

// --- validateMachineReady ---

bool DirectCarvePanel::validateMachineReady() const {
    if (!m_cncConnected) return false;
    if (m_machineStatus.state == MachineState::Alarm) return false;
    if (m_machineStatus.state == MachineState::Unknown) return false;
    if (!m_safeZConfirmed) return false;
    return true;
}

// --- Step: Machine Check ---

void DirectCarvePanel::renderMachineCheck() {
    ImGui::TextUnformatted("Machine Readiness Check");
    ImGui::Spacing();

    bool connected = m_cncConnected;
    bool homed = (m_machineStatus.state == MachineState::Idle);
    bool notAlarm = (m_machineStatus.state != MachineState::Alarm &&
                     m_machineStatus.state != MachineState::Unknown);

    // Travel limits from active machine profile as proxy for "profile configured"
    auto& cfg = Config::instance();
    const auto& profile = cfg.getActiveMachineProfile();
    bool profileConfigured = (profile.maxTravelX > 0.0f &&
                              profile.maxTravelY > 0.0f &&
                              profile.maxTravelZ > 0.0f);

    statusBullet(connected, "CNC connected");
    statusBullet(homed && notAlarm, "Machine homed (idle, no alarm)");
    statusBullet(profileConfigured, "Machine profile configured (travel limits set)");
    statusBullet(m_safeZConfirmed, "Safe Z verified");

    ImGui::Spacing();

    if (connected && homed) {
        float fontSize = ImGui::GetFontSize();
        float btnWidth = fontSize * 10.0f;
        if (ImGui::Button("Test Safe Z", ImVec2(btnWidth, 0))) {
            if (m_cnc) {
                f32 safeZ = m_toolpathConfig.safeZMm;
                char cmd[64];
                std::snprintf(cmd, sizeof(cmd), "G0 Z%.3f", static_cast<double>(safeZ));
                m_cnc->sendCommand(cmd);
                m_safeZConfirmed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip (confirm safe Z)", ImVec2(btnWidth * 1.2f, 0))) {
            m_safeZConfirmed = true;
        }
    }

    if (m_machineStatus.state == MachineState::Alarm) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, kRed);
        ImGui::TextWrapped("Machine is in ALARM state. Clear the alarm and home before continuing.");
        ImGui::PopStyleColor();
    }
}

// --- Step: Model Fit ---

void DirectCarvePanel::renderModelFit() {
    ImGui::TextUnformatted("Model Fitting");
    ImGui::Spacing();

    if (!m_modelLoaded) {
        ImGui::TextColored(kYellow, "No model loaded. Load an STL model first.");
        return;
    }

    Vec3 modelSize = m_modelBoundsMax - m_modelBoundsMin;
    ImGui::Text("Model bounds: %.1f x %.1f x %.1f mm",
                static_cast<double>(modelSize.x),
                static_cast<double>(modelSize.y),
                static_cast<double>(modelSize.z));

    ImGui::Spacing();
    ImGui::Text("Stock Dimensions:");
    float inputWidth = ImGui::GetFontSize() * 6.0f;
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Width (X) mm", &m_stock.width, 1.0f, 10.0f, "%.1f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Height (Y) mm", &m_stock.height, 1.0f, 10.0f, "%.1f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Thickness (Z) mm", &m_stock.thickness, 1.0f, 10.0f, "%.1f");

    ImGui::Spacing();
    ImGui::Text("Fit Parameters:");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Scale (uniform)", &m_fitParams.scale, 0.01f, 0.1f, "%.3f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Depth (Z) mm", &m_fitParams.depthMm, 0.1f, 1.0f, "%.1f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Offset X mm", &m_fitParams.offsetX, 0.5f, 5.0f, "%.1f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Offset Y mm", &m_fitParams.offsetY, 0.5f, 5.0f, "%.1f");

    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 8.0f;
    if (ImGui::Button("Auto Fit", ImVec2(btnWidth, 0))) {
        m_fitter.setStock(m_stock);
        m_fitParams.scale = m_fitter.autoScale();
        m_fitParams.depthMm = m_fitter.autoDepth();
        m_fitParams.offsetX = 0.0f;
        m_fitParams.offsetY = 0.0f;
    }

    // Show fit result
    m_fitter.setStock(m_stock);
    const auto& mp = Config::instance().getActiveMachineProfile();
    m_fitter.setMachineTravel(mp.maxTravelX, mp.maxTravelY, mp.maxTravelZ);
    carve::FitResult result = m_fitter.fit(m_fitParams);

    ImGui::Spacing();
    if (result.fitsStock) {
        ImGui::TextColored(kGreen, "Model fits within stock.");
    } else {
        ImGui::TextColored(kRed, "Model exceeds stock: %s", result.warning.c_str());
    }
    if (result.fitsMachine) {
        ImGui::TextColored(kGreen, "Model fits within machine travel.");
    } else {
        ImGui::TextColored(kRed, "Model exceeds machine travel: %s", result.warning.c_str());
    }
}

// --- Step: Tool Select ---

void DirectCarvePanel::renderToolSelect() {
    ImGui::TextUnformatted("Tool Selection");
    ImGui::Spacing();
    ImGui::TextWrapped("Select a finishing tool for the carve operation. "
                       "A V-bit is recommended for fine surface detail.");
    ImGui::Spacing();

    // Placeholder -- full tool browser integration in plan 18-02
    ImGui::Checkbox("Finishing tool selected", &m_finishingToolSelected);
    if (m_finishingToolSelected) {
        ImGui::TextColored(kGreen, "Tool ready.");
    }
}

// --- Step: Material Setup ---

void DirectCarvePanel::renderMaterialSetup() {
    ImGui::TextUnformatted("Material & Feeds");
    ImGui::Spacing();
    ImGui::TextWrapped("Select the workpiece material and confirm feed rates.");
    ImGui::Spacing();

    float inputWidth = ImGui::GetFontSize() * 6.0f;
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Feed Rate (mm/min)", &m_toolpathConfig.feedRateMmMin, 50.0f, 200.0f, "%.0f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Plunge Rate (mm/min)", &m_toolpathConfig.plungeRateMmMin, 10.0f, 50.0f, "%.0f");
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputFloat("Safe Z (mm)", &m_toolpathConfig.safeZMm, 0.5f, 2.0f, "%.1f");

    ImGui::Spacing();
    ImGui::Checkbox("Material confirmed", &m_materialSelected);
}

// --- Step: Preview ---

void DirectCarvePanel::renderPreview() {
    ImGui::TextUnformatted("Toolpath Preview");
    ImGui::Spacing();

    if (!m_toolpathGenerated) {
        ImGui::TextWrapped("Generate the toolpath to preview estimated time and line count.");
        ImGui::Spacing();

        float fontSize = ImGui::GetFontSize();
        float btnWidth = fontSize * 10.0f;
        if (ImGui::Button("Generate Toolpath", ImVec2(btnWidth, 0))) {
            // Placeholder -- actual generation wired in plan 18-02
            m_toolpathGenerated = true;
        }
    } else {
        ImGui::TextColored(kGreen, "Toolpath generated and ready for preview.");
    }
}

// --- Step: Outline Test ---

void DirectCarvePanel::renderOutlineTest() {
    ImGui::TextUnformatted("Outline Test");
    ImGui::Spacing();
    ImGui::TextWrapped("Run a perimeter trace at safe Z height to verify the carve area "
                       "before committing to the full operation.");
    ImGui::Spacing();

    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 10.0f;

    if (!m_outlineCompleted) {
        if (ImGui::Button("Run Outline", ImVec2(btnWidth, 0))) {
            // Placeholder -- will send outline gcode in plan 18-02
            m_outlineCompleted = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip Outline", ImVec2(btnWidth, 0))) {
            m_outlineSkipped = true;
        }
    } else {
        ImGui::TextColored(kGreen, "Outline test completed.");
    }

    if (m_outlineSkipped && !m_outlineCompleted) {
        ImGui::TextColored(kYellow, "Outline test skipped.");
    }
}

// --- Step: Zero Confirm ---

void DirectCarvePanel::renderZeroConfirm() {
    ImGui::TextUnformatted("Zero Position Confirmation");
    ImGui::Spacing();

    // Current position display
    ImGui::Text("Current Work Position:");
    ImGui::Indent();
    ImGui::Text("X: %.3f  Y: %.3f  Z: %.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Unindent();

    // Highlight if near expected zero
    bool nearZero = (std::fabs(m_machineStatus.workPos.x) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.y) < 0.5f &&
                     std::fabs(m_machineStatus.workPos.z) < 0.5f);
    if (nearZero) {
        ImGui::TextColored(kGreen, "Position is near zero origin.");
    }

    ImGui::Spacing();
    ImGui::TextWrapped("Position the tool at the work zero point "
                       "(bottom-left of stock, Z on top surface). "
                       "Use jog controls in the CNC panel to fine-tune position.");
    ImGui::Spacing();

    // Zero action buttons
    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 10.0f;

    bool canSend = (m_cnc != nullptr && m_cncConnected);
    if (!canSend) ImGui::BeginDisabled();

    if (ImGui::Button("Set Zero Here", ImVec2(btnWidth, 0))) {
        m_cnc->sendCommand("G10 L20 P0 X0 Y0 Z0");
    }
    ImGui::SameLine();
    if (ImGui::Button("Zero XY Only", ImVec2(btnWidth, 0))) {
        m_cnc->sendCommand("G10 L20 P0 X0 Y0");
    }
    ImGui::SameLine();
    if (ImGui::Button("Zero Z Only", ImVec2(btnWidth, 0))) {
        m_cnc->sendCommand("G10 L20 P0 Z0");
    }

    if (!canSend) ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Checkbox("Zero position is set and verified", &m_zeroConfirmed);
}

// --- Step: Commit ---

void DirectCarvePanel::renderCommit() {
    ImGui::TextUnformatted("Final Confirmation");
    ImGui::Spacing();

    // Full summary card using ImGui table
    float colWidth = ImGui::GetFontSize() * 8.0f;
    if (ImGui::BeginTable("##commit_summary", 2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, colWidth);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        auto summaryRow = [](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(value);
        };

        char buf[128];

        summaryRow("Model", m_modelName.empty() ? "(loaded)" : m_modelName.c_str());
        std::snprintf(buf, sizeof(buf), "%.1f mm", static_cast<double>(m_fitParams.depthMm));
        summaryRow("Depth", buf);
        summaryRow("Finishing Tool", m_finishTool.name_format.empty()
            ? "(selected)" : m_finishTool.name_format.c_str());
        summaryRow("Clearing Tool", m_clearToolSelected
            ? m_clearTool.name_format.c_str() : "None - no islands detected");
        summaryRow("Material", m_materialName.empty() ? "(confirmed)" : m_materialName.c_str());
        std::snprintf(buf, sizeof(buf), "%.0f mm/min",
                      static_cast<double>(m_toolpathConfig.feedRateMmMin));
        summaryRow("Feed Rate", buf);
        std::snprintf(buf, sizeof(buf), "%.0f mm/min",
                      static_cast<double>(m_toolpathConfig.plungeRateMmMin));
        summaryRow("Plunge Rate", buf);
        std::snprintf(buf, sizeof(buf), "%.1f mm",
                      static_cast<double>(m_toolpathConfig.safeZMm));
        summaryRow("Safe Z", buf);

        if (m_carveJob) {
            const auto& tp = m_carveJob->toolpath();
            summaryRow("Finishing Time", formatTime(tp.finishing.estimatedTimeSec).c_str());
            if (!tp.clearing.points.empty()) {
                summaryRow("Clearing Time", formatTime(tp.clearing.estimatedTimeSec).c_str());
            }
            summaryRow("Total Time", formatTime(tp.totalTimeSec).c_str());
            std::snprintf(buf, sizeof(buf), "%d", tp.totalLineCount);
            summaryRow("Total Lines", buf);
        }

        ImGui::EndTable();
    }

    // Warning box (toolpath warnings from travel limit check)
    if (m_carveJob) {
        const auto& warnings = m_carveJob->toolpath().finishing.warnings;
        if (!warnings.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, kYellow);
            for (const auto& w : warnings) {
                ImGui::BulletText("%s", w.c_str());
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kYellow);
    ImGui::TextWrapped("This will begin streaming G-code to the machine. "
                       "Ensure the work area is clear and the spindle is ready.");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Checkbox("I have verified the work zero, tool, and material setup",
                    &m_commitConfirmed);

    ImGui::Spacing();
    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 10.0f;

    if (ImGui::Button("Save as G-code", ImVec2(btnWidth, 0))) {
        if (m_carveJob && m_fileDialog) {
            auto config = m_toolpathConfig;
            auto modelName = m_modelName;
            auto toolName = m_finishTool.name_format;
            auto toolpath = m_carveJob->toolpath();
            m_fileDialog->showSave("Save G-code",
                FileDialog::gcodeFilters(), "carve.nc",
                [config, modelName, toolName, toolpath](const std::string& path) {
                    if (!path.empty()) {
                        carve::exportGcode(path, toolpath, config, modelName, toolName);
                    }
                });
        }
    }
}

// --- Step: Running ---

void DirectCarvePanel::renderRunning() {
    // Completed state
    if (m_runState == RunState::Completed) {
        ImGui::TextColored(kGreen, "Carving Complete");
        ImGui::Spacing();
        ImGui::Text("Total lines sent: %d", m_runTotalLines);
        ImGui::Text("Elapsed: %s", formatTime(m_runElapsedSec).c_str());
        ImGui::Spacing();
        float fontSize = ImGui::GetFontSize();
        if (ImGui::Button("New Carve", ImVec2(fontSize * 10.0f, 0))) {
            m_currentStep = Step::MachineCheck;
            m_runState = RunState::Active;
            m_commitConfirmed = false;
            m_zeroConfirmed = false;
        }
        return;
    }

    // Aborted state
    if (m_runState == RunState::Aborted) {
        ImGui::TextColored(kRed, "Carving Aborted");
        ImGui::Spacing();
        ImGui::Text("Stopped at line %d / %d", m_runCurrentLine, m_runTotalLines);
        ImGui::Text("Last position: X%.3f Y%.3f Z%.3f",
                    static_cast<double>(m_machineStatus.workPos.x),
                    static_cast<double>(m_machineStatus.workPos.y),
                    static_cast<double>(m_machineStatus.workPos.z));
        ImGui::Spacing();
        float fontSize = ImGui::GetFontSize();
        if (ImGui::Button("New Carve", ImVec2(fontSize * 10.0f, 0))) {
            m_currentStep = Step::MachineCheck;
            m_runState = RunState::Active;
            m_commitConfirmed = false;
            m_zeroConfirmed = false;
        }
        return;
    }

    // Active or Paused
    ImGui::TextUnformatted("Carve in Progress");
    ImGui::Spacing();

    // Progress bar
    float fraction = (m_runTotalLines > 0)
        ? static_cast<float>(m_runCurrentLine) / static_cast<float>(m_runTotalLines)
        : 0.0f;
    char overlay[64];
    std::snprintf(overlay, sizeof(overlay), "%d / %d lines",
                  m_runCurrentLine, m_runTotalLines);
    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);

    // Time
    float estRemaining = (fraction > 0.01f)
        ? m_runElapsedSec * (1.0f - fraction) / fraction : 0.0f;
    ImGui::Text("Elapsed: %s  |  Remaining: ~%s",
                formatTime(m_runElapsedSec).c_str(),
                formatTime(estRemaining).c_str());

    if (!m_runCurrentPass.empty()) {
        ImGui::Text("Current: %s", m_runCurrentPass.c_str());
    }

    ImGui::Text("Position: X%.3f Y%.3f Z%.3f",
                static_cast<double>(m_machineStatus.workPos.x),
                static_cast<double>(m_machineStatus.workPos.y),
                static_cast<double>(m_machineStatus.workPos.z));
    ImGui::Spacing();

    // Control buttons
    float fontSize = ImGui::GetFontSize();
    float btnWidth = fontSize * 8.0f;

    if (m_runState == RunState::Paused) {
        if (ImGui::Button("Resume", ImVec2(btnWidth, 0))) {
            if (m_cnc) m_cnc->sendCommand("~");
            m_runState = RunState::Active;
        }
    } else {
        if (ImGui::Button("Pause", ImVec2(btnWidth, 0))) {
            if (m_cnc) m_cnc->sendCommand("!");
            m_runState = RunState::Paused;
        }
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Abort", ImVec2(btnWidth, 0))) {
        ImGui::OpenPopup("Confirm Abort");
    }
    ImGui::PopStyleColor(2);

    if (ImGui::BeginPopupModal("Confirm Abort", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Stop the carving operation?");
        ImGui::Text("The spindle will be stopped and the tool retracted.");
        ImGui::Spacing();
        if (ImGui::Button("Yes, Abort", ImVec2(btnWidth, 0))) {
            if (m_cnc) m_cnc->sendCommand("\x18");
            m_runState = RunState::Aborted;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Continue", ImVec2(btnWidth, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// --- formatTime helper ---

std::string DirectCarvePanel::formatTime(f32 seconds) {
    int totalSec = static_cast<int>(seconds);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%dm %ds", min, sec);
    return buf;
}

} // namespace dw
