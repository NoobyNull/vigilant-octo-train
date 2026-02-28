#include "ui/dialogs/maintenance_dialog.h"

#include <imgui.h>

namespace dw {

MaintenanceDialog::MaintenanceDialog() : Dialog("Library Maintenance") {}

void MaintenanceDialog::render() {
    if (!m_open)
        return;

    const auto* viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x * 0.3f, 0), ImGuiCond_Appearing);

    if (!ImGui::Begin(m_title.c_str(),
                      &m_open,
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    switch (m_state) {
    case State::Confirm: {
        ImGui::TextWrapped("This will:");
        ImGui::Spacing();
        ImGui::BulletText("Split compound categories");
        ImGui::BulletText("Remove empty categories");
        ImGui::BulletText("Deduplicate model tags");
        ImGui::BulletText("Verify thumbnail paths");
        ImGui::BulletText("Rebuild search index");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 6;
        float totalWidth = buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
        float offset = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        if (offset > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

        if (ImGui::Button("Run", ImVec2(buttonWidth, 0))) {
            m_state = State::Running;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
            close();
            m_state = State::Confirm;
        }
        break;
    }

    case State::Running: {
        ImGui::TextWrapped("Running maintenance...");
        // Execute synchronously (fast DB operations)
        if (m_onRun) {
            m_report = m_onRun();
        }
        m_state = State::Done;
        break;
    }

    case State::Done: {
        ImGui::Text("Results:");
        ImGui::Spacing();
        ImGui::BulletText("%d compound categor%s split",
                          m_report.categoriesSplit,
                          m_report.categoriesSplit == 1 ? "y" : "ies");
        ImGui::BulletText("%d empty categor%s removed",
                          m_report.categoriesRemoved,
                          m_report.categoriesRemoved == 1 ? "y" : "ies");
        ImGui::BulletText("%d model%s had duplicate tags cleaned",
                          m_report.tagsDeduped,
                          m_report.tagsDeduped == 1 ? "" : "s");
        ImGui::BulletText("%d broken thumbnail path%s cleared",
                          m_report.thumbnailsCleared,
                          m_report.thumbnailsCleared == 1 ? "" : "s");
        if (m_report.ftsRebuilt) {
            ImGui::BulletText("Search index rebuilt");
        } else {
            ImGui::BulletText("Search index rebuild skipped");
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 6;
        float offset = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
        if (offset > 0)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
            close();
            m_state = State::Confirm;
        }
        break;
    }
    }

    ImGui::End();
}

} // namespace dw
