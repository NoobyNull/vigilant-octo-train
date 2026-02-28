#pragma once

#include "toolpath_types.h"

#include <atomic>
#include <string>

namespace dw {

class CncController;

namespace carve {

// Streams toolpath to CncController point-by-point.
// Generates G-code from toolpath data on demand, avoiding
// building a complete file in memory.
class CarveStreamer {
public:
    CarveStreamer() = default;

    // Set dependencies
    void setCncController(CncController* cnc);

    // Start streaming a toolpath
    void start(const MultiPassToolpath& toolpath,
               const ToolpathConfig& config);

    // Called by CncController when ready for next line.
    // Returns empty string when complete.
    std::string nextLine();

    // Job control
    void pause();
    void resume();
    void abort();

    // State
    bool isRunning() const;
    bool isPaused() const;
    bool isComplete() const;

    // Progress
    int currentLine() const;
    int totalLines() const;
    f32 progressFraction() const; // [0.0, 1.0]

private:
    CncController* m_cnc = nullptr;

    // Toolpath data (copied on start)
    MultiPassToolpath m_toolpath;
    ToolpathConfig m_config;

    // Current position in toolpath
    enum class Phase { Preamble, Clearing, Finishing, Postamble, Complete };
    Phase m_phase = Phase::Complete;
    size_t m_pointIndex = 0;
    int m_lineNumber = 0;
    int m_totalLines = 0;

    // Feed rate tracking for modal optimization
    f32 m_lastFeedRate = -1.0f;

    // State
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_aborted{false};

    // G-code generation helpers
    std::string formatRapid(const Vec3& pos) const;
    std::string formatLinear(const Vec3& pos, f32 feedRate);
    std::string preamble() const;
    std::string postamble() const;
};

} // namespace carve
} // namespace dw
