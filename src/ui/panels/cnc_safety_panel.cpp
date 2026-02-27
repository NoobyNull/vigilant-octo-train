#include "ui/panels/cnc_safety_panel.h"

#include <cstdio>
#include <string>

#include <imgui.h>

#include "core/cnc/cnc_controller.h"
#include "core/cnc/cnc_types.h"
#include "core/cnc/preflight_check.h"
#include "core/config/config.h"
#include "core/gcode/gcode_modal_scanner.h"
#include "ui/icons.h"
#include "ui/theme.h"

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

static LongPressButton s_abortLongPress;
} // namespace

CncSafetyPanel::CncSafetyPanel() : Panel("Safety Controls") {}

void CncSafetyPanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    // Pause-before-reset abort sequence: feed hold first, then soft reset after delay
    if (m_abortPending) {
        m_abortTimer += ImGui::GetIO().DeltaTime;
        if (m_abortTimer >= 0.2f) {
            if (m_cnc) m_cnc->softReset();
            m_abortPending = false;
            m_abortTimer = 0.0f;
        }
    }

    renderSafetyControls();
    ImGui::Separator();
    renderDrawOutline();
    ImGui::Separator();
    renderSensorDisplay();
    renderAbortConfirmDialog();
    renderResumeDialog();

    ImGui::End();
}

void CncSafetyPanel::renderSafetyControls() {
    ImGui::SeparatorText("Job Control");

    // --- Pause button ---
    bool canPause = m_connected &&
                    (m_status.state == MachineState::Run ||
                     m_status.state == MachineState::Jog);

    if (!canPause)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button, canPause
        ? ImVec4(0.85f, 0.65f, 0.13f, 1.0f)   // Amber when enabled
        : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));     // Grey when disabled
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.75f, 0.23f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.55f, 0.10f, 1.0f));

    std::string pauseLabel = std::string(Icons::Pause) + " Pause";
    if (ImGui::Button(pauseLabel.c_str(), ImVec2(100, 32))) {
        if (m_cnc) m_cnc->feedHold();
    }

    ImGui::PopStyleColor(3);
    if (!canPause)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // --- Resume button ---
    bool canResume = m_connected &&
                     m_status.state == MachineState::Hold &&
                     !isDoorInterlockActive();

    if (!canResume)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button, canResume
        ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f)      // Green when enabled
        : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));     // Grey when disabled
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.6f, 0.25f, 1.0f));

    std::string resumeLabel = std::string(Icons::Play) + " Resume";
    if (ImGui::Button(resumeLabel.c_str(), ImVec2(100, 32))) {
        if (m_cnc) m_cnc->cycleStart();
    }

    ImGui::PopStyleColor(3);
    if (!canResume)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // --- Abort button (CRITICAL: labeled "Abort", NEVER "E-Stop") ---
    bool canAbort = m_connected;
    auto& cfg = Config::instance();
    bool abortUseLongPress = cfg.getSafetyAbortLongPress();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

    if (abortUseLongPress && m_streaming) {
        // Long-press abort — the hold IS the confirmation
        float durationMs = static_cast<float>(cfg.getSafetyLongPressDurationMs());
        std::string abortLabel = std::string(Icons::Stop) + " Hold to Abort";
        if (s_abortLongPress.render(abortLabel.c_str(), ImVec2(130, 32), durationMs, canAbort)) {
            if (m_cnc) {
                if (cfg.getSafetyPauseBeforeResetEnabled()) {
                    m_cnc->feedHold();
                    m_abortPending = true;
                    m_abortTimer = 0.0f;
                } else {
                    m_cnc->softReset();
                }
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Hold for %.1fs to abort running job", static_cast<double>(durationMs / 1000.0f));
        }
    } else {
        if (!canAbort)
            ImGui::BeginDisabled();
        std::string abortLabel = std::string(Icons::Stop) + " Abort";
        if (ImGui::Button(abortLabel.c_str(), ImVec2(100, 32))) {
            if (m_streaming) {
                // Show confirmation dialog when a job is running
                m_showAbortConfirm = true;
            } else {
                // Direct soft reset when no job is running
                if (m_cnc) m_cnc->softReset();
            }
        }
        if (!canAbort)
            ImGui::EndDisabled();
    }

    ImGui::PopStyleColor(3);

    // --- Resume From Line button ---
    ImGui::Spacing();
    bool canResumeFromLine = m_connected && !m_streaming && hasProgram();
    if (!canResumeFromLine)
        ImGui::BeginDisabled();
    if (ImGui::Button("Resume From Line...", ImVec2(-1, 0))) {
        m_showResumeDialog = true;
        m_preambleGenerated = false;
        m_preambleLines.clear();
        m_preflightIssues.clear();
        m_resumeLine = 1;
    }
    if (!canResumeFromLine)
        ImGui::EndDisabled();

    // Safety note
    ImGui::Spacing();
    ImGui::TextDisabled(
        "Software stop only -- ensure hardware E-stop is accessible");
}

void CncSafetyPanel::renderAbortConfirmDialog() {
    if (m_showAbortConfirm) {
        ImGui::OpenPopup("Abort Running Job?");
        m_showAbortConfirm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Abort Running Job?", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                           "%s WARNING", Icons::Warning);
        ImGui::Spacing();
        ImGui::TextWrapped(
            "A job is currently running. Aborting will send a soft reset "
            "(0x18) which stops all motion immediately.\n\n"
            "You may need to re-home the machine afterward.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Abort Job button (red)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Abort Job", ImVec2(120, 0))) {
            if (m_cnc) {
                if (Config::instance().getSafetyPauseBeforeResetEnabled()) {
                    // Pause-before-reset: feed hold first, soft reset after 200ms
                    m_cnc->feedHold();
                    m_abortPending = true;
                    m_abortTimer = 0.0f;
                } else {
                    m_cnc->softReset();
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Cancel button
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CncSafetyPanel::renderDrawOutline() {
    ImGui::SeparatorText("Draw Outline");

    if (!m_hasBounds) {
        ImGui::TextDisabled("Load a G-code file to enable outline tracing");
        return;
    }

    // Show bounding box dimensions
    float sizeX = m_boundsMax.x - m_boundsMin.x;
    float sizeY = m_boundsMax.y - m_boundsMin.y;
    ImGui::Text("Job bounds: %.1f x %.1f mm", static_cast<double>(sizeX),
                static_cast<double>(sizeY));
    ImGui::TextDisabled("  X: %.1f to %.1f  Y: %.1f to %.1f",
                        static_cast<double>(m_boundsMin.x),
                        static_cast<double>(m_boundsMax.x),
                        static_cast<double>(m_boundsMin.y),
                        static_cast<double>(m_boundsMax.y));

    ImGui::Spacing();

    // Safe Z height input
    ImGui::Text("Safe Z height:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputFloat("##safeZ", &m_outlineSafeZ, 0.0f, 0.0f, "%.1f");
    ImGui::SameLine();
    ImGui::TextDisabled("mm");
    if (m_outlineSafeZ < 0.0f) m_outlineSafeZ = 0.0f;
    if (m_outlineSafeZ > 100.0f) m_outlineSafeZ = 100.0f;

    // Feed rate input
    ImGui::Text("Travel speed:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::InputFloat("##outlineFeed", &m_outlineFeedRate, 0.0f, 0.0f, "%.0f");
    ImGui::SameLine();
    ImGui::TextDisabled("mm/min");
    if (m_outlineFeedRate < 100.0f) m_outlineFeedRate = 100.0f;
    if (m_outlineFeedRate > 10000.0f) m_outlineFeedRate = 10000.0f;

    ImGui::Spacing();

    // Draw outline button
    bool canDraw = m_connected && !m_streaming &&
                   m_status.state == MachineState::Idle;

    if (!canDraw)
        ImGui::BeginDisabled();

    if (ImGui::Button("Draw Outline", ImVec2(-1, 28))) {
        if (m_cnc) {
            // Generate outline G-code: raise to safe Z, trace bounding box, return
            char cmd[128];

            // Switch to absolute mode and raise Z to safe height
            m_cnc->sendCommand("G90");
            std::snprintf(cmd, sizeof(cmd), "G0 Z%.1f",
                          static_cast<double>(m_outlineSafeZ));
            m_cnc->sendCommand(cmd);

            // Move to first corner (min X, min Y) at rapid
            std::snprintf(cmd, sizeof(cmd), "G0 X%.3f Y%.3f",
                          static_cast<double>(m_boundsMin.x),
                          static_cast<double>(m_boundsMin.y));
            m_cnc->sendCommand(cmd);

            // Trace rectangle at feed rate
            std::snprintf(cmd, sizeof(cmd), "G1 X%.3f Y%.3f F%.0f",
                          static_cast<double>(m_boundsMax.x),
                          static_cast<double>(m_boundsMin.y),
                          static_cast<double>(m_outlineFeedRate));
            m_cnc->sendCommand(cmd);

            std::snprintf(cmd, sizeof(cmd), "G1 X%.3f Y%.3f",
                          static_cast<double>(m_boundsMax.x),
                          static_cast<double>(m_boundsMax.y));
            m_cnc->sendCommand(cmd);

            std::snprintf(cmd, sizeof(cmd), "G1 X%.3f Y%.3f",
                          static_cast<double>(m_boundsMin.x),
                          static_cast<double>(m_boundsMax.y));
            m_cnc->sendCommand(cmd);

            std::snprintf(cmd, sizeof(cmd), "G1 X%.3f Y%.3f",
                          static_cast<double>(m_boundsMin.x),
                          static_cast<double>(m_boundsMin.y));
            m_cnc->sendCommand(cmd);

            // Return to work zero XY at rapid, keep Z at safe height
            m_cnc->sendCommand("G0 X0 Y0");
        }
    }

    if (!canDraw)
        ImGui::EndDisabled();

    if (!m_connected) {
        ImGui::TextDisabled("Connect to CNC to use");
    } else if (m_streaming) {
        ImGui::TextDisabled("Cannot draw during active job");
    } else if (m_status.state != MachineState::Idle) {
        ImGui::TextDisabled("Machine must be idle");
    }
}

void CncSafetyPanel::renderSensorDisplay() {
    ImGui::SeparatorText("Input Pins");

    if (!m_connected) {
        ImGui::TextDisabled("Not connected");
        return;
    }

    // Door interlock warning banner
    if (isDoorInterlockActive()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        ImGui::TextWrapped("%s DOOR INTERLOCK ACTIVE -- Rapid moves and spindle "
                           "commands are blocked until door is closed", Icons::Warning);
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Helper lambda for pin indicator
    auto pinIndicator = [&](const char* label, u32 pinMask, ImVec4 activeColor) {
        bool active = (m_status.inputPins & pinMask) != 0;
        ImVec4 color = active ? activeColor : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(color, "%s %s", active ? Icons::Warning : "  ", label);
    };

    // Limit switches (red when active -- indicates potential issue)
    ImVec4 limitColor(1.0f, 0.3f, 0.3f, 1.0f);
    pinIndicator("X Limit", cnc::PIN_X_LIMIT, limitColor);
    ImGui::SameLine(120);
    pinIndicator("Y Limit", cnc::PIN_Y_LIMIT, limitColor);
    ImGui::SameLine(240);
    pinIndicator("Z Limit", cnc::PIN_Z_LIMIT, limitColor);

    // Probe (green when active)
    ImVec4 probeColor(0.3f, 0.8f, 0.3f, 1.0f);
    pinIndicator("Probe", cnc::PIN_PROBE, probeColor);

    ImGui::SameLine(120);

    // Door (yellow when active)
    ImVec4 doorColor(1.0f, 0.8f, 0.2f, 1.0f);
    pinIndicator("Door", cnc::PIN_DOOR, doorColor);
}

void CncSafetyPanel::renderResumeDialog() {
    if (m_showResumeDialog) {
        ImGui::OpenPopup("Resume From Line");
        m_showResumeDialog = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("Resume From Line", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize))
        return;

    int totalLines = static_cast<int>(m_fullProgram.size());

    // --- Line number input ---
    ImGui::Text("Line number:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputInt("##resumeline", &m_resumeLine)) {
        // Clamp to valid range
        if (m_resumeLine < 1) m_resumeLine = 1;
        if (m_resumeLine > totalLines) m_resumeLine = totalLines;
        m_preambleGenerated = false; // Reset on line change
    }
    ImGui::SameLine();
    ImGui::TextDisabled("of %d lines", totalLines);

    ImGui::Spacing();

    // --- Generate Preamble button ---
    if (ImGui::Button("Generate Preamble", ImVec2(-1, 0))) {
        // Convert 1-based display to 0-based for scanner
        auto state = GCodeModalScanner::scanToLine(
            m_fullProgram, m_resumeLine - 1);
        m_preambleLines = state.toPreamble();
        m_preambleGenerated = true;

        // Run pre-flight checks
        if (m_cnc) {
            m_preflightIssues = runPreflightChecks(
                *m_cnc, false, false);
        }
    }

    // --- Preamble preview ---
    if (m_preambleGenerated) {
        ImGui::Spacing();
        ImGui::SeparatorText("Modal State Preamble");
        ImGui::TextDisabled(
            "%d lines will be sent before resuming:",
            static_cast<int>(m_preambleLines.size()));

        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
        if (ImGui::BeginChild("##preamble", ImVec2(-1, 120), true)) {
            for (const auto& line : m_preambleLines) {
                ImGui::TextUnformatted(line.c_str());
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::TextDisabled(
            "These commands restore machine state to line %d",
            m_resumeLine);
    }

    // --- Pre-flight check results ---
    bool hasErrors = false;
    if (m_preambleGenerated && !m_preflightIssues.empty()) {
        ImGui::Spacing();
        ImGui::SeparatorText("Pre-flight Checks");
        for (const auto& issue : m_preflightIssues) {
            if (issue.severity == PreflightIssue::Error) {
                hasErrors = true;
                ImGui::TextColored(
                    ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                    "%s %s", Icons::Error, issue.message.c_str());
            } else {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                    "%s %s", Icons::Warning, issue.message.c_str());
            }
        }
    }

    // --- Warning text ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                       "%s CAUTION", Icons::Warning);
    ImGui::TextWrapped(
        "Resuming from an arbitrary line is inherently risky. "
        "Verify the preamble restores correct machine state "
        "before proceeding.");
    ImGui::TextWrapped(
        "Arc commands (G2/G3) at the resume point may not "
        "execute correctly.");

    // --- Action buttons ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Resume button (green, only when preamble generated and no errors)
    bool canResume = m_preambleGenerated && !hasErrors;
    if (!canResume)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.15f, 0.6f, 0.25f, 1.0f));

    if (ImGui::Button("Resume", ImVec2(120, 0))) {
        // Build combined program: preamble + remaining lines
        std::vector<std::string> combined;
        combined.reserve(
            m_preambleLines.size() +
            m_fullProgram.size() - static_cast<size_t>(m_resumeLine - 1));

        for (const auto& line : m_preambleLines)
            combined.push_back(line);

        for (int i = m_resumeLine - 1;
             i < static_cast<int>(m_fullProgram.size()); ++i) {
            combined.push_back(m_fullProgram[static_cast<size_t>(i)]);
        }

        if (m_cnc)
            m_cnc->startStream(combined);

        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(3);
    if (!canResume)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // Cancel button
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_preambleGenerated = false;
        m_preambleLines.clear();
        m_preflightIssues.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void CncSafetyPanel::onStatusUpdate(const MachineStatus& status) {
    m_status = status;
    m_doorActive = (status.inputPins & cnc::PIN_DOOR) != 0 ||
                   status.state == MachineState::Door;
}

bool CncSafetyPanel::isDoorInterlockActive() const {
    return m_doorActive && Config::instance().getSafetyDoorInterlockEnabled();
}

void CncSafetyPanel::onConnectionChanged(bool connected, const std::string& /*version*/) {
    m_connected = connected;
    if (!connected) {
        m_streaming = false;
        m_status = MachineStatus{};
    }
}

} // namespace dw
