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

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;
    std::string m_version;
    int m_lastAlarmCode = 0;
    std::string m_lastAlarmDesc;
};

} // namespace dw
