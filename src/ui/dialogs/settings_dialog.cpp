#include "settings_dialog.h"

#include "../../core/paths/app_paths.h"
#include "../icons.h"

#include <imgui.h>

namespace dw {

SettingsDialog::SettingsDialog(Config* config)
    : Dialog("Settings"), m_config(config) {
    // Load current values
    if (m_config) {
        m_selectedTheme = m_config->getDarkMode() ? 0 : 1;
        m_uiScale = m_config->getUiScale();
        m_showGrid = m_config->getShowGrid();
        m_showAxis = m_config->getShowAxis();
    }
}

void SettingsDialog::render() {
    if (!m_open) {
        return;
    }

    ImGui::OpenPopup(m_title.c_str());

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(m_title.c_str(), &m_open)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                renderGeneralTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance")) {
                renderAppearanceTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Paths")) {
                renderPathsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About")) {
                renderAboutTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Separator();

        // Buttons
        float buttonWidth = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float contentWidth = ImGui::GetContentRegionAvail().x;

        ImGui::SetCursorPosX(contentWidth - buttonWidth * 2 - spacing);

        if (ImGui::Button("Apply", ImVec2(buttonWidth, 0))) {
            if (m_config) {
                m_config->setDarkMode(m_selectedTheme == 0);
                m_config->setUiScale(m_uiScale);
                m_config->setShowGrid(m_showGrid);
                m_config->setShowAxis(m_showAxis);
                m_config->save();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(buttonWidth, 0))) {
            m_open = false;
        }

        ImGui::EndPopup();
    }
}

void SettingsDialog::renderGeneralTab() {
    ImGui::Spacing();

    ImGui::Text("Viewport");
    ImGui::Indent();
    ImGui::Checkbox("Show Grid", &m_showGrid);
    ImGui::Checkbox("Show Axis", &m_showAxis);
    ImGui::Unindent();
}

void SettingsDialog::renderAppearanceTab() {
    ImGui::Spacing();

    ImGui::Text("Theme");
    ImGui::Indent();

    const char* themes[] = {"Dark", "Light", "High Contrast"};
    ImGui::Combo("##Theme", &m_selectedTheme, themes, 3);

    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("UI Scale");
    ImGui::Indent();
    ImGui::SliderFloat("##Scale", &m_uiScale, 0.75f, 2.0f, "%.2f");
    if (ImGui::Button("Reset to 100%")) {
        m_uiScale = 1.0f;
    }
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::TextDisabled("Note: Theme changes take effect immediately.");
    ImGui::TextDisabled("UI scale changes require restart.");
}

void SettingsDialog::renderPathsTab() {
    ImGui::Spacing();

    ImGui::Text("Application Paths");
    ImGui::Spacing();

    // Config path
    ImGui::TextDisabled("Configuration:");
    ImGui::Text("  %s", paths::getConfigDir().string().c_str());

    ImGui::Spacing();

    // Data path
    ImGui::TextDisabled("Application Data:");
    ImGui::Text("  %s", paths::getDataDir().string().c_str());

    ImGui::Spacing();

    // User path
    ImGui::TextDisabled("User Projects:");
    ImGui::Text("  %s", paths::getDefaultProjectsDir().string().c_str());

    ImGui::Spacing();

    // Database path
    ImGui::TextDisabled("Database:");
    ImGui::Text("  %s", paths::getDatabasePath().string().c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Open Config Folder")) {
        // TODO: Open in file manager
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Projects Folder")) {
        // TODO: Open in file manager
    }
}

void SettingsDialog::renderAboutTab() {
    ImGui::Spacing();

    ImGui::Text("Digital Workshop");
    ImGui::TextDisabled("Version 0.1.0-dev");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Digital Workshop is a 3D model management application "
        "for CNC and 3D printing workflows.");

    ImGui::Spacing();

    ImGui::Text("Libraries:");
    ImGui::BulletText("SDL2 - Window management");
    ImGui::BulletText("Dear ImGui - User interface");
    ImGui::BulletText("OpenGL 3.3 - 3D rendering");
    ImGui::BulletText("SQLite3 - Database");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Built with C++17");
}

}  // namespace dw
