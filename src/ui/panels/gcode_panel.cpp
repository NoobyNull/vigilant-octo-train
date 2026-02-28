#include "gcode_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <numeric>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/database/gcode_repository.h"
#include "../../core/cnc/cnc_controller.h"
#include "../../core/cnc/preflight_check.h"
#include "../../core/gcode/gcode_modal_scanner.h"
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

    // Force GCode panel to stay docked — never float
    ImGuiWindowClass gcodeClass;
    gcodeClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoUndocking;
    ImGui::SetNextWindowClass(&gcodeClass);

    applyMinSize(24, 12);
    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        // Job completion flash bar
        if (m_jobFlashTimer > 0.0f) {
            m_jobFlashTimer -= ImGui::GetIO().DeltaTime;
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float width = ImGui::GetContentRegionAvail().x;
            float height = 4.0f;
            float alpha = 0.5f + 0.5f * std::sin(m_jobFlashTimer * 6.0f);
            ImU32 flashColor = ImGui::ColorConvertFloat4ToU32(
                ImVec4(0.2f, 0.8f, 0.2f, alpha));
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos, ImVec2(cursorPos.x + width, cursorPos.y + height),
                flashColor);
            ImGui::Dummy(ImVec2(width, height));
        }

        // Override keyboard shortcuts (only when connected)
        if (m_cnc && m_cncConnected) {
            auto& cfg = Config::instance();
            auto checkOverride = [&](BindAction action) -> bool {
                auto b = cfg.getBinding(action);
                if (!b.isValid() || b.type != InputType::Key)
                    return false;
                bool modsMatch = true;
                if ((b.modifiers & Mod_Ctrl) && !ImGui::GetIO().KeyCtrl) modsMatch = false;
                if ((b.modifiers & Mod_Shift) && !ImGui::GetIO().KeyShift) modsMatch = false;
                if ((b.modifiers & Mod_Alt) && !ImGui::GetIO().KeyAlt) modsMatch = false;
                return modsMatch && ImGui::IsKeyPressed(static_cast<ImGuiKey>(b.value), false);
            };

            if (checkOverride(BindAction::FeedOverridePlus)) {
                m_cnc->setFeedOverride(std::min(200, m_machineStatus.feedOverride + 10));
            }
            if (checkOverride(BindAction::FeedOverrideMinus)) {
                m_cnc->setFeedOverride(std::max(10, m_machineStatus.feedOverride - 10));
            }
            if (checkOverride(BindAction::SpindleOverridePlus)) {
                m_cnc->setSpindleOverride(std::min(200, m_machineStatus.spindleOverride + 10));
            }
            if (checkOverride(BindAction::SpindleOverrideMinus)) {
                m_cnc->setSpindleOverride(std::max(10, m_machineStatus.spindleOverride - 10));
            }
        }

        renderModeTabs();
        renderToolbar();
        renderSearchBar();

        if (m_showJobHistory) {
            renderJobHistory();
        }

        if (hasGCode()) {
            ImGui::Separator();

            // Mode-specific top sections
            if (m_mode == GCodePanelMode::Send) {
                renderConnectionBar();
                if (m_cncConnected) {
                    renderMachineStatus();
                    renderPlaybackControls();
                    renderProgressBar();
                    renderCarveProgress();
                    renderFeedOverride();
                }
            } else if (m_mode == GCodePanelMode::View) {
                renderSimulationControls();
            }

            ImGui::Separator();

            // Main content area: stats + path view
            float availWidth = ImGui::GetContentRegionAvail().x;

            if (availWidth < 420.0f) {
                float statsH = ImGui::GetContentRegionAvail().y * 0.4f;
                ImGui::BeginChild("Stats", ImVec2(0, statsH), true);
                renderStatistics();
                ImGui::EndChild();

                ImGui::BeginChild("PathView", ImVec2(0, 0), true);
                renderPathView();
                ImGui::EndChild();
            } else {
                float statsWidth = availWidth * 0.35f;
                float spacing = ImGui::GetStyle().ItemSpacing.x;

                ImGui::BeginChild("Stats", ImVec2(statsWidth, 0), true);
                renderStatistics();

                // Console below stats in sender mode
                if (m_mode == GCodePanelMode::Send) {
                    ImGui::Separator();
                    renderConsole();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("PathView", ImVec2(availWidth - statsWidth - spacing, 0), true);
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
    if (ImGui::BeginTabBar("##GCodeModeTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
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

        // Add to recent G-code files list
        Config::instance().addRecentGCodeFile(path);
        Config::instance().save();

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

    ImGui::SameLine();
    renderRecentFiles();

    if (m_jobRepo) {
        ImGui::SameLine();
        if (ImGui::Button(m_showJobHistory ? "Hide History" : "History")) {
            m_showJobHistory = !m_showJobHistory;
            if (m_showJobHistory)
                m_jobHistoryDirty = true;
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
        ImGui::SameLine();
        if (ImGui::Checkbox("Color by Tool", &m_colorByTool))
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

void GCodePanel::renderRecentFiles() {
    auto& cfg = Config::instance();
    const auto& recent = cfg.getRecentGCodeFiles();
    if (recent.empty()) {
        ImGui::BeginDisabled();
        ImGui::Button("Recent");
        ImGui::EndDisabled();
        return;
    }

    if (ImGui::Button("Recent")) {
        ImGui::OpenPopup("RecentGCodePopup");
    }
    if (ImGui::BeginPopup("RecentGCodePopup")) {
        for (const auto& rpath : recent) {
            std::string filename = rpath.string();
            auto lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos)
                filename = filename.substr(lastSlash + 1);

            if (ImGui::MenuItem(filename.c_str())) {
                loadFile(rpath.string());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", rpath.string().c_str());
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear Recent")) {
            cfg.clearRecentGCodeFiles();
            cfg.save();
        }
        ImGui::EndPopup();
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

    // Mode selector
    ImGui::SameLine();
    if (ImGui::RadioButton("Serial", m_connMode == ConnMode::Serial))
        m_connMode = ConnMode::Serial;
    ImGui::SameLine();
    if (ImGui::RadioButton("TCP", m_connMode == ConnMode::Tcp))
        m_connMode = ConnMode::Tcp;
    ImGui::SameLine();

    if (m_connMode == ConnMode::Serial) {
        // Refresh button
        if (ImGui::Button(Icons::Refresh)) {
            m_availablePorts = listSerialPorts();
            if (m_selectedPort >= m_availablePorts.size())
                m_selectedPort = 0;
        }
        ImGui::SameLine();

        // Port combo
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("/dev/ttyUSB0000").x + ImGui::GetStyle().FramePadding.x * 2);
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
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("115200").x + ImGui::GetStyle().FramePadding.x * 2);
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
    } else {
        // TCP mode: Host input + Port input
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("/dev/ttyUSB0000").x + ImGui::GetStyle().FramePadding.x * 2);
        ImGui::InputText("##TcpHost", m_tcpHost, sizeof(m_tcpHost));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000").x + ImGui::GetStyle().FramePadding.x * 2);
        ImGui::InputInt("##TcpPort", &m_tcpPort, 0, 0);
        if (m_tcpPort < 1)
            m_tcpPort = 1;
        if (m_tcpPort > 65535)
            m_tcpPort = 65535;
    }

    // Connect / Disconnect button
    ImGui::SameLine();
    if (m_cncConnected) {
        if (ImGui::Button("Disconnect")) {
            if (m_cnc)
                m_cnc->disconnect();
        }
    } else {
        bool canConnect = false;
        if (m_connMode == ConnMode::Serial)
            canConnect = m_cnc && !m_availablePorts.empty();
        else
            canConnect = m_cnc && m_tcpHost[0] != '\0';

        if (!canConnect)
            ImGui::BeginDisabled();
        if (ImGui::Button("Connect")) {
            if (m_connMode == ConnMode::Serial) {
                if (!m_cnc->connect(m_availablePorts[m_selectedPort], BAUD_RATES[m_selectedBaud])) {
                    addConsoleLine("Failed to open " + m_availablePorts[m_selectedPort], ConsoleLine::Error);
                } else {
                    addConsoleLine("Connecting to " + m_availablePorts[m_selectedPort] + "...", ConsoleLine::Info);
                }
            } else {
                std::string host(m_tcpHost);
                std::string addr = host + ":" + std::to_string(m_tcpPort);
                if (!m_cnc->connectTcp(host, m_tcpPort)) {
                    addConsoleLine("Failed to connect to " + addr, ConsoleLine::Error);
                } else {
                    addConsoleLine("Connecting to " + addr + "...", ConsoleLine::Info);
                }
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
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);

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
        // Door interlock: block start when door is active
        bool doorBlocked = Config::instance().getSafetyDoorInterlockEnabled() &&
                           ((m_machineStatus.inputPins & cnc::PIN_DOOR) != 0 ||
                            m_machineStatus.state == MachineState::Door);
        bool canStart = m_cncConnected &&
                        (m_machineStatus.state == MachineState::Idle) &&
                        !doorBlocked;
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
            if (m_cnc) {
                if (Config::instance().getSafetyPauseBeforeResetEnabled()) {
                    m_cnc->feedHold(); // Real-time byte, takes effect immediately
                }
                m_cnc->stopStream();
            }
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

// --- Direct Carve streaming ---

void GCodePanel::onCarveStreamStart(int totalLines) {
    m_carveStreamActive = true;
    m_carveCurrentLine = 0;
    m_carveTotalLines = totalLines;
    m_carveElapsedSec = 0.0f;
    m_carveAborted = false;
}

void GCodePanel::onCarveStreamProgress(int currentLine, int totalLines, float elapsedSec) {
    m_carveCurrentLine = currentLine;
    m_carveTotalLines = totalLines;
    m_carveElapsedSec = elapsedSec;
}

void GCodePanel::onCarveStreamComplete() {
    m_carveStreamActive = false;
}

void GCodePanel::onCarveStreamAborted() {
    m_carveStreamActive = false;
    m_carveAborted = true;
}

void GCodePanel::renderCarveProgress() {
    if (!m_carveStreamActive || m_carveTotalLines <= 0) return;

    float fraction = static_cast<float>(m_carveCurrentLine) /
                     static_cast<float>(m_carveTotalLines);

    // ETA from line rate
    float etaSec = 0.0f;
    if (m_carveCurrentLine > 0 && fraction < 1.0f) {
        float rate = static_cast<float>(m_carveCurrentLine) / m_carveElapsedSec;
        float remaining = static_cast<float>(m_carveTotalLines - m_carveCurrentLine);
        etaSec = remaining / rate;
    }

    int etaMin = static_cast<int>(etaSec / 60.0f);
    int etaS = static_cast<int>(etaSec) % 60;
    char overlay[128];
    snprintf(overlay, sizeof(overlay), "Carve: %d / %d  (%.0f%%)  ETA: %d:%02d",
             m_carveCurrentLine, m_carveTotalLines,
             static_cast<double>(fraction * 100.0f), etaMin, etaS);

    ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
}

// --- Feed override ---

void GCodePanel::renderFeedOverride() {
    ImGui::Text("Feed Override:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.15f);
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
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
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
        consoleHeight = ImGui::GetTextLineHeightWithSpacing() * 5;

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
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
    ImGui::Text("Speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("000000").x + ImGui::GetStyle().FramePadding.x * 2);
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
    // Live CNC tracking: when streaming, drive the 3D visualization from acked line index
    if (m_cncConnected && m_lastAckedLine >= 0 && !m_program.path.empty() &&
        m_streamProgress.totalLines > 0) {
        // Find the last path segment whose lineNumber <= m_lastAckedLine
        size_t bestIdx = 0;
        for (size_t i = 0; i < m_program.path.size(); ++i) {
            if (m_program.path[i].lineNumber <= m_lastAckedLine)
                bestIdx = i + 1; // +1 because simSegmentIndex is the *next* segment to animate
        }
        m_simSegmentIndex = bestIdx;
        m_simSegmentProgress = 0.0f;
        return;
    }

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
    m_toolGroups.clear();

    if (m_program.path.empty()) {
        m_pathDirty = false;
        return;
    }

    // Helper: add segment vertices (Y↔Z swap: G-code Z-up, renderer Y-up)
    auto addSegVerts = [](std::vector<f32>& verts, const gcode::PathSegment& seg) {
        verts.push_back(seg.start.x);
        verts.push_back(seg.start.z);
        verts.push_back(seg.start.y);
        verts.push_back(seg.end.x);
        verts.push_back(seg.end.z);
        verts.push_back(seg.end.y);
    };

    // Helper: classify non-rapid segment (returns false if filtered out)
    auto isVisibleNonRapid = [this](const gcode::PathSegment& seg) -> bool {
        float dz = seg.end.z - seg.start.z;
        float dxy2 = (seg.end.x - seg.start.x) * (seg.end.x - seg.start.x) +
                      (seg.end.y - seg.start.y) * (seg.end.y - seg.start.y);
        bool zDominant = (dz * dz) > dxy2 * 0.25f;

        if (dz < -0.001f && zDominant) return m_showPlunge;
        if (dz > 0.001f && zDominant) return m_showRetract;
        return m_showCut;
    };

    std::vector<f32> allVerts;

    if (m_colorByTool) {
        // --- Color by tool mode: group by tool number ---
        // First pass: collect rapids
        std::vector<f32> rapidVerts;
        // Collect non-rapid segments grouped by tool
        std::map<int, std::vector<f32>> toolVerts;

        for (const auto& seg : m_program.path) {
            if (seg.end.z > m_currentLayer) continue;

            if (seg.isRapid) {
                if (m_showRapid) addSegVerts(rapidVerts, seg);
            } else {
                if (!isVisibleNonRapid(seg)) continue;
                addSegVerts(toolVerts[seg.toolNumber], seg);
            }
        }

        allVerts.reserve(rapidVerts.size());
        for (auto& [tool, verts] : toolVerts)
            allVerts.reserve(allVerts.capacity() + verts.size());

        // Rapids first
        m_rapidStart = 0;
        m_rapidCount = static_cast<u32>(rapidVerts.size() / 3);
        allVerts.insert(allVerts.end(), rapidVerts.begin(), rapidVerts.end());

        // Zero out type-based groups (not used in tool mode)
        m_cutStart = m_cutCount = 0;
        m_plungeStart = m_plungeCount = 0;
        m_retractStart = m_retractCount = 0;

        // Tool groups
        u32 offset = m_rapidCount;
        for (auto& [tool, verts] : toolVerts) {
            ToolGroup tg;
            tg.toolNumber = tool;
            tg.start = offset;
            tg.count = static_cast<u32>(verts.size() / 3);
            m_toolGroups.push_back(tg);
            allVerts.insert(allVerts.end(), verts.begin(), verts.end());
            offset += tg.count;
        }
    } else {
        // --- Standard mode: group by move type ---
        std::vector<f32> rapidVerts;
        std::vector<f32> cutVerts;
        std::vector<f32> plungeVerts;
        std::vector<f32> retractVerts;

        for (const auto& seg : m_program.path) {
            if (seg.end.z > m_currentLayer) continue;

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

            addSegVerts(*target, seg);
        }

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
    }

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
        } else if (m_colorByTool && !m_toolGroups.empty()) {
            // Color-by-tool mode: rapids gray, then each tool with its color
            if (m_rapidCount > 0) {
                flat.setVec4("uColor", Vec4{0.4f, 0.4f, 0.4f, 0.5f});
                glDrawArrays(GL_LINES, static_cast<GLint>(m_rapidStart),
                             static_cast<GLsizei>(m_rapidCount));
            }
            for (const auto& tg : m_toolGroups) {
                if (tg.count == 0) continue;
                Vec3 tc = toolColor(tg.toolNumber);
                flat.setVec4("uColor", Vec4{tc.x, tc.y, tc.z, 1.0f});
                glDrawArrays(GL_LINES, static_cast<GLint>(tg.start),
                             static_cast<GLsizei>(tg.count));
            }
        } else {
            // Standard mode: color by move type
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

    // Live CNC tool position (when connected, regardless of G-code loaded)
    if (m_cncConnected) {
        auto& cfg = Config::instance();
        Vec3 wp = m_machineStatus.workPos;
        Vec3 renderPos{wp.x, wp.z, wp.y};

        // Expand far plane to encompass the work envelope
        f32 savedFar = m_pathCamera.farPlane();
        if (cfg.getCncShowWorkEnvelope()) {
            const auto& profile = cfg.getActiveMachineProfile();
            f32 envExtent = std::max({profile.maxTravelX, profile.maxTravelY,
                                      profile.maxTravelZ});
            f32 needed = (m_pathCamera.distance() + envExtent) * 2.0f;
            if (needed > savedFar) {
                m_pathCamera.setFarPlane(needed);
                m_pathRenderer.setCamera(m_pathCamera);
            }
        }

        if (cfg.getCncShowToolDot()) {
            m_pathRenderer.renderPoint(renderPos, cfg.getCncToolDotSize(), cfg.getCncToolDotColor());
        }
        if (cfg.getCncShowWorkEnvelope()) {
            const auto& profile = cfg.getActiveMachineProfile();
            Vec3 envMax{profile.maxTravelX, profile.maxTravelZ, profile.maxTravelY};
            m_pathRenderer.renderWireBox(Vec3{0, 0, 0}, envMax, cfg.getCncEnvelopeColor());
        }

        // Restore original far plane
        if (m_pathCamera.farPlane() != savedFar) {
            m_pathCamera.setFarPlane(savedFar);
        }
    }

    m_pathRenderer.endFrame();
    m_pathFramebuffer.unbind();

    // Display framebuffer texture in ImGui
    ImGui::Image(static_cast<ImTextureID>(m_pathFramebuffer.colorTexture()),
                 contentSize,
                 ImVec2(0, 1),
                 ImVec2(1, 0));

    // Live DRO overlay
    if (m_cncConnected && Config::instance().getCncShowDroOverlay()) {
        renderLiveDro();
    }
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
        // Finalize active job as aborted on disconnect
        if (m_jobRepo && m_activeJobId > 0) {
            auto modal = GCodeModalScanner::scanToLine(getRawLines(), m_streamProgress.ackedLines);
            m_jobRepo->finishJob(m_activeJobId, "aborted", m_streamProgress.ackedLines,
                                 m_streamProgress.elapsedSeconds, m_streamProgress.errorCount, modal);
            m_activeJobId = -1;
            m_jobHistoryDirty = true;
        }
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

    // Periodic progress save (every 50 acked lines)
    if (m_jobRepo && m_activeJobId > 0 &&
        progress.ackedLines - m_lastProgressSave >= 50) {
        m_jobRepo->updateProgress(m_activeJobId, progress.ackedLines, progress.elapsedSeconds);
        m_lastProgressSave = progress.ackedLines;
    }

    // Check completion
    if (progress.ackedLines >= progress.totalLines && progress.totalLines > 0) {
        addConsoleLine("Stream complete", ConsoleLine::Info);

        // Finalize job record
        if (m_jobRepo && m_activeJobId > 0) {
            auto modal = GCodeModalScanner::scanToLine(getRawLines(), progress.ackedLines);
            m_jobRepo->finishJob(m_activeJobId, "completed", progress.ackedLines,
                                 progress.elapsedSeconds, progress.errorCount, modal);
            m_activeJobId = -1;
            m_jobHistoryDirty = true;
        }

        auto& cfg = Config::instance();
        if (cfg.getJobCompletionNotify()) {
            int totalSec = static_cast<int>(progress.elapsedSeconds);
            int min = totalSec / 60;
            int sec = totalSec % 60;
            char msg[128];
            std::snprintf(msg, sizeof(msg), "G-code streaming finished in %d:%02d", min, sec);
            ToastManager::instance().show(ToastType::Success, "Job Complete", msg);
        }

        if (cfg.getJobCompletionFlash()) {
            m_jobFlashTimer = 3.0f;
        }
    }
}

void GCodePanel::onGrblAlarm(int code, const std::string& desc) {
    addConsoleLine("ALARM:" + std::to_string(code) + " " + desc, ConsoleLine::Error);
    ToastManager::instance().show(ToastType::Error, "Alarm", desc);

    // Finalize active job as aborted
    if (m_jobRepo && m_activeJobId > 0) {
        auto modal = GCodeModalScanner::scanToLine(getRawLines(), m_streamProgress.ackedLines);
        m_jobRepo->finishJob(m_activeJobId, "aborted", m_streamProgress.ackedLines,
                             m_streamProgress.elapsedSeconds, m_streamProgress.errorCount, modal);
        m_activeJobId = -1;
        m_jobHistoryDirty = true;
    }
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

    // Run pre-flight checks before streaming (with soft limit check)
    auto& profile = Config::instance().getActiveMachineProfile();
    Vec3 bmin = m_stats.boundsMin;
    Vec3 bmax = m_stats.boundsMax;
    auto issues = runPreflightChecks(*m_cnc, false, false,
                                     &bmin, &bmax, &profile);
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
    m_lastProgressSave = 0;

    // Record job start in database
    if (m_jobRepo) {
        JobRecord job;
        job.fileName = file::getStem(m_filePath) + "." + file::getExtension(m_filePath);
        job.filePath = m_filePath;
        job.totalLines = static_cast<int>(lines.size());
        auto id = m_jobRepo->insert(job);
        m_activeJobId = id.value_or(-1);
        m_jobHistoryDirty = true;
    }

    m_cnc->startStream(lines);
    addConsoleLine(
        "Streaming " + std::to_string(lines.size()) + " lines",
        ConsoleLine::Info);
}

// --- Search/Goto (EXT-12) ---

void GCodePanel::renderSearchBar() {
    if (!hasGCode()) return;

    // Toggle search with Ctrl+F
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
        m_searchActive = !m_searchActive;
    }

    if (!m_searchActive) return;

    ImGui::Separator();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
    bool searchChanged = ImGui::InputText("##Search", m_searchBuf, sizeof(m_searchBuf),
                                           ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();
    if (ImGui::SmallButton("Find Next") || searchChanged) {
        findNext();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("000000").x + ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::InputInt("##Goto", &m_gotoLine, 0, 0,
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        gotoLineNumber(m_gotoLine);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go to line number");

    ImGui::SameLine();
    if (ImGui::SmallButton("X##CloseSearch")) {
        m_searchActive = false;
        m_searchBuf[0] = '\0';
        m_searchResultLine = -1;
    }

    if (m_searchResultLine >= 0) {
        ImGui::SameLine();
        ImGui::TextDisabled("Match at line %d", m_searchResultLine + 1);
    }
}

void GCodePanel::findNext() {
    if (m_searchBuf[0] == '\0') return;

    auto lines = getRawLines();
    int total = static_cast<int>(lines.size());
    if (total == 0) return;

    std::string needle(m_searchBuf);
    for (auto& c : needle) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    int startLine = m_searchResultLine + 1;

    for (int i = 0; i < total; ++i) {
        int idx = (startLine + i) % total;
        std::string upper = lines[static_cast<size_t>(idx)];
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper.find(needle) != std::string::npos) {
            m_searchResultLine = idx;
            m_scrollToLine = idx;
            return;
        }
    }
    m_searchResultLine = -1;
}

void GCodePanel::gotoLineNumber(int lineNum) {
    auto lines = getRawLines();
    int idx = lineNum - 1;
    if (idx >= 0 && idx < static_cast<int>(lines.size())) {
        m_scrollToLine = idx;
        m_searchResultLine = idx;
    }
}

// --- Job History ---

void GCodePanel::renderJobHistory() {
    if (!m_jobRepo)
        return;

    // Refresh cache when dirty
    if (m_jobHistoryDirty) {
        m_jobHistoryCache = m_jobRepo->findRecent(50);
        m_jobHistoryDirty = false;
    }

    ImGui::Separator();
    ImGui::Text("Job History");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear All")) {
        m_jobRepo->clearAll();
        m_jobHistoryDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh"))
        m_jobHistoryDirty = true;

    if (m_jobHistoryCache.empty()) {
        ImGui::TextDisabled("No job history");
        ImGui::Separator();
        return;
    }

    float historyHeight = std::min(ImGui::GetContentRegionAvail().y * 0.4f, ImGui::GetContentRegionAvail().y * 0.5f);
    if (ImGui::BeginChild("JobHistoryList", ImVec2(0, historyHeight), true)) {
        if (ImGui::BeginTable("Jobs", 6,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                  ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            float colStatus = ImGui::CalcTextSize("Status__").x;
            float colLines = ImGui::CalcTextSize("00000/00000").x;
            float colDur = ImGui::CalcTextSize("00:00:00").x;
            float colDate = ImGui::CalcTextSize("2026-02-28 12:00").x;
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, colStatus);
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Lines", ImGuiTableColumnFlags_WidthFixed, colLines);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, colDur);
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, colDate);
            ImGui::TableSetupColumn("##Actions", ImGuiTableColumnFlags_WidthFixed, colStatus);
            ImGui::TableHeadersRow();

            i64 removeId = -1;
            for (const auto& job : m_jobHistoryCache) {
                ImGui::TableNextRow();
                ImGui::PushID(static_cast<int>(job.id));

                // Status column with color
                ImGui::TableNextColumn();
                if (job.status == "completed") {
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Done");
                } else if (job.status == "aborted") {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Abort");
                } else if (job.status == "interrupted") {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.1f, 1.0f), "Crash");
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Run");
                }

                // File name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(job.fileName.c_str());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", job.filePath.c_str());

                // Lines (acked/total)
                ImGui::TableNextColumn();
                ImGui::Text("%d/%d", job.lastAckedLine, job.totalLines);

                // Duration
                ImGui::TableNextColumn();
                int totalSec = static_cast<int>(job.elapsedSeconds);
                int min = totalSec / 60;
                int sec = totalSec % 60;
                ImGui::Text("%d:%02d", min, sec);

                // Date
                ImGui::TableNextColumn();
                // Show just date portion of ISO timestamp
                if (job.startedAt.size() >= 16)
                    ImGui::TextUnformatted(job.startedAt.substr(0, 16).c_str());
                else
                    ImGui::TextUnformatted(job.startedAt.c_str());

                // Actions
                ImGui::TableNextColumn();
                if (job.status != "completed" && job.status != "running" &&
                    job.lastAckedLine > 0) {
                    if (ImGui::SmallButton("Resume")) {
                        // Load file and pre-set resume line
                        if (loadFile(job.filePath)) {
                            m_showJobHistory = false;
                            m_mode = GCodePanelMode::Send;
                            addConsoleLine(
                                "Loaded from history — resume from line " +
                                    std::to_string(job.lastAckedLine),
                                ConsoleLine::Info);
                        }
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Load file for resume from line %d", job.lastAckedLine);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                    removeId = job.id;

                ImGui::PopID();
            }

            ImGui::EndTable();

            if (removeId > 0) {
                m_jobRepo->remove(removeId);
                m_jobHistoryDirty = true;
            }
        }
    }
    ImGui::EndChild();
    ImGui::Separator();
}

void GCodePanel::renderLiveDro() {
    ImVec2 rectMin = ImGui::GetItemRectMin();
    ImVec2 rectMax = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const auto& wp = m_machineStatus.workPos;
    bool metric = Config::instance().getDisplayUnitsMetric();
    const char* unit = metric ? "mm" : "in";
    f32 scale = metric ? 1.0f : (1.0f / 25.4f);

    char xBuf[32], yBuf[32], zBuf[32];
    std::snprintf(xBuf, sizeof(xBuf), "X: %8.3f %s", static_cast<double>(wp.x * scale), unit);
    std::snprintf(yBuf, sizeof(yBuf), "Y: %8.3f %s", static_cast<double>(wp.y * scale), unit);
    std::snprintf(zBuf, sizeof(zBuf), "Z: %8.3f %s", static_cast<double>(wp.z * scale), unit);

    f32 lineH = ImGui::GetTextLineHeightWithSpacing();
    f32 padding = ImGui::GetStyle().FramePadding.x;
    f32 textW = ImGui::CalcTextSize(xBuf).x;
    f32 boxW = textW + padding * 2.0f;
    f32 boxH = lineH * 3.0f + padding * 2.0f;

    ImVec2 boxMin = {rectMin.x + padding, rectMax.y - boxH - padding};
    ImVec2 boxMax = {boxMin.x + boxW, boxMin.y + boxH};

    dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 160), 4.0f);

    f32 textX = boxMin.x + padding;
    f32 textY = boxMin.y + padding;
    dl->AddText({textX, textY}, IM_COL32(255, 80, 80, 255), xBuf);
    dl->AddText({textX, textY + lineH}, IM_COL32(80, 255, 80, 255), yBuf);
    dl->AddText({textX, textY + lineH * 2.0f}, IM_COL32(80, 130, 255, 255), zBuf);
}

} // namespace dw
