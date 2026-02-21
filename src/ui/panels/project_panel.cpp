#include "project_panel.h"

#include <imgui.h>

#include "../../core/config/config.h"
#include "../icons.h"
#include "../widgets/toast.h"

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

void ProjectPanel::renderProjectInfo() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        return;
    }

    if (ImGui::CollapsingHeader("Project Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Project name with modified indicator
        std::string nameLabel = project->name();
        if (project->isModified()) {
            nameLabel += " *";
        }
        ImGui::TextWrapped("%s %s", Icons::Project, nameLabel.c_str());

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
            bool success = false;
            if (m_saveProjectCallback) {
                m_saveProjectCallback();
                success = true;
            } else {
                try {
                    m_projectManager->save(*project);
                    success = true;
                } catch (...) {
                    success = false;
                }
            }

            if (success) {
                ToastManager::instance().show(ToastType::Success, "Saved",
                                             "Project saved successfully");
            } else {
                ToastManager::instance().show(ToastType::Error, "Save Failed",
                                             "Could not save project");
            }
        }

        // Wrap Close button to new line if narrow
        if (ImGui::GetContentRegionAvail().x > 100.0f) {
            ImGui::SameLine();
        }

        if (ImGui::Button("Close")) {
            if (project->isModified()) {
                ImGui::OpenPopup("Unsaved Changes");
            } else {
                m_projectManager->close(*project);
                m_projectManager->setCurrentProject(nullptr);
            }
        }

        // Unsaved changes confirmation dialog
        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Save changes before closing?");
            ImGui::TextDisabled("You have unsaved changes in this project.");
            ImGui::Spacing();

            if (ImGui::Button("Save & Close", ImVec2(150, 0))) {
                m_projectManager->save(*project);
                m_projectManager->close(*project);
                m_projectManager->setCurrentProject(nullptr);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard", ImVec2(150, 0))) {
                m_projectManager->close(*project);
                m_projectManager->setCurrentProject(nullptr);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(150, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Unindent();
    }
}

void ProjectPanel::renderModelList() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        return;
    }

    if (ImGui::CollapsingHeader("Project Models", ImGuiTreeNodeFlags_DefaultOpen)) {
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
                    Icons::Model + std::string(" Model #") + std::to_string(modelId);

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

    // Wrap Open button to new line if narrow
    if (ImGui::GetContentRegionAvail().x > 100.0f) {
        ImGui::SameLine();
    }

    if (ImGui::Button("Open Project")) {
        if (m_openProjectCallback) {
            m_openProjectCallback();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Recent projects
    ImGui::Text("Recent Projects");
    ImGui::Spacing();

    const auto& recentProjects = Config::instance().getRecentProjects();

    if (recentProjects.empty()) {
        ImGui::TextDisabled("No recent projects.");
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

            ImGui::SameLine();
            ImGui::TextDisabled("%s", projectPath.parent_path().string().c_str());

            ImGui::PopID();
        }
    }
}

} // namespace dw
