#pragma once

#include <string>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

namespace dw {

class CncController;

// Jog control panel — XYZ jog buttons with step sizes, homing, and unlock.
// Receives MachineStatus updates via callbacks from CncController (main thread).
class CncJogPanel : public Panel {
  public:
    CncJogPanel();
    ~CncJogPanel() override = default;

    void render() override;

    void setCncController(CncController* cnc) { m_cnc = cnc; }
    void onStatusUpdate(const MachineStatus& status);
    void onConnectionChanged(bool connected, const std::string& version);

    // Called by UIManager::handleCncKeyboardJog() each frame
    void handleKeyboardJog();

  private:
    void renderStepSizeSelector();
    void renderJogButtons();
    void renderHomingSection();

    void jogAxis(int axis, float direction);
    void startContinuousJog(int axis, float direction, int key);
    void stopContinuousJog();

    CncController* m_cnc = nullptr;
    MachineStatus m_status{};
    bool m_connected = false;

    // Step sizes in mm: 0.1, 1, 10, 100
    static constexpr float STEP_SIZES[] = {0.1f, 1.0f, 10.0f, 100.0f};
    static constexpr const char* STEP_LABELS[] = {"0.1", "1", "10", "100"};
    static constexpr int NUM_STEPS = 4;
    int m_selectedStep = 1; // Default to 1mm

    // Jog feed rates matched to step sizes (mm/min)
    static constexpr float JOG_FEEDS[] = {500.0f, 1000.0f, 2000.0f, 3000.0f};

    // Continuous jog tracking
    int m_contJogAxis = -1;       // -1 = not jogging, 0=X, 1=Y, 2=Z
    float m_contJogDir = 0.0f;    // +1 or -1
    int m_contJogKey = 0;         // ImGuiKey value
    float m_jogWatchdogTimer = 0.0f; // Dead-man watchdog timer (ms)
    static constexpr float CONTINUOUS_JOG_FEED = 2000.0f;
    static constexpr float CONTINUOUS_JOG_DISTANCE = 10000.0f;
};

} // namespace dw
