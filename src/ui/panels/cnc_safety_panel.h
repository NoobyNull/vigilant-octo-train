#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_types.h"
#include "../../core/cnc/preflight_check.h"
#include "../../core/types.h"
#include "panel.h"

namespace dw {

class CncController;

// CNC safety control panel -- Pause/Resume/Abort buttons, abort confirmation
// dialog, sensor pin display from Pn: field. Primary operator safety interface.
class CncSafetyPanel : public Panel {
  public:
    CncSafetyPanel();
    ~CncSafetyPanel() override = default;

    void render() override;

    // Callbacks -- called on main thread via MainThreadQueue
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);

    // Dependencies
    void setCncController(CncController* ctrl) { m_cnc = ctrl; }

    // Streaming state management (set by application wiring)
    void setStreaming(bool streaming) { m_streaming = streaming; }

    // Program data for resume-from-line (set by application wiring)
    void setProgram(const std::vector<std::string>& lines) { m_fullProgram = lines; }
    bool hasProgram() const { return !m_fullProgram.empty(); }
    const std::vector<std::string>& program() const { return m_fullProgram; }

    // G-code bounds for draw-outline feature
    void setProgramBounds(const Vec3& bmin, const Vec3& bmax) {
        m_boundsMin = bmin;
        m_boundsMax = bmax;
        m_hasBounds = true;
    }

    // Door interlock query — true when door is active AND interlock is enabled
    bool isDoorInterlockActive() const;

  private:
    void renderSafetyControls();
    void renderSensorDisplay();
    void renderDrawOutline();
    void renderAbortConfirmDialog();
    void renderResumeDialog();

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;
    bool m_streaming = false;
    bool m_showAbortConfirm = false;
    bool m_doorActive = false;

    // Pause-before-reset abort sequence state
    bool m_abortPending = false;
    float m_abortTimer = 0.0f;

    // Program lines cached for resume-from-line feature
    std::vector<std::string> m_fullProgram;

    // Resume-from-line state
    bool m_showResumeDialog = false;
    int m_resumeLine = 1;                     // 1-based for user display
    std::vector<std::string> m_preambleLines; // Generated preamble preview
    bool m_preambleGenerated = false;

    // Pre-flight state for resume dialog
    std::vector<PreflightIssue> m_preflightIssues;

    // Draw outline state
    Vec3 m_boundsMin{0.0f};
    Vec3 m_boundsMax{0.0f};
    bool m_hasBounds = false;
    float m_outlineSafeZ = 5.0f;     // Safe Z height for outline (mm above work zero)
    float m_outlineFeedRate = 1000.0f; // Feed rate for outline traverse (mm/min)

    // Probe workflows (EXT-11, EXT-15, EXT-16)
    void renderProbeDialog();
    void renderZProbeTab();
    void renderTlsTab();
    void render3DProbeTab();
    bool m_probeDialogOpen = false;

    // Z-probe parameters
    float m_probeApproachSpeed = 100.0f;
    float m_probePlateThickness = 0.0f;
    float m_probeRetractDist = 2.0f;
    float m_probeSearchDist = 50.0f;

    // TLS parameters
    float m_tlsApproachSpeed = 50.0f;
    float m_tlsSearchDist = 100.0f;
    float m_tlsReferenceZ = 0.0f;

    // 3D probe parameters
    float m_3dProbeSpeed = 100.0f;
    float m_3dProbeRetract = 5.0f;
    float m_3dProbeSearchDist = 50.0f;
    int m_3dProbeMode = 0;
};

} // namespace dw
