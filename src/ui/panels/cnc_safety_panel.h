#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_types.h"
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

  private:
    void renderSafetyControls();
    void renderSensorDisplay();
    void renderAbortConfirmDialog();

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;
    bool m_streaming = false;
    bool m_showAbortConfirm = false;

    // Program lines cached for resume-from-line feature (Plan 04)
    std::vector<std::string> m_fullProgram;
};

} // namespace dw
