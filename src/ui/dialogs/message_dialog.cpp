#include "message_dialog.h"

#include <imgui.h>

#include "../icons.h"

namespace dw {

MessageDialog::MessageDialog() : Dialog("Message") {}

void MessageDialog::render() {
    if (!m_open) {
        return;
    }

    ImGui::OpenPopup(m_title.c_str());

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(m_title.c_str(), &m_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Icon based on type
        const char* icon = Icons::Info;
        ImVec4 iconColor = ImVec4(0.4f, 0.6f, 0.8f, 1.0f);

        switch (m_type) {
        case MessageType::Warning:
            icon = Icons::Warning;
            iconColor = ImVec4(0.8f, 0.6f, 0.3f, 1.0f);
            break;
        case MessageType::Error:
            icon = Icons::Error;
            iconColor = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            break;
        case MessageType::Question:
            icon = Icons::Question;
            iconColor = ImVec4(0.5f, 0.7f, 0.9f, 1.0f);
            break;
        default:
            break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextWrapped("%s", m_message.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float contentWidth = ImGui::GetContentRegionAvail().x;

        if (m_type == MessageType::Question) {
            // Yes/No buttons
            ImGui::SetCursorPosX((contentWidth - buttonWidth * 2 - spacing) / 2);
            if (ImGui::Button("Yes", ImVec2(buttonWidth, 0))) {
                m_open = false;
                if (m_callback) {
                    m_callback(DialogResult::Yes);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(buttonWidth, 0))) {
                m_open = false;
                if (m_callback) {
                    m_callback(DialogResult::No);
                }
            }
        } else {
            // OK button
            ImGui::SetCursorPosX((contentWidth - buttonWidth) / 2);
            if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
                m_open = false;
                if (m_callback) {
                    m_callback(DialogResult::Ok);
                }
            }
        }

        ImGui::EndPopup();
    }
}

void MessageDialog::show(const std::string& title, const std::string& message, MessageType type,
                         std::function<void(DialogResult)> callback) {
    m_title = title;
    m_message = message;
    m_type = type;
    m_callback = std::move(callback);
    m_open = true;
}

MessageDialog& MessageDialog::instance() {
    static MessageDialog dialog;
    return dialog;
}

void MessageDialog::info(const std::string& title, const std::string& message) {
    instance().show(title, message, MessageType::Info);
}

void MessageDialog::warning(const std::string& title, const std::string& message) {
    instance().show(title, message, MessageType::Warning);
}

void MessageDialog::error(const std::string& title, const std::string& message) {
    instance().show(title, message, MessageType::Error);
}

// ConfirmDialog

ConfirmDialog::ConfirmDialog() : Dialog("Confirm") {}

void ConfirmDialog::render() {
    if (!m_open) {
        return;
    }

    ImGui::OpenPopup(m_title.c_str());

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(m_title.c_str(), &m_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 0.9f, 1.0f));
        ImGui::Text("%s", Icons::Question);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextWrapped("%s", m_message.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float contentWidth = ImGui::GetContentRegionAvail().x;

        ImGui::SetCursorPosX((contentWidth - buttonWidth * 2 - spacing) / 2);

        if (ImGui::Button("Yes", ImVec2(buttonWidth, 0))) {
            m_open = false;
            if (m_callback) {
                m_callback(true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(buttonWidth, 0))) {
            m_open = false;
            if (m_callback) {
                m_callback(false);
            }
        }

        ImGui::EndPopup();
    }
}

void ConfirmDialog::show(const std::string& title, const std::string& message,
                         std::function<void(bool)> callback) {
    m_title = title;
    m_message = message;
    m_callback = std::move(callback);
    m_open = true;
}

} // namespace dw
