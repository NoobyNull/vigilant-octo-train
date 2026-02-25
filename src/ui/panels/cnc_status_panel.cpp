#include "ui/panels/cnc_status_panel.h"

#include <cstdio>

#include <imgui.h>

#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

namespace {

const char* machineStateName(MachineState state) {
    switch (state) {
    case MachineState::Idle: return "Idle";
    case MachineState::Run: return "Run";
    case MachineState::Hold: return "Hold";
    case MachineState::Jog: return "Jog";
    case MachineState::Alarm: return "Alarm";
    case MachineState::Door: return "Door";
    case MachineState::Check: return "Check";
    case MachineState::Home: return "Homing";
    case MachineState::Sleep: return "Sleep";
    case MachineState::Unknown: return "Unknown";
    default: return "???";
    }
}

ImVec4 machineStateColor(MachineState state) {
    switch (state) {
    case MachineState::Idle: return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);   // Green
    case MachineState::Run: return ImVec4(0.3f, 0.5f, 1.0f, 1.0f);    // Blue
    case MachineState::Hold: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);   // Yellow
    case MachineState::Jog: return ImVec4(0.3f, 0.7f, 1.0f, 1.0f);    // Light blue
    case MachineState::Alarm: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
    case MachineState::Door: return ImVec4(1.0f, 0.5f, 0.2f, 1.0f);   // Orange
    case MachineState::Check: return ImVec4(0.6f, 0.6f, 0.8f, 1.0f);  // Lavender
    case MachineState::Home: return ImVec4(0.5f, 0.8f, 1.0f, 1.0f);   // Cyan
    case MachineState::Sleep: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
    default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

// Darker version of state color for background fill
ImU32 machineStateBgColor(MachineState state) {
    ImVec4 c = machineStateColor(state);
    // Darken to ~40% intensity for background
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x * 0.4f, c.y * 0.4f, c.z * 0.4f, 0.9f));
}

} // anonymous namespace

CncStatusPanel::CncStatusPanel() : Panel("CNC Status") {}

void CncStatusPanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_connected) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s Disconnected", Icons::Unlink);
        if (!m_version.empty())
            ImGui::TextDisabled("Last firmware: %s", m_version.c_str());
        else
            ImGui::TextDisabled("Connect a CNC machine to see status");
        ImGui::End();
        return;
    }

    renderStateIndicator();
    ImGui::Spacing();
    renderDRO();
    ImGui::Spacing();
    renderFeedSpindle();
    renderOverrides();

    ImGui::End();
}

void CncStatusPanel::renderStateIndicator() {
    ImVec4 stateColor = machineStateColor(m_status.state);
    ImU32 bgColor = machineStateBgColor(m_status.state);
    const char* stateName = machineStateName(m_status.state);

    // Draw a full-width colored banner for the state
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 30.0f;

    ImGui::GetWindowDrawList()->AddRectFilled(
        cursorPos, ImVec2(cursorPos.x + width, cursorPos.y + height), bgColor, 4.0f);

    // Center the state text on the banner
    char label[64];
    std::snprintf(label, sizeof(label), "%s  %s", Icons::Info, stateName);
    ImVec2 textSize = ImGui::CalcTextSize(label);
    float textX = cursorPos.x + (width - textSize.x) * 0.5f;
    float textY = cursorPos.y + (height - textSize.y) * 0.5f;

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(textX, textY), ImGui::ColorConvertFloat4ToU32(stateColor), label);

    // Advance cursor past the banner
    ImGui::Dummy(ImVec2(width, height));

    // Show firmware version in small text
    if (!m_version.empty()) {
        ImGui::TextDisabled("GRBL %s", m_version.c_str());
    }
}

void CncStatusPanel::renderDRO() {
    ImGui::SeparatorText("Position");

    // Axis labels and colors
    static const char* axisNames[] = {"X", "Y", "Z"};
    static const ImVec4 axisColors[] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f), // X = Red
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f), // Y = Green
        ImVec4(0.3f, 0.5f, 1.0f, 1.0f), // Z = Blue
    };
    float workPos[] = {m_status.workPos.x, m_status.workPos.y, m_status.workPos.z};
    float machPos[] = {m_status.machinePos.x, m_status.machinePos.y, m_status.machinePos.z};

    // Work position — large digits
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    ImGui::SetWindowFontScale(2.0f);

    for (int i = 0; i < 3; ++i) {
        ImGui::TextColored(axisColors[i], "%s", axisNames[i]);
        ImGui::SameLine();
        ImGui::Text("%+10.3f", static_cast<double>(workPos[i]));
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar();

    // Machine position — smaller, secondary
    ImGui::Spacing();
    ImGui::TextDisabled("Machine:");
    ImGui::SameLine();
    for (int i = 0; i < 3; ++i) {
        ImGui::TextDisabled("%s %+.3f", axisNames[i], static_cast<double>(machPos[i]));
        if (i < 2)
            ImGui::SameLine();
    }
}

void CncStatusPanel::renderFeedSpindle() {
    ImGui::SeparatorText("Feed & Spindle");

    float availWidth = ImGui::GetContentRegionAvail().x;
    float halfWidth = availWidth * 0.5f;

    // Feed rate — left side
    ImGui::BeginGroup();
    ImGui::TextDisabled("Feed Rate");
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%.0f", static_cast<double>(m_status.feedRate));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    ImGui::TextDisabled("mm/min");
    ImGui::EndGroup();

    ImGui::SameLine(halfWidth);

    // Spindle speed — right side
    ImGui::BeginGroup();
    ImGui::TextDisabled("Spindle");
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%.0f", static_cast<double>(m_status.spindleSpeed));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    ImGui::TextDisabled("RPM");
    ImGui::EndGroup();
}

void CncStatusPanel::renderOverrides() {
    // Only show if any override differs from 100%
    if (m_status.feedOverride == 100 && m_status.rapidOverride == 100 &&
        m_status.spindleOverride == 100)
        return;

    ImGui::Spacing();
    ImGui::SeparatorText("Overrides");

    auto overrideColor = [](int pct) -> ImVec4 {
        if (pct == 100)
            return ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Normal — white/gray
        if (pct < 100)
            return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Reduced — yellow
        return ImVec4(0.3f, 0.8f, 1.0f, 1.0f);      // Increased — cyan
    };

    ImGui::TextColored(overrideColor(m_status.feedOverride), "Feed %d%%",
                        m_status.feedOverride);
    ImGui::SameLine();
    ImGui::TextColored(overrideColor(m_status.rapidOverride), "Rapid %d%%",
                        m_status.rapidOverride);
    ImGui::SameLine();
    ImGui::TextColored(overrideColor(m_status.spindleOverride), "Spindle %d%%",
                        m_status.spindleOverride);
}

void CncStatusPanel::onStatusUpdate(const MachineStatus& status) {
    m_status = status;
}

void CncStatusPanel::onConnectionChanged(bool connected, const std::string& version) {
    m_connected = connected;
    m_version = version;
    if (!connected) {
        // Reset status to defaults on disconnect
        m_status = MachineStatus{};
    }
}

} // namespace dw
