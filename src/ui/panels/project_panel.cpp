#include "project_panel.h"

#include "../icons.h"

#include <imgui.h>

namespace dw {

ProjectPanel::ProjectPanel(ProjectManager* projectManager)
    : Panel("Project"), m_projectManager(projectManager) {}

void ProjectPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        if (m_projectManager && m_projectManager->currentProject()) {
            renderProjectInfo();
            ImGui::Separator();
            renderModelList();
        } else {
            renderNoProject();
        }
    }
    ImGui::End();
}

void ProjectPanel::refresh() {
    // Force UI update on next frame
}

void ProjectPanel::renderProjectInfo() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        return;
    }

    if (ImGui::CollapsingHeader("Project Info",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Project name with modified indicator
        std::string nameLabel = project->name();
        if (project->isModified()) {
            nameLabel += " *";
        }
        ImGui::Text("%s %s", Icons::Project, nameLabel.c_str());

        // Description
        if (!project->description().empty()) {
            ImGui::TextWrapped("%s", project->description().c_str());
        }

        // Model count
        size_t modelCount = project->modelIds().size();
        ImGui::Text("Models: %zu", modelCount);

        ImGui::Spacing();

        // Quick actions
        if (ImGui::Button("Save")) {
            m_projectManager->save(*project);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            m_projectManager->close(*project);
            m_projectManager->setCurrentProject(nullptr);
        }

        ImGui::Unindent();
    }
}

void ProjectPanel::renderModelList() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        return;
    }

    if (ImGui::CollapsingHeader("Project Models",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        const auto& modelIds = project->modelIds();

        if (modelIds.empty()) {
            ImGui::TextDisabled("No models in project");
            ImGui::TextDisabled("Add models from the Library panel");
        } else {
            for (int64_t modelId : modelIds) {
                ImGui::PushID(static_cast<int>(modelId));

                bool isSelected = (modelId == m_selectedModelId);
                std::string label =
                    Icons::Model + std::string(" Model #") +
                    std::to_string(modelId);

                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    m_selectedModelId = modelId;
                    if (m_onModelSelected) {
                        m_onModelSelected(modelId);
                    }
                }

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Remove from Project")) {
                        project->removeModel(modelId);
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
        }

        ImGui::Unindent();
    }
}

void ProjectPanel::renderNoProject() {
    ImGui::TextDisabled("No project open");
    ImGui::Spacing();

    if (ImGui::Button("New Project")) {
        if (m_projectManager) {
            auto project = m_projectManager->create("New Project");
            m_projectManager->setCurrentProject(project);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Open Project")) {
        // TODO: Show project open dialog
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Recent projects
    ImGui::Text("Recent Projects");
    ImGui::TextDisabled("(Not yet implemented)");
}

}  // namespace dw
