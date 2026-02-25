#include "ui/panels/cnc_safety_panel.h"

#include <string>

#include <imgui.h>

#include "core/cnc/cnc_controller.h"
#include "core/cnc/cnc_types.h"
#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

CncSafetyPanel::CncSafetyPanel() : Panel("Safety Controls") {}

void CncSafetyPanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    renderSafetyControls();
    ImGui::Separator();
    renderSensorDisplay();
    renderAbortConfirmDialog();

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
                     m_status.state == MachineState::Hold;

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

    if (!canAbort)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

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

    ImGui::PopStyleColor(3);
    if (!canAbort)
        ImGui::EndDisabled();

    // Safety note
    ImGui::Spacing();
    ImGui::TextDisabled("Software stop only -- ensure hardware E-stop is accessible");
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
            if (m_cnc) m_cnc->softReset();
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

void CncSafetyPanel::renderSensorDisplay() {
    ImGui::SeparatorText("Input Pins");

    if (!m_connected) {
        ImGui::TextDisabled("Not connected");
        return;
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

void CncSafetyPanel::onStatusUpdate(const MachineStatus& status) {
    m_status = status;
}

void CncSafetyPanel::onConnectionChanged(bool connected, const std::string& /*version*/) {
    m_connected = connected;
    if (!connected) {
        m_streaming = false;
        m_status = MachineStatus{};
    }
}

} // namespace dw
