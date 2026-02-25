#include "ui/panels/cnc_console_panel.h"

#include <cstring>

#include <imgui.h>

#include "core/cnc/cnc_controller.h"
#include "ui/icons.h"

namespace dw {

CncConsolePanel::CncConsolePanel() : Panel("MDI Console") {}

void CncConsolePanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_connected) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s Disconnected", Icons::Unlink);
        ImGui::TextDisabled("Connect a CNC machine to use console");
        ImGui::End();
        return;
    }

    // Response area — scrollable region above input
    float inputHeight = ImGui::GetFrameHeightWithSpacing() + 4.0f;
    if (ImGui::BeginChild("ConsoleOutput", ImVec2(0, -inputHeight), ImGuiChildFlags_Borders)) {
        for (const auto& line : m_lines) {
            ImVec4 color;
            const char* prefix = "";
            switch (line.type) {
            case ConsoleLine::Sent:
                color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // Cyan
                prefix = "> ";
                break;
            case ConsoleLine::Received:
                color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // White
                break;
            case ConsoleLine::Error:
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
                break;
            case ConsoleLine::Info:
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
                break;
            }
            ImGui::TextColored(color, "%s%s", prefix, line.text.c_str());
        }

        if (m_autoScroll && m_scrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_scrollToBottom = false;
        }
    }
    ImGui::EndChild();

    // Input line
    bool isStreaming = m_cnc && m_cnc->isStreaming();
    if (isStreaming)
        ImGui::BeginDisabled();

    ImGui::SetNextItemWidth(-1);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                 ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("##mdi_input", m_inputBuf, sizeof(m_inputBuf), flags,
                          inputCallback, this)) {
        std::string cmd(m_inputBuf);
        // Trim whitespace
        while (!cmd.empty() && (cmd.back() == ' ' || cmd.back() == '\t'))
            cmd.pop_back();
        while (!cmd.empty() && (cmd.front() == ' ' || cmd.front() == '\t'))
            cmd.erase(cmd.begin());

        if (!cmd.empty() && m_cnc) {
            m_cnc->sendCommand(cmd);

            // Add to history
            m_history.push_back(cmd);
            if (m_history.size() > MAX_HISTORY)
                m_history.erase(m_history.begin());
            m_historyPos = -1;

            // Clear input
            m_inputBuf[0] = '\0';
        }
        m_focusInput = true;
    }

    if (isStreaming) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Cannot send commands during active job");
        }
    }

    // Keep focus on input after sending
    if (m_focusInput) {
        ImGui::SetKeyboardFocusHere(-1);
        m_focusInput = false;
    }

    ImGui::End();
}

int CncConsolePanel::inputCallback(ImGuiInputTextCallbackData* data) {
    auto* self = static_cast<CncConsolePanel*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        int prevPos = self->m_historyPos;

        if (data->EventKey == ImGuiKey_UpArrow) {
            if (self->m_historyPos < static_cast<int>(self->m_history.size()) - 1)
                self->m_historyPos++;
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (self->m_historyPos > -1)
                self->m_historyPos--;
        }

        if (self->m_historyPos != prevPos) {
            const char* entry = "";
            if (self->m_historyPos >= 0 && !self->m_history.empty()) {
                int idx = static_cast<int>(self->m_history.size()) - 1 - self->m_historyPos;
                if (idx >= 0 && idx < static_cast<int>(self->m_history.size()))
                    entry = self->m_history[static_cast<size_t>(idx)].c_str();
            }
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, entry);
        }
    }
    return 0;
}

void CncConsolePanel::onRawLine(const std::string& line, bool isSent) {
    ConsoleLine cl;
    cl.text = line;
    cl.type = isSent ? ConsoleLine::Sent : ConsoleLine::Received;
    m_lines.push_back(std::move(cl));
    if (m_lines.size() > MAX_LINES)
        m_lines.pop_front();
    m_scrollToBottom = true;
}

void CncConsolePanel::onError(const std::string& message) {
    ConsoleLine cl;
    cl.text = message;
    cl.type = ConsoleLine::Error;
    m_lines.push_back(std::move(cl));
    if (m_lines.size() > MAX_LINES)
        m_lines.pop_front();
    m_scrollToBottom = true;
}

void CncConsolePanel::onAlarm(int code, const std::string& desc) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "ALARM:%d - %s", code, desc.c_str());
    ConsoleLine cl;
    cl.text = buf;
    cl.type = ConsoleLine::Error;
    m_lines.push_back(std::move(cl));
    if (m_lines.size() > MAX_LINES)
        m_lines.pop_front();
    m_scrollToBottom = true;
}

void CncConsolePanel::onConnectionChanged(bool connected, const std::string& /*version*/) {
    m_connected = connected;
    ConsoleLine cl;
    cl.type = ConsoleLine::Info;
    cl.text = connected ? "Connected to GRBL" : "Disconnected";
    m_lines.push_back(std::move(cl));
    if (m_lines.size() > MAX_LINES)
        m_lines.pop_front();
    m_scrollToBottom = true;
}

} // namespace dw
