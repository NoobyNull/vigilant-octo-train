#pragma once

#include <string>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

namespace dw {

// CNC job progress panel — displays streaming progress, time estimation,
// line counts, and feed rate deviation warnings during G-code streaming.
// Receives callbacks from CncController via MainThreadQueue.
class CncJobPanel : public Panel {
  public:
    CncJobPanel();
    ~CncJobPanel() override = default;

    void render() override;

    // Callbacks — called on main thread via MainThreadQueue
    void onStatusUpdate(const MachineStatus& status);
    void onProgressUpdate(const StreamProgress& progress);

    // Streaming state management
    void setStreaming(bool streaming);

    // Recommended feed rate from calculator (set by wiring code)
    void setRecommendedFeedRate(float rate) { m_recommendedFeedRate = rate; }

  private:
    void renderProgressBar();
    void renderLineInfo();
    void renderTimeInfo();
    void renderFeedDeviation();

    static std::string formatTime(float seconds);

    MachineStatus m_status{};
    StreamProgress m_progress{};
    bool m_streaming = false;

    // Feed deviation (populated by onStatusUpdate, rendered by renderFeedDeviation)
    float m_recommendedFeedRate = 0.0f;
    float m_feedDeviation = 0.0f;
    bool m_feedDeviationWarning = false;
};

} // namespace dw
