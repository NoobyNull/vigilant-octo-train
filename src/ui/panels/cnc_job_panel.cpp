#include "ui/panels/cnc_job_panel.h"

#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

CncJobPanel::CncJobPanel() : Panel("Job Progress") {}

void CncJobPanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_streaming && m_progress.totalLines == 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("No active job");
        ImGui::TextDisabled("Load and stream G-code to see progress");
        ImGui::End();
        return;
    }

    renderProgressBar();
    ImGui::Spacing();
    renderLineInfo();
    ImGui::Spacing();
    renderTimeInfo();
    ImGui::Spacing();
    renderFeedDeviation();

    ImGui::End();
}

void CncJobPanel::renderProgressBar() {
    if (m_progress.totalLines <= 0)
        return;

    float fraction = static_cast<float>(m_progress.ackedLines) /
                     static_cast<float>(m_progress.totalLines);

    // Progress bar with percentage overlay
    char overlay[64];
    std::snprintf(overlay, sizeof(overlay), "%.1f%%",
                  static_cast<double>(fraction * 100.0f));
    ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);

    // Color-coded percentage text below bar
    ImVec4 pctColor;
    if (fraction >= 0.75f)
        pctColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f); // Green
    else if (fraction >= 0.25f)
        pctColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
    else
        pctColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // White/gray

    ImGui::TextColored(pctColor, "%.1f%% complete",
                       static_cast<double>(fraction * 100.0f));
}

void CncJobPanel::renderLineInfo() {
    ImGui::SeparatorText("Progress");

    ImGui::Text("Line:");
    ImGui::SameLine(100);
    ImGui::Text("%d / %d", m_progress.ackedLines, m_progress.totalLines);

    if (m_progress.errorCount > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "%s Errors: %d", Icons::Error, m_progress.errorCount);
    }
}

void CncJobPanel::renderTimeInfo() {
    ImGui::SeparatorText("Time");

    // Elapsed time
    std::string elapsedStr = formatTime(m_progress.elapsedSeconds);
    ImGui::Text("Elapsed:");
    ImGui::SameLine(100);
    ImGui::Text("%s", elapsedStr.c_str());

    // Remaining time estimate
    ImGui::Text("Remaining:");
    ImGui::SameLine(100);

    if (m_progress.ackedLines < 5 || m_progress.elapsedSeconds < 3.0f) {
        // Not enough data for a reliable estimate
        ImGui::TextDisabled("Calculating...");
    } else if (m_progress.ackedLines >= m_progress.totalLines) {
        // Job complete
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Complete");
    } else {
        // Line-rate based ETA — naturally adjusts with feed rate changes
        float rate = static_cast<float>(m_progress.ackedLines) / m_progress.elapsedSeconds;
        float remaining = static_cast<float>(m_progress.totalLines - m_progress.ackedLines);
        float eta = remaining / rate;

        std::string etaStr = formatTime(eta);
        ImGui::Text("~%s", etaStr.c_str());
    }
}

void CncJobPanel::renderFeedDeviation() {
    // Feed deviation rendering implemented in Plan 05-02
    // Member variables m_recommendedFeedRate, m_feedDeviation, m_feedDeviationWarning
    // are in place and populated by onStatusUpdate().
}

void CncJobPanel::onStatusUpdate(const MachineStatus& status) {
    m_status = status;

    // Feed deviation check — only meaningful during Run state while streaming
    if (m_streaming && m_recommendedFeedRate > 0.0f &&
        status.state == MachineState::Run && status.feedRate > 0.0f) {
        // Account for feed override: if user set 50% override,
        // the expected feed is 50% of recommended
        float effectiveRecommended = m_recommendedFeedRate *
            (static_cast<float>(status.feedOverride) / 100.0f);

        if (effectiveRecommended > 0.0f) {
            m_feedDeviation = std::abs(status.feedRate - effectiveRecommended) / effectiveRecommended;
            m_feedDeviationWarning = (m_feedDeviation > 0.20f);
        }
    } else {
        // Not in a state where deviation is meaningful
        m_feedDeviation = 0.0f;
        m_feedDeviationWarning = false;
    }
}

void CncJobPanel::onProgressUpdate(const StreamProgress& progress) {
    m_progress = progress;
}

void CncJobPanel::setStreaming(bool streaming) {
    m_streaming = streaming;
    if (!streaming) {
        // Clear deviation state when streaming stops
        m_feedDeviation = 0.0f;
        m_feedDeviationWarning = false;
    }
}

std::string CncJobPanel::formatTime(float seconds) {
    int totalSec = static_cast<int>(seconds);
    int h = totalSec / 3600;
    int m = (totalSec / 60) % 60;
    int s = totalSec % 60;

    char buf[32];
    if (h > 0) {
        std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    } else {
        std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    }
    return std::string(buf);
}

} // namespace dw
