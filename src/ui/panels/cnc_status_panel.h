#pragma once

#include <string>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

namespace dw {

// Real-time CNC status display panel — DRO, state indicator, feed/spindle readout.
// Receives MachineStatus updates via callbacks from CncController (main thread).
class CncStatusPanel : public Panel {
  public:
    CncStatusPanel();
    ~CncStatusPanel() override = default;

    void render() override;

    // Callbacks — called on main thread via MainThreadQueue, no mutex needed
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);

  private:
    void renderStateIndicator();
    void renderDRO();
    void renderFeedSpindle();
    void renderOverrides();

    MachineStatus m_status{};
    bool m_connected = false;
    std::string m_version;
};

} // namespace dw
