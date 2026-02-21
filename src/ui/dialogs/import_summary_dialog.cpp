// Digital Workshop - Import Summary Dialog Implementation

#include "ui/dialogs/import_summary_dialog.h"

#include <imgui.h>

namespace dw {

ImportSummaryDialog::ImportSummaryDialog() : Dialog("Import Complete") {}

void ImportSummaryDialog::open(const ImportBatchSummary& summary) {
    m_summary = summary;
    m_open = true;
    ImGui::OpenPopup(m_title.c_str());
}

void ImportSummaryDialog::render() {
    if (!m_open)
        return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Appearing);

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::BeginPopupModal(m_title.c_str(), &m_open, flags)) {
        // Header
        if (m_summary.hasIssues()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Import Complete (with issues)");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Import Complete");
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Stats section
        ImGui::Text("Total files: %d", m_summary.totalFiles);
        ImGui::Text("Successfully imported: %d", m_summary.successCount);

        if (m_summary.duplicateCount > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Duplicates skipped: %d",
                               m_summary.duplicateCount);
        }

        if (m_summary.failedCount > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Errors: %d", m_summary.failedCount);
        }

        ImGui::Spacing();

        // Duplicates section (collapsible)
        if (m_summary.duplicateCount > 0 && !m_summary.duplicateNames.empty()) {
            if (ImGui::CollapsingHeader("Skipped Duplicates", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                for (const auto& name : m_summary.duplicateNames) {
                    ImGui::BulletText("%s", name.c_str());
                }
                ImGui::Unindent();
            }
            ImGui::Spacing();
        }

        // Errors section (collapsible)
        if (m_summary.failedCount > 0 && !m_summary.errors.empty()) {
            if (ImGui::CollapsingHeader("Errors", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                for (const auto& [filename, error] : m_summary.errors) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::BulletText("%s", filename.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Indent();
                    ImGui::TextWrapped("%s", error.c_str());
                    ImGui::Unindent();
                    ImGui::Spacing();
                }
                ImGui::Unindent();
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // OK button
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_open = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace dw
