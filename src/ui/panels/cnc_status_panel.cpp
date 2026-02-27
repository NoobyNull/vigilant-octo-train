#include "ui/panels/cnc_status_panel.h"

#include <cstdio>

#include <imgui.h>

#include "core/cnc/cnc_controller.h"
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
    renderAlarmBanner();
    ImGui::Spacing();
    renderDRO();
    ImGui::Spacing();
    renderFeedSpindle();
    renderOverrideControls();
    renderCoolantControls();
    renderMoveToDialog();

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

    // Show firmware version in small text with WCS selector right-aligned
    if (!m_version.empty()) {
        ImGui::TextDisabled("GRBL %s", m_version.c_str());
    }
    renderWcsSelector();
}

void CncStatusPanel::renderDRO() {
    ImGui::SeparatorText("Position");

    // Axis labels and colors
    static const char* axisNames[] = {"X", "Y", "Z"};
    static const char axisLetters[] = {'X', 'Y', 'Z'};
    static const ImVec4 axisColors[] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f), // X = Red
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f), // Y = Green
        ImVec4(0.3f, 0.5f, 1.0f, 1.0f), // Z = Blue
    };
    float workPos[] = {m_status.workPos.x, m_status.workPos.y, m_status.workPos.z};
    float machPos[] = {m_status.machinePos.x, m_status.machinePos.y, m_status.machinePos.z};

    // Work position — large digits, double-click to zero axis
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));

    for (int i = 0; i < 3; ++i) {
        ImGui::SetWindowFontScale(2.0f);
        ImGui::TextColored(axisColors[i], "%s", axisNames[i]);
        ImGui::SameLine();

        // Render as Selectable so double-click is detectable
        char posLabel[32];
        std::snprintf(posLabel, sizeof(posLabel), "%+10.3f##DRO%d",
                      static_cast<double>(workPos[i]), i);
        if (ImGui::Selectable(posLabel, false, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                // Zero this axis: G10 L20 P0 <axis>0
                if (m_cnc && m_connected && m_status.state == MachineState::Idle) {
                    char cmd[64];
                    std::snprintf(cmd, sizeof(cmd), "G10 L20 P0 %c0", axisLetters[i]);
                    m_cnc->sendCommand(cmd);
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetWindowFontScale(1.0f);
            ImGui::SetTooltip("Double-click to zero %s axis", axisNames[i]);
            ImGui::SetWindowFontScale(2.0f);
        }
        ImGui::SetWindowFontScale(1.0f);
    }

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

    // Move To button
    bool canMove = m_cnc && m_connected && m_status.state == MachineState::Idle;
    if (!canMove) ImGui::BeginDisabled();
    if (ImGui::SmallButton("Move To...")) {
        m_moveToX = m_status.workPos.x;
        m_moveToY = m_status.workPos.y;
        m_moveToZ = m_status.workPos.z;
        m_moveToOpen = true;
    }
    if (!canMove) ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Move machine to specific coordinates");
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

void CncStatusPanel::renderOverrideControls() {
    ImGui::SeparatorText("Overrides");

    // Feed override — slider (10-200%)
    ImGui::TextDisabled("Feed");
    ImGui::SameLine(70);
    int feedOvr = m_status.feedOverride;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
    if (ImGui::SliderInt("##FeedOvr", &feedOvr, 10, 200, "%d%%")) {
        if (m_cnc) m_cnc->setFeedOverride(feedOvr);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("R##Feed")) {
        if (m_cnc) m_cnc->setFeedOverride(100);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to 100%%");

    // Spindle override — slider (10-200%)
    ImGui::TextDisabled("Spindle");
    ImGui::SameLine(70);
    int spindleOvr = m_status.spindleOverride;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
    if (ImGui::SliderInt("##SpindleOvr", &spindleOvr, 10, 200, "%d%%")) {
        if (m_cnc) m_cnc->setSpindleOverride(spindleOvr);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("R##Spindle")) {
        if (m_cnc) m_cnc->setSpindleOverride(100);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to 100%%");

    // Rapid override — 3 buttons
    ImGui::TextDisabled("Rapid");
    ImGui::SameLine(70);
    int rapidOvr = m_status.rapidOverride;
    auto rapidBtn = [&](const char* label, int pct) {
        bool active = (rapidOvr == pct);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::SmallButton(label)) {
            if (m_cnc) m_cnc->setRapidOverride(pct);
        }
        if (active) ImGui::PopStyleColor();
    };
    rapidBtn("25%##Rapid", 25);
    ImGui::SameLine();
    rapidBtn("50%##Rapid", 50);
    ImGui::SameLine();
    rapidBtn("100%##Rapid", 100);
}

void CncStatusPanel::renderCoolantControls() {
    ImGui::SeparatorText("Coolant");

    bool canSend = m_cnc && m_connected &&
                   m_status.state != MachineState::Alarm;

    if (!canSend) ImGui::BeginDisabled();

    float btnWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;

    if (ImGui::Button("Flood (M8)", ImVec2(btnWidth, 0))) {
        if (m_cnc) m_cnc->sendCommand("M8");
    }
    ImGui::SameLine();
    if (ImGui::Button("Mist (M7)", ImVec2(btnWidth, 0))) {
        if (m_cnc) m_cnc->sendCommand("M7");
    }
    ImGui::SameLine();
    if (ImGui::Button("Off (M9)", ImVec2(btnWidth, 0))) {
        if (m_cnc) m_cnc->sendCommand("M9");
    }

    if (!canSend) ImGui::EndDisabled();
}

void CncStatusPanel::renderAlarmBanner() {
    if (m_status.state != MachineState::Alarm)
        return;

    ImGui::Spacing();

    // Red banner with alarm code and description
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 50.0f;

    ImGui::GetWindowDrawList()->AddRectFilled(
        cursorPos, ImVec2(cursorPos.x + width, cursorPos.y + height),
        IM_COL32(180, 40, 40, 200), 4.0f);

    // Alarm text
    char alarmText[128];
    if (m_lastAlarmCode > 0) {
        std::snprintf(alarmText, sizeof(alarmText), "ALARM %d: %s",
                      m_lastAlarmCode, m_lastAlarmDesc.c_str());
    } else {
        std::snprintf(alarmText, sizeof(alarmText), "ALARM (unknown code)");
    }
    ImVec2 textSize = ImGui::CalcTextSize(alarmText);
    float textX = cursorPos.x + 8.0f;
    float textY = cursorPos.y + (height - textSize.y) * 0.5f - 8.0f;
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(textX, textY), IM_COL32(255, 255, 255, 255), alarmText);

    ImGui::Dummy(ImVec2(width, height - 28.0f));

    // Inline unlock button
    if (ImGui::Button("Unlock ($X)", ImVec2(110, 0))) {
        if (m_cnc) m_cnc->unlock();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Clear alarm state to continue");

    // Alarm reference tooltip
    ImGui::SameLine();
    ImGui::SmallButton("?##AlarmRef");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("GRBL Alarm Codes");
        ImGui::Separator();
        int count = 0;
        const auto* entries = alarmReference(count);
        for (int i = 0; i < count; ++i) {
            bool isActive = (entries[i].code == m_lastAlarmCode);
            if (isActive)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::Text("ALARM %d: %s -- %s", entries[i].code,
                         entries[i].name, entries[i].description);
            if (isActive)
                ImGui::PopStyleColor();
        }
        ImGui::EndTooltip();
    }
}

void CncStatusPanel::renderWcsSelector() {
    // WCS combo — compact, right-aligned next to firmware version
    bool canSwitch = m_cnc && m_connected &&
                     (m_status.state == MachineState::Idle ||
                      m_status.state == MachineState::Jog);

    if (!canSwitch) ImGui::BeginDisabled();

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70);
    ImGui::SetNextItemWidth(70);
    if (ImGui::BeginCombo("##WCS", WCS_NAMES[m_activeWcs], ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < NUM_WCS; ++i) {
            bool selected = (m_activeWcs == i);
            if (ImGui::Selectable(WCS_NAMES[i], selected)) {
                m_activeWcs = i;
                if (m_cnc) {
                    m_cnc->sendCommand(WCS_NAMES[i]);
                }
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Work Coordinate System");
    }

    if (!canSwitch) ImGui::EndDisabled();
}

void CncStatusPanel::renderMoveToDialog() {
    if (m_moveToOpen) {
        ImGui::OpenPopup("Move To Position");
        m_moveToOpen = false;
    }
    if (ImGui::BeginPopupModal("Move To Position", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter target work coordinates:");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("X##moveto", &m_moveToX, 0.1f, 1.0f, "%.3f");
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Y##moveto", &m_moveToY, 0.1f, 1.0f, "%.3f");
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Z##moveto", &m_moveToZ, 0.1f, 1.0f, "%.3f");

        ImGui::Spacing();
        ImGui::Checkbox("Rapid (G0)", &m_moveToUseG0);
        if (!m_moveToUseG0) {
            ImGui::SameLine();
            ImGui::TextDisabled("(uses current feed rate)");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canGo = m_cnc && m_connected && m_status.state == MachineState::Idle;
        if (!canGo) ImGui::BeginDisabled();
        if (ImGui::Button("Go", ImVec2(80, 0))) {
            char cmd[128];
            const char* moveCmd = m_moveToUseG0 ? "G0" : "G1";
            std::snprintf(cmd, sizeof(cmd), "G90 %s X%.3f Y%.3f Z%.3f",
                          moveCmd,
                          static_cast<double>(m_moveToX),
                          static_cast<double>(m_moveToY),
                          static_cast<double>(m_moveToZ));
            m_cnc->sendCommand(cmd);
            ImGui::CloseCurrentPopup();
        }
        if (!canGo) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CncStatusPanel::onAlarm(int alarmCode, const std::string& desc) {
    m_lastAlarmCode = alarmCode;
    m_lastAlarmDesc = desc;
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
