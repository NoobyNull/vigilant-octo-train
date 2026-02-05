#include "start_page.h"

#include "../../core/config/config.h"
#include "../../core/paths/app_paths.h"
#include "version.h"

#include <imgui.h>

namespace dw {

StartPage::StartPage() : Panel("Start Page") {}

void StartPage::render() {
    if (!m_open) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin(m_title.c_str(), &m_open, flags)) {
        // Header
        ImGui::Text("Digital Workshop");
        ImGui::TextDisabled("Version %s", VERSION);
        ImGui::TextDisabled("3D Model Management for CNC and 3D Printing");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Two-column layout
        float availWidth = ImGui::GetContentRegionAvail().x;
        float leftWidth = availWidth * 0.6f;

        ImGui::BeginChild("##StartLeft", ImVec2(leftWidth, 0), false);
        renderRecentProjects();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##StartRight", ImVec2(0, 0), false);
        renderQuickActions();
        ImGui::EndChild();
    }
    ImGui::End();
}

void StartPage::renderRecentProjects() {
    ImGui::Text("Recent Projects");
    ImGui::Spacing();

    const auto& recentProjects = Config::instance().getRecentProjects();

    if (recentProjects.empty()) {
        ImGui::TextDisabled("No recent projects.");
        ImGui::TextDisabled("Create a new project or open an existing one to get started.");
    } else {
        for (size_t i = 0; i < recentProjects.size(); ++i) {
            const auto& projectPath = recentProjects[i];
            ImGui::PushID(static_cast<int>(i));

            std::string name = projectPath.stem().string();
            if (name.empty()) {
                name = projectPath.filename().string();
            }

            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0, 24))) {
                if (m_onOpenRecentProject) {
                    m_onOpenRecentProject(projectPath);
                }
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", projectPath.string().c_str());
            }

            // Show path in dim text on same line
            ImGui::SameLine();
            ImGui::TextDisabled("%s", projectPath.parent_path().string().c_str());

            ImGui::PopID();
        }
    }
}

void StartPage::renderQuickActions() {
    ImGui::Text("Quick Actions");
    ImGui::Spacing();

    float buttonWidth = ImGui::GetContentRegionAvail().x - 16.0f;
    float buttonHeight = 32.0f;

    if (ImGui::Button("New Project", ImVec2(buttonWidth, buttonHeight))) {
        if (m_onNewProject) m_onNewProject();
    }

    ImGui::Spacing();

    if (ImGui::Button("Open Project", ImVec2(buttonWidth, buttonHeight))) {
        if (m_onOpenProject) m_onOpenProject();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Import Model", ImVec2(buttonWidth, buttonHeight))) {
        if (m_onImportModel) m_onImportModel();
    }
}

}  // namespace dw
