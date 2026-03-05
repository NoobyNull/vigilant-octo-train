#pragma once

#include <string>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

namespace dw {

class CncController;

// Real-time CNC status display panel — DRO, state indicator, feed/spindle readout,
// override controls, coolant toggles, and alarm display.
// Receives MachineStatus updates via callbacks from CncController (main thread).
class CncStatusPanel : public Panel {
  public:
    CncStatusPanel();
    ~CncStatusPanel() override = default;

    void render() override;

    // Wire the CncController for sending commands
    void setCncController(CncController* cnc) { m_cnc = cnc; }

    // Callbacks — called on main thread via MainThreadQueue, no mutex needed
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);
    void onAlarm(int alarmCode, const std::string& desc);

  private:
    void renderStateIndicator();
    void renderDRO();
    void renderFeedSpindle();
    void renderOverrideControls();
    void renderCoolantControls();
    void renderAlarmBanner();
    void renderProbeIndicator();
    void renderWcsSelector();
    void renderMoveToDialog();

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;
    std::string m_version;
    int m_lastAlarmCode = 0;
    std::string m_lastAlarmDesc;

    // WCS quick-switch: 0=G54, 1=G55, ..., 5=G59
    int m_activeWcs = 0;
    static constexpr const char* WCS_NAMES[] = {"G54", "G55", "G56", "G57", "G58", "G59"};
    static constexpr int NUM_WCS = 6;

    // WCS alias editing
    int m_editingWcsIdx = -1;
    char m_wcsAliasBuf[64] = {};

    // Move-To dialog state
    bool m_moveToOpen = false;
    float m_moveToX = 0.0f;
    float m_moveToY = 0.0f;
    float m_moveToZ = 0.0f;
    bool m_moveToUseG0 = true; // true=G0 rapid, false=G1 feed move
};

} // namespace dw
