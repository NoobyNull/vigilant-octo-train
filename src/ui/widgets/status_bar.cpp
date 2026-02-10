// Digital Workshop - Status Bar Widget Implementation

#include "ui/widgets/status_bar.h"

#include <cstdio>

#include <imgui.h>

#include "core/import/import_task.h"

namespace dw {

void StatusBar::render() {
    auto* viewport = ImGui::GetMainViewport();
    float barHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2;

    ImVec2 pos(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - barHeight);
    ImVec2 size(viewport->WorkSize.x, barHeight);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    if (ImGui::Begin("##StatusBar", nullptr, flags)) {
        if (m_progress && m_progress->active.load()) {
            // Show import progress
            int completed = m_progress->completedFiles.load();
            int total = m_progress->totalFiles.load();
            int failed = m_progress->failedFiles.load();

            // Left: current file and stage
            std::string currentFile = m_progress->getCurrentFileName();
            if (!currentFile.empty()) {
                ImGui::Text("%s", currentFile.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("- %s", importStageName(m_progress->currentStage.load()));
            }

            // Right: progress bar and counts
            float progressBarWidth = 200.0f;
            float textWidth = ImGui::CalcTextSize("999/999").x;
            float totalWidth = progressBarWidth + textWidth + 16; // spacing
            ImGui::SameLine(ImGui::GetWindowWidth() - totalWidth - 8);

            // Progress bar
            float fraction =
                total > 0 ? static_cast<float>(completed) / static_cast<float>(total) : 0.0f;
            char overlay[64];
            std::snprintf(overlay, sizeof(overlay), "%d/%d", completed, total);
            ImGui::ProgressBar(fraction, ImVec2(progressBarWidth, 0), overlay);

            // Cancel button
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                // Note: We don't have direct access to ImportQueue here.
                // Application wiring will handle cancellation if needed.
                // For now, this is a visual placeholder.
            }

            // Show failed count if > 0
            if (failed > 0) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%d failed", failed);
            }
        } else {
            // No active import — show general status
            ImGui::TextDisabled("Ready");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void StatusBar::setImportProgress(const ImportProgress* progress) {
    m_progress = progress;
}

void StatusBar::clearImportProgress() {
    m_progress = nullptr;
}

} // namespace dw
