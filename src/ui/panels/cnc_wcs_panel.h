#pragma once

#include <string>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

namespace dw {

class CncController;

// Work coordinate system panel — zero-set buttons, G54-G59 selector, offset display.
class CncWcsPanel : public Panel {
  public:
    CncWcsPanel();
    ~CncWcsPanel() override = default;

    void render() override;

    void setCncController(CncController* cnc) { m_cnc = cnc; }
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);
    void onRawLine(const std::string& line, bool isSent);

  private:
    void renderZeroButtons();
    void renderWcsSelector();
    void renderOffsetDisplay();
    void renderConfirmationPopup();
    void requestOffsets();

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;

    // Active WCS: 0=G54, 1=G55, ..., 5=G59
    int m_activeWcs = 0;
    static constexpr const char* WCS_NAMES[] = {"G54", "G55", "G56", "G57", "G58", "G59"};
    static constexpr int NUM_WCS = 6;

    // Stored offsets from $# query
    WcsOffsets m_offsets{};
    bool m_offsetsLoaded = false;

    // Confirmation popup state
    bool m_confirmZeroOpen = false;
    std::string m_confirmZeroLabel;
    std::string m_confirmZeroCmd;

    // $# response parsing state
    bool m_parsingOffsets = false;
    WcsOffsets m_pendingOffsets{};
    int m_parsedWcsCount = 0;
};

} // namespace dw
