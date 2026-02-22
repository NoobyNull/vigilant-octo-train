#include "progress_dialog.h"

#include <imgui.h>

namespace dw {

ProgressDialog::ProgressDialog() : Dialog("Progress") {}

void ProgressDialog::start(const std::string& title, int total, bool cancellable) {
    m_title = title;
    m_total.store(total, std::memory_order_relaxed);
    m_completed.store(0, std::memory_order_relaxed);
    m_cancelled.store(false, std::memory_order_relaxed);
    m_cancellable = cancellable;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentItem.clear();
    }
    m_pendingOpen = true;
    m_open = true;
}

void ProgressDialog::advance(const std::string& currentItem) {
    m_completed.fetch_add(1, std::memory_order_relaxed);
    if (!currentItem.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentItem = currentItem;
    }
}

bool ProgressDialog::isCancelled() const {
    return m_cancelled.load(std::memory_order_relaxed);
}

void ProgressDialog::finish() {
    m_completed.store(m_total.load(std::memory_order_relaxed), std::memory_order_relaxed);
    m_open = false;
    m_pendingOpen = false;
}

void ProgressDialog::render() {
    if (!m_open) {
        return;
    }

    if (m_pendingOpen) {
        ImGui::OpenPopup(m_title.c_str());
        m_pendingOpen = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 0));

    if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, ImGuiWindowFlags_NoMove)) {
        int completed = m_completed.load(std::memory_order_relaxed);
        int total = m_total.load(std::memory_order_relaxed);

        // Status text
        ImGui::Text("Processing %d of %d...", completed, total);
        ImGui::Spacing();

        // Progress bar
        float fraction = total > 0 ? static_cast<float>(completed) / static_cast<float>(total)
                                   : 0.0f;
        ImGui::ProgressBar(fraction, ImVec2(-1, 0));
        ImGui::Spacing();

        // Current item name
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_currentItem.empty()) {
                ImGui::TextDisabled("%s", m_currentItem.c_str());
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Cancel button (centered)
        if (m_cancellable) {
            float buttonWidth = 120.0f;
            float contentWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX((contentWidth - buttonWidth) / 2);
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
                m_cancelled.store(true, std::memory_order_relaxed);
            }
        }

        // Auto-close when done
        if (completed >= total && total > 0) {
            ImGui::CloseCurrentPopup();
            m_open = false;
        }

        ImGui::EndPopup();
    }
}

} // namespace dw
