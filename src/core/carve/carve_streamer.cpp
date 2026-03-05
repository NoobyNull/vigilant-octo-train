#include "carve_streamer.h"

#include "../cnc/cnc_controller.h"

#include <cstdio>

namespace dw {
namespace carve {

namespace {

// Format float with minimal trailing zeros (consistent with gcode_export.cpp)
std::string fmt(f32 v)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", static_cast<double>(v));
    std::string s(buf);
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        auto last = s.find_last_not_of('0');
        if (last == dot) {
            s.erase(dot + 2); // Keep at least one decimal
        } else {
            s.erase(last + 1);
        }
    }
    return s;
}

} // anonymous namespace

void CarveStreamer::setCncController(CncController* cnc)
{
    m_cnc = cnc;
}

void CarveStreamer::start(const MultiPassToolpath& toolpath,
                          const ToolpathConfig& config)
{
    m_toolpath = toolpath;
    m_config = config;
    m_pointIndex = 0;
    m_lineNumber = 0;
    m_lastFeedRate = -1.0f;
    m_aborted.store(false, std::memory_order_release);
    m_paused.store(false, std::memory_order_release);

    // Compute total lines: preamble(1) + clearing + finishing + postamble(3: retract + M5 + M30)
    int clearingCount = static_cast<int>(toolpath.clearing.points.size());
    int finishingCount = static_cast<int>(toolpath.finishing.points.size());
    m_totalLines = 1 + clearingCount + finishingCount + 3;

    // Determine starting phase
    if (clearingCount > 0) {
        m_phase = Phase::Preamble;
    } else if (finishingCount > 0) {
        m_phase = Phase::Preamble;
    } else {
        m_phase = Phase::Complete;
        m_totalLines = 0;
        m_running.store(false, std::memory_order_release);
        return;
    }

    m_running.store(true, std::memory_order_release);
}

std::string CarveStreamer::nextLine()
{
    if (m_aborted.load(std::memory_order_acquire)) {
        return {};
    }

    if (m_paused.load(std::memory_order_acquire)) {
        return {};
    }

    if (m_phase == Phase::Complete) {
        return {};
    }

    // Preamble: G90 G21 (absolute, metric)
    if (m_phase == Phase::Preamble) {
        m_lineNumber++;
        // Determine next phase after preamble
        if (!m_toolpath.clearing.points.empty()) {
            m_phase = Phase::Clearing;
        } else {
            m_phase = Phase::Finishing;
        }
        m_pointIndex = 0;
        return preamble();
    }

    // Clearing pass
    if (m_phase == Phase::Clearing) {
        if (m_pointIndex < m_toolpath.clearing.points.size()) {
            const auto& pt = m_toolpath.clearing.points[m_pointIndex];
            m_pointIndex++;
            m_lineNumber++;
            if (pt.rapid) {
                return formatRapid(pt.position);
            }
            return formatLinear(pt.position, m_config.feedRateMmMin);
        }
        // Clearing complete, switch to finishing
        m_phase = Phase::Finishing;
        m_pointIndex = 0;
    }

    // Finishing pass
    if (m_phase == Phase::Finishing) {
        if (m_pointIndex < m_toolpath.finishing.points.size()) {
            const auto& pt = m_toolpath.finishing.points[m_pointIndex];
            m_pointIndex++;
            m_lineNumber++;
            if (pt.rapid) {
                return formatRapid(pt.position);
            }
            return formatLinear(pt.position, m_config.feedRateMmMin);
        }
        // Finishing complete, emit postamble
        m_phase = Phase::Postamble;
        m_pointIndex = 0;
    }

    // Postamble: 3 lines (retract, spindle stop, program end)
    if (m_phase == Phase::Postamble) {
        std::string line;
        if (m_pointIndex == 0) {
            line = "G0 Z" + fmt(m_config.safeZMm);
        } else if (m_pointIndex == 1) {
            line = "M5";
        } else {
            line = "M30";
            m_phase = Phase::Complete;
            m_running.store(false, std::memory_order_release);
        }
        m_pointIndex++;
        m_lineNumber++;
        return line;
    }

    return {};
}

void CarveStreamer::pause()
{
    m_paused.store(true, std::memory_order_release);
    if (m_cnc) {
        m_cnc->feedHold();
    }
}

void CarveStreamer::resume()
{
    m_paused.store(false, std::memory_order_release);
    if (m_cnc) {
        m_cnc->cycleStart();
    }
}

void CarveStreamer::abort()
{
    m_aborted.store(true, std::memory_order_release);
    m_running.store(false, std::memory_order_release);
    m_phase = Phase::Complete;
    if (m_cnc) {
        m_cnc->softReset();
    }
}

bool CarveStreamer::isRunning() const
{
    return m_running.load(std::memory_order_acquire);
}

bool CarveStreamer::isPaused() const
{
    return m_paused.load(std::memory_order_acquire);
}

bool CarveStreamer::isComplete() const
{
    return m_phase == Phase::Complete;
}

int CarveStreamer::currentLine() const
{
    return m_lineNumber;
}

int CarveStreamer::totalLines() const
{
    return m_totalLines;
}

f32 CarveStreamer::progressFraction() const
{
    if (m_totalLines <= 0) return 1.0f;
    return static_cast<f32>(m_lineNumber) / static_cast<f32>(m_totalLines);
}

std::string CarveStreamer::formatRapid(const Vec3& pos) const
{
    return "G0 X" + fmt(pos.x) + " Y" + fmt(pos.y) + " Z" + fmt(pos.z);
}

std::string CarveStreamer::formatLinear(const Vec3& pos, f32 feedRate)
{
    std::string line = "G1 X" + fmt(pos.x) + " Y" + fmt(pos.y) + " Z" + fmt(pos.z);
    if (feedRate != m_lastFeedRate) {
        line += " F" + fmt(feedRate);
        m_lastFeedRate = feedRate;
    }
    return line;
}

std::string CarveStreamer::preamble() const
{
    return "G90 G21";
}

std::string CarveStreamer::postamble() const
{
    return "G0 Z" + fmt(m_config.safeZMm) + "\nM5\nM30";
}

} // namespace carve
} // namespace dw
