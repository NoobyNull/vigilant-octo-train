#include "gcode_panel.h"

#include <algorithm>
#include <cstdio>
#include <numeric>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/database/gcode_repository.h"
#include "../../core/cnc/cnc_controller.h"
#include "../../core/cnc/preflight_check.h"
#include "../../core/project/project.h"
#include "../../core/cnc/serial_port.h"
#include "../../core/utils/file_utils.h"
#include "../../render/gl_utils.h"
#include "../dialogs/file_dialog.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

namespace {
struct LongPressButton {
    bool holding = false;
    float holdTime = 0.0f;

    bool render(const char* label, ImVec2 size, float requiredMs, bool enabled) {
        if (!enabled) ImGui::BeginDisabled();
        ImGui::Button(label, size);
        if (!enabled) ImGui::EndDisabled();
        if (!enabled) { holding = false; holdTime = 0.0f; return false; }

        bool isHeld = ImGui::IsItemActive();
        if (isHeld) {
            holdTime += ImGui::GetIO().DeltaTime * 1000.0f;
            float progress = holdTime / requiredMs;
            if (progress > 1.0f) progress = 1.0f;
            ImVec2 rmin = ImGui::GetItemRectMin();
            ImVec2 rmax = ImGui::GetItemRectMax();
            ImVec2 fillMax = {rmin.x + (rmax.x - rmin.x) * progress, rmax.y};
            ImGui::GetWindowDrawList()->AddRectFilled(
                rmin, fillMax, IM_COL32(255, 255, 255, 40), 3.0f);
            holding = true;
        } else {
            if (holding) { holding = false; holdTime = 0.0f; }
        }
        if (holdTime >= requiredMs) {
            holding = false;
            holdTime = 0.0f;
            return true;
        }
        return false;
    }
};

static LongPressButton s_startLongPress;
} // namespace

constexpr int GCodePanel::BAUD_RATES[];

GCodePanel::GCodePanel() : Panel("G-code") {
    m_pathRenderer.initialize();
    m_pathCamera.reset();
    // Start with top-down view (familiar 2D-like default)
    m_pathCamera.setYaw(0.0f);
    m_pathCamera.setPitch(89.0f);
    m_availablePorts = listSerialPorts();
}

GCodePanel::~GCodePanel() {
    destroyPathGeometry();
}

void GCodePanel::render() {
    if (!m_open)
        return;

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderModeTabs();
        renderToolbar();

        if (hasGCode()) {
            ImGui::Separator();

            // Mode-specific top sections
            if (m_mode == GCodePanelMode::Send) {
                renderConnectionBar();
                if (m_cncConnected) {
                    renderMachineStatus();
                    renderPlaybackControls();
                    renderProgressBar();
                    renderFeedOverride();
                }
            } else if (m_mode == GCodePanelMode::View) {
                renderSimulationControls();
            }

            ImGui::Separator();

            // Main content area: stats + path view
            float availWidth = ImGui::GetContentRegionAvail().x;

            if (availWidth < 420.0f) {
                ImGui::BeginChild("Stats", ImVec2(0, 250), true);
                renderStatistics();
                ImGui::EndChild();

                ImGui::BeginChild("PathView", ImVec2(0, 0), true);
                renderPathView();
                ImGui::EndChild();
            } else {
                float statsWidth = std::min(250.0f, availWidth * 0.4f);

                ImGui::BeginChild("Stats", ImVec2(statsWidth, 0), true);
                renderStatistics();

                // Console below stats in sender mode
                if (m_mode == GCodePanelMode::Send) {
                    ImGui::Separator();
                    renderConsole();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("PathView", ImVec2(availWidth - statsWidth - 8, 0), true);
                renderPathView();
                ImGui::EndChild();
            }
        } else {
            ImGui::TextDisabled("No G-code loaded");
            ImGui::TextDisabled("Open a G-code file to view");

            if (m_mode == GCodePanelMode::Send) {
                ImGui::Separator();
                renderConnectionBar();
            }
        }
    }
    ImGui::End();
}

void GCodePanel::renderModeTabs() {
    if (ImGui::BeginTabBar("##GCodeModeTabs")) {
        if (ImGui::BeginTabItem("View")) {
            m_mode = GCodePanelMode::View;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Send")) {
            m_mode = GCodePanelMode::Send;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

bool GCodePanel::loadFile(const std::string& path) {
    auto content = file::readText(path);
    if (!content) {
        ToastManager::instance().show(ToastType::Error,
                                      "File Read Error",
                                      "Could not read G-code file: " + path);
        return false;
    }

    gcode::Parser parser;
    m_program = parser.parse(*content);

    if (!m_program.commands.empty()) {
        m_filePath = path;

        gcode::Analyzer analyzer;
        analyzer.setMachineProfile(Config::instance().getActiveMachineProfile());
        m_stats = analyzer.analyze(m_program);

        if (!m_program.path.empty()) {
            m_maxLayer = m_program.boundsMax.z;
            m_currentLayer = m_maxLayer;
        }

        // Build precomputed segment times from trapezoidal planner (convert min->sec)
        m_segmentTimes.resize(m_stats.segmentTimes.size());
        for (size_t i = 0; i < m_stats.segmentTimes.size(); ++i)
            m_segmentTimes[i] = m_stats.segmentTimes[i] * 60.0f;

        // Build cumulative sum for O(log n) binary search
        m_segmentTimeCumulative.resize(m_segmentTimes.size());
        if (!m_segmentTimes.empty()) {
            std::partial_sum(m_segmentTimes.begin(), m_segmentTimes.end(),
                             m_segmentTimeCumulative.begin());
            m_simTotalTime = m_segmentTimeCumulative.back();
        } else {
            m_simTotalTime = 0.0f;
        }

        m_pathDirty = true;
        m_needsCameraFit = true;

        m_currentGCodeId = -1;
        if (m_gcodeRepo) {
            std::string filename = path;
            size_t pos = filename.find_last_of("/\\");
            if (pos != std::string::npos)
                filename = filename.substr(pos + 1);
            auto existing = m_gcodeRepo->findByName(filename);
            if (!existing.empty())
                m_currentGCodeId = existing.front().id;
        }

        return true;
    }

    ToastManager::instance().show(ToastType::Warning,
                                  "Empty G-code",
                                  "File contains no valid G-code commands");
    return false;
}

void GCodePanel::clear() {
    m_program = gcode::Program{};
    m_stats = gcode::Statistics{};
    m_filePath.clear();
    m_currentLayer = 0.0f;
    m_maxLayer = 100.0f;
    destroyPathGeometry();
    m_pathDirty = true;
    m_currentGCodeId = -1;
    m_simState = SimState::Stopped;
    m_simTime = 0.0f;
    m_simSegmentIndex = 0;
    m_simSegmentProgress = 0.0f;
    m_segmentTimes.clear();
    m_segmentTimeCumulative.clear();
    m_lastAckedLine = -1;
    m_streamProgress = {};
}

void GCodePanel::renderToolbar() {
    if (ImGui::Button("Open")) {
        if (m_fileDialog) {
            m_fileDialog->showOpen("Open G-code",
                                   FileDialog::gcodeFilters(),
                                   [this](const std::string& path) {
                                       if (!path.empty())
                                           loadFile(path);
                                   });
        }
    }

    if (hasGCode()) {
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            clear();
        }

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        float availWidth = ImGui::GetContentRegionAvail().x;
        if (availWidth < 350.0f)
            ImGui::NewLine();

        if (ImGui::Checkbox("Rapid", &m_showRapid))
            m_pathDirty = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Cut", &m_showCut))
            m_pathDirty = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Plunge", &m_showPlunge))
            m_pathDirty = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Retract", &m_showRetract))
            m_pathDirty = true;

        if (m_projectManager && m_projectManager->currentProject() && m_currentGCodeId > 0) {
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();
            bool alreadyInProject = m_gcodeRepo &&
                m_gcodeRepo->isInProject(m_projectManager->currentProject()->id(), m_currentGCodeId);
            if (alreadyInProject) {
                ImGui::BeginDisabled();
                ImGui::Button("In Project");
                ImGui::EndDisabled();
            } else {
                if (ImGui::Button("Add to Project")) {
                    if (m_gcodeRepo) {
                        m_gcodeRepo->addToProject(
                            m_projectManager->currentProject()->id(), m_currentGCodeId);
                        ToastManager::instance().show(ToastType::Success,
                            "Added", "G-code added to project");
                    }
                }
            }
        }
    }
}

void GCodePanel::renderStatistics() {
    if (!hasGCode())
        return;

    renderMachineProfileSelector();
    ImGui::Separator();

    ImGui::Text("%s Statistics", Icons::Info);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        std::string filename = m_filePath;
        size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos)
            filename = filename.substr(pos + 1);
        ImGui::TextWrapped("File: %s", filename.c_str());
        ImGui::Text("Lines: %d", m_stats.lineCount);
        ImGui::Text("Commands: %d", m_stats.commandCount);
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Time", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        int hours = static_cast<int>(m_stats.estimatedTime / 60);
        int mins = static_cast<int>(m_stats.estimatedTime) % 60;
        if (hours > 0)
            ImGui::Text("Estimated: %dh %dm", hours, mins);
        else
            ImGui::Text("Estimated: %dm", mins);
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Distance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Text("Total: %.1f mm", static_cast<double>(m_stats.totalPathLength));
        ImGui::Text("Rapid: %.1f mm", static_cast<double>(m_stats.rapidPathLength));
        ImGui::Text("Cutting: %.1f mm", static_cast<double>(m_stats.cuttingPathLength));
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        const auto& bMin = m_stats.boundsMin;
        const auto& bMax = m_stats.boundsMax;
        ImGui::Text("X: %.1f - %.1f", static_cast<double>(bMin.x), static_cast<double>(bMax.x));
        ImGui::Text("Y: %.1f - %.1f", static_cast<double>(bMin.y), static_cast<double>(bMax.y));
        ImGui::Text("Z: %.1f - %.1f", static_cast<double>(bMin.z), static_cast<double>(bMax.z));
        double sizeX = static_cast<double>(bMax.x - bMin.x);
        double sizeY = static_cast<double>(bMax.y - bMin.y);
        double sizeZ = static_cast<double>(bMax.z - bMin.z);
        ImGui::Text("Size: %.1f x %.1f x %.1f", sizeX, sizeY, sizeZ);
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Toolpath", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Text("Segments: %zu", m_program.path.size());
        ImGui::Text("Tool Changes: %d", m_stats.toolChangeCount);
        ImGui::Unindent();
    }
}

// --- Machine profile selector ---

void GCodePanel::renderMachineProfileSelector() {
    auto& config = Config::instance();
    const auto& profiles = config.getMachineProfiles();
    int activeIdx = config.getActiveMachineProfileIndex();

    ImGui::Text("%s Machine", Icons::Settings);
    ImGui::SetNextItemWidth(-1);
    const char* preview = profiles[static_cast<size_t>(activeIdx)].name.c_str();
    if (ImGui::BeginCombo("##MachineProfile", preview)) {
        for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
            bool selected = (i == activeIdx);
            if (ImGui::Selectable(profiles[static_cast<size_t>(i)].name.c_str(), selected)) {
                config.setActiveMachineProfileIndex(i);
                reanalyze();
                config.save();
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Edit Machine Profile")) {
        if (!m_profileDialog.isOpen()) {
            m_profileDialog.setOnProfileChanged([this]() { reanalyze(); });
            m_profileDialog.open();
        } else {
            m_profileDialog.close();
        }
    }

    // Render the dialog window (it's a floating window, renders itself)
    m_profileDialog.render();
}

void GCodePanel::reanalyze() {
    if (m_program.commands.empty())
        return;

    gcode::Analyzer analyzer;
    analyzer.setMachineProfile(Config::instance().getActiveMachineProfile());
    m_stats = analyzer.analyze(m_program);

    // Rebuild segment times (min->sec)
    m_segmentTimes.resize(m_stats.segmentTimes.size());
    for (size_t i = 0; i < m_stats.segmentTimes.size(); ++i)
        m_segmentTimes[i] = m_stats.segmentTimes[i] * 60.0f;

    m_segmentTimeCumulative.resize(m_segmentTimes.size());
    if (!m_segmentTimes.empty()) {
        std::partial_sum(m_segmentTimes.begin(), m_segmentTimes.end(),
                         m_segmentTimeCumulative.begin());
        m_simTotalTime = m_segmentTimeCumulative.back();
    } else {
        m_simTotalTime = 0.0f;
    }

    // Reset simulation state
    m_simState = SimState::Stopped;
    m_simTime = 0.0f;
    m_simSegmentIndex = 0;
    m_simSegmentProgress = 0.0f;

    m_pathDirty = true;
}

// --- Connection bar (Send mode) ---

void GCodePanel::renderConnectionBar() {
    ImGui::Text("%s Connection", Icons::Plug);

    // Port combo
    if (ImGui::Button(Icons::Refresh)) {
        m_availablePorts = listSerialPorts();
        if (m_selectedPort >= m_availablePorts.size())
            m_selectedPort = 0;
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(150);
    if (m_availablePorts.empty()) {
        ImGui::BeginDisabled();
        if (ImGui::BeginCombo("##Port", "No ports found")) {
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
    } else {
        const char* portPreview = m_availablePorts[m_selectedPort].c_str();
        if (ImGui::BeginCombo("##Port", portPreview)) {
            for (size_t i = 0; i < m_availablePorts.size(); ++i) {
                bool selected = (i == m_selectedPort);
                if (ImGui::Selectable(m_availablePorts[i].c_str(), selected))
                    m_selectedPort = i;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    // Baud combo
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    char baudLabel[16];
    snprintf(baudLabel, sizeof(baudLabel), "%d", BAUD_RATES[m_selectedBaud]);
    if (ImGui::BeginCombo("##Baud", baudLabel)) {
        for (size_t i = 0; i < static_cast<size_t>(NUM_BAUD_RATES); ++i) {
            char label[16];
            snprintf(label, sizeof(label), "%d", BAUD_RATES[i]);
            bool selected = (i == m_selectedBaud);
            if (ImGui::Selectable(label, selected))
                m_selectedBaud = i;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Connect / Disconnect button
    ImGui::SameLine();
    if (m_cncConnected) {
        if (ImGui::Button("Disconnect")) {
            if (m_cnc)
                m_cnc->disconnect();
        }
    } else {
        bool canConnect = m_cnc && !m_availablePorts.empty();
        if (!canConnect)
            ImGui::BeginDisabled();
        if (ImGui::Button("Connect")) {
            if (!m_cnc->connect(m_availablePorts[m_selectedPort], BAUD_RATES[m_selectedBaud])) {
                addConsoleLine("Failed to open " + m_availablePorts[m_selectedPort], ConsoleLine::Error);
            } else {
                addConsoleLine("Connecting to " + m_availablePorts[m_selectedPort] + "...", ConsoleLine::Info);
            }
        }
        if (!canConnect)
            ImGui::EndDisabled();
    }

    if (m_cncConnected) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "%s", m_cncVersion.c_str());
    }
}

// --- Machine status display (Send mode, connected) ---

void GCodePanel::renderMachineStatus() {
    // State badge with color coding
    const char* stateText = "Unknown";
    ImVec4 stateColor(0.5f, 0.5f, 0.5f, 1.0f);

    switch (m_machineStatus.state) {
    case MachineState::Idle:
        stateText = "IDLE";
        stateColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        break;
    case MachineState::Run:
        stateText = "RUN";
        stateColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
        break;
    case MachineState::Hold:
        stateText = "HOLD";
        stateColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        break;
    case MachineState::Alarm:
        stateText = "ALARM";
        stateColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
        break;
    case MachineState::Home:
        stateText = "HOME";
        stateColor = ImVec4(0.6f, 0.4f, 1.0f, 1.0f);
        break;
    default: break;
    }

    ImGui::TextColored(stateColor, "%s", stateText);
    ImGui::SameLine(0, 20);

    const auto& mp = m_machineStatus.machinePos;
    const auto& wp = m_machineStatus.workPos;
    ImGui::Text("MPos: %.3f, %.3f, %.3f",
                static_cast<double>(mp.x), static_cast<double>(mp.y), static_cast<double>(mp.z));
    ImGui::Text("WPos: %.3f, %.3f, %.3f",
                static_cast<double>(wp.x), static_cast<double>(wp.y), static_cast<double>(wp.z));
    ImGui::Text("Feed: %.0f mm/min  Spindle: %.0f RPM",
                static_cast<double>(m_machineStatus.feedRate),
                static_cast<double>(m_machineStatus.spindleSpeed));

    // Unlock button for alarm state
    if (m_machineStatus.state == MachineState::Alarm) {
        ImGui::SameLine();
        if (ImGui::Button("Unlock ($X)")) {
            if (m_cnc)
                m_cnc->unlock();
        }
    }
}

// --- Playback controls (Send mode) ---

void GCodePanel::renderPlaybackControls() {
    if (!hasGCode())
        return;

    bool isStreaming = m_cnc && m_cnc->isStreaming();

    if (!isStreaming) {
        // Start button — long-press when safety enabled
        bool canStart = m_cncConnected &&
                        (m_machineStatus.state == MachineState::Idle);
        auto& cfg = Config::instance();
        if (cfg.getSafetyLongPressEnabled()) {
            float durationMs = static_cast<float>(cfg.getSafetyLongPressDurationMs());
            if (s_startLongPress.render("Hold to Start", ImVec2(0, 0), durationMs, canStart)) {
                buildSendProgram();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Hold for %.1fs to start streaming", static_cast<double>(durationMs / 1000.0f));
            }
        } else {
            if (!canStart)
                ImGui::BeginDisabled();
            if (ImGui::Button("Start")) {
                buildSendProgram();
            }
            if (!canStart)
                ImGui::EndDisabled();
        }
    } else {
        // Pause / Resume
        if (ImGui::Button("Hold")) {
            if (m_cnc)
                m_cnc->feedHold();
        }
        ImGui::SameLine();
        if (ImGui::Button("Resume")) {
            if (m_cnc)
                m_cnc->cycleStart();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            if (m_cnc)
                m_cnc->stopStream();
        }
    }

    ImGui::SameLine();
    // Soft reset (emergency)
    if (ImGui::Button("Reset")) {
        if (m_cnc)
            m_cnc->softReset();
    }
}

// --- Progress bar ---

void GCodePanel::renderProgressBar() {
    if (m_streamProgress.totalLines <= 0)
        return;

    float fraction = static_cast<float>(m_streamProgress.ackedLines) /
                     static_cast<float>(m_streamProgress.totalLines);

    // ETA from ack rate
    float elapsed = m_streamProgress.elapsedSeconds;
    float etaStr = 0.0f;
    if (m_streamProgress.ackedLines > 0 && fraction < 1.0f) {
        float rate = static_cast<float>(m_streamProgress.ackedLines) / elapsed;
        float remaining = static_cast<float>(m_streamProgress.totalLines - m_streamProgress.ackedLines);
        etaStr = remaining / rate;
    }

    char overlay[128];
    int etaMin = static_cast<int>(etaStr / 60.0f);
    int etaSec = static_cast<int>(etaStr) % 60;
    snprintf(overlay, sizeof(overlay), "%d / %d  (%.0f%%)  ETA: %d:%02d",
             m_streamProgress.ackedLines, m_streamProgress.totalLines,
             static_cast<double>(fraction * 100.0f), etaMin, etaSec);

    ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);

    if (m_streamProgress.errorCount > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Errors: %d",
                           m_streamProgress.errorCount);
    }
}

// --- Feed override ---

void GCodePanel::renderFeedOverride() {
    ImGui::Text("Feed Override:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderInt("##FeedOvr", &m_feedOverridePercent, 10, 200, "%d%%")) {
        if (m_cnc)
            m_cnc->setFeedOverride(m_feedOverridePercent);
    }
    ImGui::SameLine();
    if (ImGui::Button("100%")) {
        m_feedOverridePercent = 100;
        if (m_cnc)
            m_cnc->setFeedOverride(100);
    }

    // Rapid override buttons
    ImGui::SameLine(0, 20);
    ImGui::Text("Rapid:");
    ImGui::SameLine();
    if (ImGui::SmallButton("25%")) {
        if (m_cnc) m_cnc->setRapidOverride(25);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("50%")) {
        if (m_cnc) m_cnc->setRapidOverride(50);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("R100%")) {
        if (m_cnc) m_cnc->setRapidOverride(100);
    }
}

// --- Console ---

void GCodePanel::renderConsole() {
    if (!ImGui::CollapsingHeader("Console"))
        return;

    float consoleHeight = ImGui::GetContentRegionAvail().y;
    if (consoleHeight < 50.0f)
        consoleHeight = 100.0f;

    ImGui::BeginChild("##Console", ImVec2(0, consoleHeight), false);
    for (const auto& line : m_consoleLines) {
        ImVec4 color(0.7f, 0.7f, 0.7f, 1.0f);
        switch (line.type) {
        case ConsoleLine::Sent: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
        case ConsoleLine::Ok: color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f); break;
        case ConsoleLine::Error: color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
        case ConsoleLine::Status: color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f); break;
        case ConsoleLine::Info: color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break;
        }
        ImGui::TextColored(color, "%s", line.text.c_str());
    }
    if (m_consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

// --- Simulation controls ---

void GCodePanel::renderSimulationControls() {
    if (!hasGCode())
        return;

    // Play / Pause / Reset
    if (m_simState == SimState::Stopped || m_simState == SimState::Paused) {
        if (ImGui::Button("Play")) {
            m_simState = SimState::Playing;
        }
    } else {
        if (ImGui::Button("Pause")) {
            m_simState = SimState::Paused;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        m_simState = SimState::Stopped;
        m_simTime = 0.0f;
        m_simSegmentIndex = 0;
        m_simSegmentProgress = 0.0f;
    }

    // Speed selector
    ImGui::SameLine(0, 20);
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    static const float speeds[] = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
    static const char* speedLabels[] = {"0.5x", "1x", "2x", "5x", "10x"};
    int currentSpeedIdx = 1;
    for (int i = 0; i < 5; ++i) {
        if (m_simSpeed == speeds[i])
            currentSpeedIdx = i;
    }
    if (ImGui::BeginCombo("##SimSpeed", speedLabels[currentSpeedIdx])) {
        for (int i = 0; i < 5; ++i) {
            if (ImGui::Selectable(speedLabels[i], i == currentSpeedIdx))
                m_simSpeed = speeds[i];
        }
        ImGui::EndCombo();
    }

    // Scrub slider
    ImGui::Text("Progress:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    float scrubFrac = (m_simTotalTime > 0.0f) ? (m_simTime / m_simTotalTime) : 0.0f;
    if (ImGui::SliderFloat("##SimScrub", &scrubFrac, 0.0f, 1.0f, "%.1f%%")) {
        m_simTime = scrubFrac * m_simTotalTime;
        // O(log n) binary search to find segment from time
        if (!m_segmentTimeCumulative.empty()) {
            auto it = std::lower_bound(m_segmentTimeCumulative.begin(),
                                        m_segmentTimeCumulative.end(), m_simTime);
            size_t idx = static_cast<size_t>(it - m_segmentTimeCumulative.begin());
            if (idx >= m_program.path.size())
                idx = m_program.path.size() - 1;
            float segStart = (idx > 0) ? m_segmentTimeCumulative[idx - 1] : 0.0f;
            float segDur = m_segmentTimes[idx];
            m_simSegmentIndex = idx;
            m_simSegmentProgress = (segDur > 0.0f) ? (m_simTime - segStart) / segDur : 0.0f;
        } else {
            m_simSegmentIndex = 0;
            m_simSegmentProgress = 0.0f;
        }
    }
}

void GCodePanel::updateSimulation(float dt) {
    if (m_simState != SimState::Playing || m_program.path.empty())
        return;

    m_simTime += dt * m_simSpeed;

    if (m_segmentTimeCumulative.empty()) {
        m_simState = SimState::Stopped;
        return;
    }

    if (m_simTime >= m_simTotalTime) {
        // Past end - stop
        m_simSegmentIndex = m_program.path.size();
        m_simSegmentProgress = 0.0f;
        m_simState = SimState::Stopped;
        return;
    }

    // O(log n) binary search on cumulative time array
    auto it = std::lower_bound(m_segmentTimeCumulative.begin(),
                                m_segmentTimeCumulative.end(), m_simTime);
    size_t idx = static_cast<size_t>(it - m_segmentTimeCumulative.begin());
    if (idx >= m_program.path.size())
        idx = m_program.path.size() - 1;

    float segStart = (idx > 0) ? m_segmentTimeCumulative[idx - 1] : 0.0f;
    float segDur = m_segmentTimes[idx];
    m_simSegmentIndex = idx;
    m_simSegmentProgress = (segDur > 0.0f) ? (m_simTime - segStart) / segDur : 0.0f;
}

// --- 3D Path Geometry ---

void GCodePanel::buildPathGeometry() {
    destroyPathGeometry();

    if (m_program.path.empty()) {
        m_pathDirty = false;
        return;
    }

    // Classify segments into 4 groups and collect vertices
    std::vector<f32> rapidVerts;
    std::vector<f32> cutVerts;
    std::vector<f32> plungeVerts;
    std::vector<f32> retractVerts;

    for (const auto& seg : m_program.path) {
        // Z-clipping: skip segments above threshold
        if (seg.end.z > m_currentLayer)
            continue;

        // Classify
        std::vector<f32>* target = nullptr;

        if (seg.isRapid) {
            if (!m_showRapid) continue;
            target = &rapidVerts;
        } else {
            float dz = seg.end.z - seg.start.z;
            float dxy2 = (seg.end.x - seg.start.x) * (seg.end.x - seg.start.x) +
                          (seg.end.y - seg.start.y) * (seg.end.y - seg.start.y);
            bool zDominant = (dz * dz) > dxy2 * 0.25f;

            if (dz < -0.001f && zDominant) {
                if (!m_showPlunge) continue;
                target = &plungeVerts;
            } else if (dz > 0.001f && zDominant) {
                if (!m_showRetract) continue;
                target = &retractVerts;
            } else {
                if (!m_showCut) continue;
                target = &cutVerts;
            }
        }

        // Add start + end (2 vertices, 3 floats each)
        // Swap Y↔Z: G-code uses Z-up, renderer uses Y-up
        target->push_back(seg.start.x);
        target->push_back(seg.start.z);
        target->push_back(seg.start.y);
        target->push_back(seg.end.x);
        target->push_back(seg.end.z);
        target->push_back(seg.end.y);
    }

    // Concatenate into single buffer, record offsets
    std::vector<f32> allVerts;
    allVerts.reserve(rapidVerts.size() + cutVerts.size() + plungeVerts.size() + retractVerts.size());

    m_rapidStart = 0;
    m_rapidCount = static_cast<u32>(rapidVerts.size() / 3);
    allVerts.insert(allVerts.end(), rapidVerts.begin(), rapidVerts.end());

    m_cutStart = m_rapidCount;
    m_cutCount = static_cast<u32>(cutVerts.size() / 3);
    allVerts.insert(allVerts.end(), cutVerts.begin(), cutVerts.end());

    m_plungeStart = m_cutStart + m_cutCount;
    m_plungeCount = static_cast<u32>(plungeVerts.size() / 3);
    allVerts.insert(allVerts.end(), plungeVerts.begin(), plungeVerts.end());

    m_retractStart = m_plungeStart + m_plungeCount;
    m_retractCount = static_cast<u32>(retractVerts.size() / 3);
    allVerts.insert(allVerts.end(), retractVerts.begin(), retractVerts.end());

    if (allVerts.empty()) {
        m_pathDirty = false;
        return;
    }

    // Upload to GPU
    GL_CHECK(glGenVertexArrays(1, &m_pathVAO));
    GL_CHECK(glGenBuffers(1, &m_pathVBO));

    GL_CHECK(glBindVertexArray(m_pathVAO));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_pathVBO));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>(allVerts.size() * sizeof(f32)),
                          allVerts.data(),
                          GL_STATIC_DRAW));

    // Position attribute (location 0): vec3
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr));
    GL_CHECK(glEnableVertexAttribArray(0));

    GL_CHECK(glBindVertexArray(0));

    // Fit camera to toolpath bounds only on first load (not on filter/clip changes)
    if (m_needsCameraFit) {
        // Swap Y↔Z to match renderer's Y-up convention
        Vec3 bMin{m_program.boundsMin.x, m_program.boundsMin.z, m_program.boundsMin.y};
        Vec3 bMax{m_program.boundsMax.x, m_program.boundsMax.z, m_program.boundsMax.y};
        m_pathCamera.fitToBounds(bMin, bMax);
        m_needsCameraFit = false;
    }

    m_pathDirty = false;
}

void GCodePanel::destroyPathGeometry() {
    if (m_pathVBO != 0) {
        glDeleteBuffers(1, &m_pathVBO);
        m_pathVBO = 0;
    }
    if (m_pathVAO != 0) {
        glDeleteVertexArrays(1, &m_pathVAO);
        m_pathVAO = 0;
    }
    if (m_simVBO != 0) {
        glDeleteBuffers(1, &m_simVBO);
        m_simVBO = 0;
    }
    if (m_simVAO != 0) {
        glDeleteVertexArrays(1, &m_simVAO);
        m_simVAO = 0;
    }
    m_rapidCount = m_cutCount = m_plungeCount = m_retractCount = 0;
}

void GCodePanel::handlePathInput() {
    if (!ImGui::IsWindowHovered())
        return;

    ImGuiIO& io = ImGui::GetIO();
    auto& cfg = Config::instance();
    NavStyle nav = cfg.getNavStyle();
    f32 orbitSignX = cfg.getInvertOrbitX() ? 1.0f : -1.0f;
    f32 orbitSignY = cfg.getInvertOrbitY() ? 1.0f : -1.0f;

    // Mouse wheel zoom (all styles)
    if (io.MouseWheel != 0.0f) {
        m_pathCamera.zoom(io.MouseWheel * 0.5f);
    }

    switch (nav) {
    case NavStyle::CAD:
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = io.MouseDelta;
            if (io.KeyShift) {
                m_pathCamera.pan(-delta.x, delta.y);
            } else {
                m_pathCamera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
            }
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 delta = io.MouseDelta;
            m_pathCamera.pan(-delta.x, delta.y);
        }
        break;

    case NavStyle::Maya:
        if (io.KeyAlt) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = io.MouseDelta;
                m_pathCamera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                ImVec2 delta = io.MouseDelta;
                m_pathCamera.pan(-delta.x, delta.y);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                ImVec2 delta = io.MouseDelta;
                m_pathCamera.zoom(delta.y * 0.01f);
            }
        }
        break;

    default: // NavStyle::Default
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = io.MouseDelta;
            if (io.KeyShift) {
                m_pathCamera.pan(-delta.x, delta.y);
            } else {
                m_pathCamera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
            }
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = io.MouseDelta;
            m_pathCamera.pan(-delta.x, delta.y);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 delta = io.MouseDelta;
            m_pathCamera.zoom(delta.y * 0.01f);
        }
        break;
    }
}

// --- 3D Path View ---

void GCodePanel::renderPathView() {
    if (!hasGCode())
        return;

    renderZClipSlider();
    ImGui::Separator();

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    int width = static_cast<int>(contentSize.x);
    int height = static_cast<int>(contentSize.y);

    if (width <= 0 || height <= 0)
        return;

    // Resize framebuffer + camera viewport if dimensions changed
    if (width != m_pathViewWidth || height != m_pathViewHeight) {
        m_pathViewWidth = width;
        m_pathViewHeight = height;
        m_pathFramebuffer.resize(width, height);
        m_pathCamera.setViewport(width, height);
    }

    // Rebuild geometry if dirty
    if (m_pathDirty)
        buildPathGeometry();

    // Handle orbit/pan/zoom input
    handlePathInput();

    // --- Render to framebuffer ---
    m_pathFramebuffer.bind();

    m_pathRenderer.beginFrame(Color{0.12f, 0.12f, 0.14f, 1.0f});
    m_pathRenderer.setCamera(m_pathCamera);

    // Grid and axis
    m_pathRenderer.renderGrid(20.0f, 1.0f);
    m_pathRenderer.renderAxis(2.0f);

    // Draw toolpath lines
    if (m_pathVAO != 0) {
        Shader& flat = m_pathRenderer.flatShader();
        flat.bind();
        flat.setMat4("uMVP", m_pathCamera.viewProjectionMatrix());

        glDisable(GL_CULL_FACE);
        glBindVertexArray(m_pathVAO);

        bool simActive = m_simState != SimState::Stopped;

        // In simulation mode, draw all base geometry as dim ghost lines (future path)
        // In view mode, draw per-category colors
        if (simActive) {
            // Draw entire base geometry as dim ghost
            u32 totalVerts = m_retractStart + m_retractCount;
            if (totalVerts > 0) {
                flat.setVec4("uColor", Vec4{0.3f, 0.3f, 0.35f, 0.35f});
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(totalVerts));
            }
        } else {
            // Rapid: dim gray
            if (m_rapidCount > 0) {
                flat.setVec4("uColor", Vec4{0.4f, 0.4f, 0.4f, 0.5f});
                glDrawArrays(GL_LINES, static_cast<GLint>(m_rapidStart),
                             static_cast<GLsizei>(m_rapidCount));
            }

            // Cut: blue
            if (m_cutCount > 0) {
                flat.setVec4("uColor", Vec4{0.2f, 0.6f, 1.0f, 1.0f});
                glDrawArrays(GL_LINES, static_cast<GLint>(m_cutStart),
                             static_cast<GLsizei>(m_cutCount));
            }

            // Plunge: orange
            if (m_plungeCount > 0) {
                flat.setVec4("uColor", Vec4{1.0f, 0.5f, 0.1f, 1.0f});
                glDrawArrays(GL_LINES, static_cast<GLint>(m_plungeStart),
                             static_cast<GLsizei>(m_plungeCount));
            }

            // Retract: green
            if (m_retractCount > 0) {
                flat.setVec4("uColor", Vec4{0.3f, 0.8f, 0.3f, 0.6f});
                glDrawArrays(GL_LINES, static_cast<GLint>(m_retractStart),
                             static_cast<GLsizei>(m_retractCount));
            }
        }

        // --- Simulation overlay: completed + current segment ---
        if (simActive) {
            std::vector<f32> simVerts;
            simVerts.reserve((m_simSegmentIndex + 1) * 6);

            for (size_t si = 0; si < m_simSegmentIndex && si < m_program.path.size(); ++si) {
                const auto& seg = m_program.path[si];
                if (seg.end.z > m_currentLayer) continue;
                // Swap Y↔Z for Y-up renderer
                simVerts.push_back(seg.start.x);
                simVerts.push_back(seg.start.z);
                simVerts.push_back(seg.start.y);
                simVerts.push_back(seg.end.x);
                simVerts.push_back(seg.end.z);
                simVerts.push_back(seg.end.y);
            }

            u32 completedVertCount = static_cast<u32>(simVerts.size() / 3);

            // Current segment partial
            if (m_simSegmentIndex < m_program.path.size()) {
                const auto& cur = m_program.path[m_simSegmentIndex];
                float t = std::clamp(m_simSegmentProgress, 0.0f, 1.0f);
                float ex = cur.start.x + (cur.end.x - cur.start.x) * t;
                float ey = cur.start.y + (cur.end.y - cur.start.y) * t;
                float ez = cur.start.z + (cur.end.z - cur.start.z) * t;
                // Swap Y↔Z
                simVerts.push_back(cur.start.x);
                simVerts.push_back(cur.start.z);
                simVerts.push_back(cur.start.y);
                simVerts.push_back(ex);
                simVerts.push_back(ez);
                simVerts.push_back(ey);
            }

            if (!simVerts.empty()) {
                // Create/update sim VBO
                if (m_simVAO == 0) {
                    GL_CHECK(glGenVertexArrays(1, &m_simVAO));
                    GL_CHECK(glGenBuffers(1, &m_simVBO));
                    GL_CHECK(glBindVertexArray(m_simVAO));
                    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_simVBO));
                    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr));
                    GL_CHECK(glEnableVertexAttribArray(0));
                } else {
                    GL_CHECK(glBindVertexArray(m_simVAO));
                    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m_simVBO));
                }

                GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                                      static_cast<GLsizeiptr>(simVerts.size() * sizeof(f32)),
                                      simVerts.data(),
                                      GL_DYNAMIC_DRAW));

                // Draw completed in bright green
                if (completedVertCount > 0) {
                    flat.setVec4("uColor", Vec4{0.1f, 0.85f, 0.1f, 1.0f});
                    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(completedVertCount));
                }

                // Draw current segment in yellow
                if (m_simSegmentIndex < m_program.path.size()) {
                    flat.setVec4("uColor", Vec4{1.0f, 0.85f, 0.2f, 1.0f});
                    glDrawArrays(GL_LINES, static_cast<GLint>(completedVertCount), 2);

                    // Cutter dot at current position
                    flat.setVec4("uColor", Vec4{1.0f, 0.2f, 0.2f, 1.0f});
                    glPointSize(8.0f);
                    glDrawArrays(GL_POINTS, static_cast<GLint>(completedVertCount) + 1, 1);
                }
            }
        }

        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }

    m_pathRenderer.endFrame();
    m_pathFramebuffer.unbind();

    // Display framebuffer texture in ImGui
    ImGui::Image(static_cast<ImTextureID>(m_pathFramebuffer.colorTexture()),
                 contentSize,
                 ImVec2(0, 1),
                 ImVec2(1, 0));
}

void GCodePanel::renderZClipSlider() {
    ImGui::Text("Show Z up to:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##ZRange", &m_currentLayer, m_program.boundsMin.z, m_maxLayer, "%.2f mm")) {
        m_pathDirty = true;
    }
}

// --- GRBL event callbacks ---

void GCodePanel::onGrblConnected(bool connected, const std::string& version) {
    m_cncConnected = connected;
    m_cncVersion = version;
    if (connected) {
        addConsoleLine("Connected: " + version, ConsoleLine::Info);
        m_availablePorts = listSerialPorts(); // Refresh
    } else if (m_cncVersion.empty()) {
        // Was never connected — banner detection failed
        addConsoleLine("Connection failed — no compatible controller detected", ConsoleLine::Error);
    } else {
        addConsoleLine("Disconnected", ConsoleLine::Info);
    }
}

void GCodePanel::onGrblStatus(const MachineStatus& status) {
    m_machineStatus = status;
}

void GCodePanel::onGrblLineAcked(const LineAck& ack) {
    m_lastAckedLine = ack.lineIndex;
    if (!ack.ok) {
        addConsoleLine("error:" + std::to_string(ack.errorCode) + " " + ack.errorMessage,
                       ConsoleLine::Error);
    }
}

void GCodePanel::onGrblProgress(const StreamProgress& progress) {
    m_streamProgress = progress;

    // Check completion
    if (progress.ackedLines >= progress.totalLines && progress.totalLines > 0) {
        addConsoleLine("Stream complete", ConsoleLine::Info);
        ToastManager::instance().show(ToastType::Success, "Done",
                                      "G-code streaming finished");
    }
}

void GCodePanel::onGrblAlarm(int code, const std::string& desc) {
    addConsoleLine("ALARM:" + std::to_string(code) + " " + desc, ConsoleLine::Error);
    ToastManager::instance().show(ToastType::Error, "Alarm", desc);
}

void GCodePanel::onGrblError(const std::string& message) {
    addConsoleLine("[MSG] " + message, ConsoleLine::Info);
}

void GCodePanel::onGrblRawLine(const std::string& line, bool isSent) {
    if (isSent)
        addConsoleLine("> " + line, ConsoleLine::Sent);
    else if (line == "ok")
        addConsoleLine("< ok", ConsoleLine::Ok);
    else if (line.front() == '<')
        {} // Skip status reports in console (too noisy)
    else
        addConsoleLine("< " + line, ConsoleLine::Info);
}

void GCodePanel::addConsoleLine(const std::string& text, ConsoleLine::Type type) {
    m_consoleLines.push_back({text, type});
    while (m_consoleLines.size() > MAX_CONSOLE_LINES)
        m_consoleLines.pop_front();
}

std::vector<std::string> GCodePanel::getRawLines() const {
    std::vector<std::string> lines;
    for (const auto& cmd : m_program.commands) {
        lines.push_back(cmd.raw);
    }
    return lines;
}

void GCodePanel::buildSendProgram() {
    if (!m_cnc || !hasGCode())
        return;

    // Run pre-flight checks before streaming
    auto issues = runPreflightChecks(*m_cnc, false, false);
    bool hasErrors = false;
    for (const auto& issue : issues) {
        if (issue.severity == PreflightIssue::Error) {
            hasErrors = true;
            addConsoleLine(
                "PRE-FLIGHT ERROR: " + issue.message,
                ConsoleLine::Error);
        }
    }
    if (hasErrors)
        return;

    // Show warnings but proceed
    for (const auto& issue : issues) {
        if (issue.severity == PreflightIssue::Warning)
            addConsoleLine(
                "WARNING: " + issue.message, ConsoleLine::Info);
    }

    // Extract raw command lines, skip blanks and comments
    std::vector<std::string> lines;
    for (const auto& cmd : m_program.commands) {
        std::string cmdLine = cmd.raw;
        // Strip whitespace
        while (!cmdLine.empty() &&
               (cmdLine.back() == ' ' || cmdLine.back() == '\r'))
            cmdLine.pop_back();
        if (cmdLine.empty() ||
            cmdLine.front() == ';' || cmdLine.front() == '(')
            continue;
        // Strip inline comments
        auto semi = cmdLine.find(';');
        if (semi != std::string::npos)
            cmdLine = cmdLine.substr(0, semi);
        lines.push_back(cmdLine);
    }

    m_lastAckedLine = -1;
    m_streamProgress = {};
    m_cnc->startStream(lines);
    addConsoleLine(
        "Streaming " + std::to_string(lines.size()) + " lines",
        ConsoleLine::Info);
}

} // namespace dw
