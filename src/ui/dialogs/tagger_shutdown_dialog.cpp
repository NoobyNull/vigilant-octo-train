#include "tagger_shutdown_dialog.h"

#include <imgui.h>

#include "core/import/background_tagger.h"

namespace dw {

TaggerShutdownDialog::TaggerShutdownDialog() : Dialog("Tagging In Progress") {}

void TaggerShutdownDialog::open(const TaggerProgress* progress) {
    m_progress = progress;
    m_open = true;
    ImGui::OpenPopup(m_title.c_str());
}

void TaggerShutdownDialog::setOnQuit(std::function<void()> callback) {
    m_onQuit = std::move(callback);
}

void TaggerShutdownDialog::render() {
    if (!m_open)
        return;

    // Auto-close if tagger finished
    if (m_progress && !m_progress->active.load()) {
        m_open = false;
        if (m_onQuit)
            m_onQuit();
        return;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

    if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, flags)) {
        if (m_progress) {
            int completed = m_progress->completed.load();
            int total = m_progress->totalUntagged.load();
            ImGui::Text("Tagging in progress (%d / %d)", completed, total);
            ImGui::Spacing();

            // Show current model name
            {
                std::lock_guard<std::mutex> lock(
                    const_cast<std::mutex&>(m_progress->nameMutex));
                if (m_progress->currentModel[0] != '\0')
                    ImGui::TextWrapped("Current: %s", m_progress->currentModel);
            }

            // Progress bar
            float frac = total > 0 ? static_cast<float>(completed) / static_cast<float>(total) : 0.0f;
            ImGui::ProgressBar(frac, ImVec2(-1, 0));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Cancel tagging & quit", ImVec2(-1, 0))) {
            m_open = false;
            ImGui::CloseCurrentPopup();
            if (m_onQuit)
                m_onQuit();
        }

        ImGui::EndPopup();
    }
}

} // namespace dw
