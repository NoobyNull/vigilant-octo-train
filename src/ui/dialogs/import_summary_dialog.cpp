// Digital Workshop - Import Summary Dialog Implementation

#include "ui/dialogs/import_summary_dialog.h"

#include <algorithm>

#include <imgui.h>

#include "core/utils/file_utils.h"
#include "ui/icons.h"

namespace dw {

ImportSummaryDialog::ImportSummaryDialog() : Dialog("Import Complete") {}

void ImportSummaryDialog::open(const ImportBatchSummary& summary) {
    m_summary = summary;
    m_checked.assign(m_summary.duplicates.size(), true); // default: all checked
    m_open = true;
    ImGui::OpenPopup(m_title.c_str());
}

void ImportSummaryDialog::setOnReimport(ReimportCallback callback) {
    m_onReimport = std::move(callback);
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
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Duplicates found: %d",
                               m_summary.duplicateCount);
        }

        if (m_summary.failedCount > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Errors: %d", m_summary.failedCount);
        }

        ImGui::Spacing();

        // Interactive duplicates section
        if (m_summary.duplicateCount > 0 && !m_summary.duplicates.empty()) {
            if (ImGui::CollapsingHeader("Duplicates — Select to Re-import",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();

                // Select All / Deselect All helpers
                if (ImGui::SmallButton("Select All")) {
                    std::fill(m_checked.begin(), m_checked.end(), true);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Deselect All")) {
                    std::fill(m_checked.begin(), m_checked.end(), false);
                }

                ImGui::Spacing();

                // Per-duplicate checkboxes
                for (size_t i = 0; i < m_summary.duplicates.size(); ++i) {
                    const auto& dup = m_summary.duplicates[i];
                    auto stem = file::getStem(dup.sourcePath);
                    std::string label = stem + "  ->  duplicate of: " + dup.existingName + "##dup" +
                                        std::to_string(i);
                    bool checked = m_checked[i];
                    if (ImGui::Checkbox(label.c_str(), &checked)) {
                        m_checked[i] = checked;
                    }
                }

                ImGui::Unindent();
            }

            ImGui::Spacing();

            // Count selected
            int selectedCount = 0;
            for (bool c : m_checked)
                if (c)
                    selectedCount++;

            // Re-import / Skip buttons
            std::string reimportLabel =
                "Re-import Selected (" + std::to_string(selectedCount) + ")";
            bool hasSelection = selectedCount > 0;

            if (!hasSelection)
                ImGui::BeginDisabled();
            if (ImGui::Button(reimportLabel.c_str(), ImVec2(200, 0))) {
                // Collect checked duplicates and invoke callback
                std::vector<DuplicateRecord> selected;
                for (size_t i = 0; i < m_summary.duplicates.size(); ++i) {
                    if (m_checked[i])
                        selected.push_back(m_summary.duplicates[i]);
                }
                if (m_onReimport && !selected.empty())
                    m_onReimport(std::move(selected));
                m_open = false;
                ImGui::CloseCurrentPopup();
            }
            if (!hasSelection)
                ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Skip All", ImVec2(120, 0))) {
                m_open = false;
                ImGui::CloseCurrentPopup();
            }
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

        // If no duplicates, show simple OK button
        if (m_summary.duplicateCount == 0) {
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                m_open = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

} // namespace dw
