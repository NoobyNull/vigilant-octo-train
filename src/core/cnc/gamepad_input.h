#pragma once

#include <string>

struct _SDL_GameController; // Forward declaration (SDL uses typedef)
typedef struct _SDL_GameController SDL_GameController;

namespace dw {

class CncController;

// Maps gamepad axes to jog movement and buttons to CNC actions.
// Polls SDL_GameController state each frame from the main loop.
class GamepadInput {
  public:
    GamepadInput();
    ~GamepadInput();

    GamepadInput(const GamepadInput&) = delete;
    GamepadInput& operator=(const GamepadInput&) = delete;

    // Set the CNC controller to send commands to
    void setCncController(CncController* cnc) { m_cnc = cnc; }

    // Poll gamepad state and send commands. Call once per frame.
    void update(float dt);

    // Check if a gamepad is connected
    bool isConnected() const { return m_controller != nullptr; }
    std::string controllerName() const;

    // Enable/disable gamepad input
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

  private:
    void tryOpen();
    void close();
    void processAxes(float dt);
    void processButtons();

    SDL_GameController* m_controller = nullptr;
    CncController* m_cnc = nullptr;
    bool m_enabled = false;
    int m_deviceIndex = -1;

    // Axis state for jog (deadzone filtered)
    float m_axisX = 0.0f;     // Left stick X -> X jog
    float m_axisY = 0.0f;     // Left stick Y -> Y jog
    float m_axisZ = 0.0f;     // Right stick Y -> Z jog
    float m_jogTimer = 0.0f;  // Accumulator for jog command rate limiting

    // Button debounce (prevent repeated triggers)
    bool m_prevStart = false;
    bool m_prevBack = false;
    bool m_prevA = false;
    bool m_prevB = false;
    bool m_prevHome = false;

    // Tuning
    static constexpr float DEADZONE = 0.15f;        // 15% deadzone
    static constexpr float JOG_INTERVAL = 0.1f;     // Send jog every 100ms
    static constexpr float JOG_FEED_SLOW = 500.0f;  // mm/min at low stick deflection
    static constexpr float JOG_FEED_FAST = 3000.0f;  // mm/min at full stick deflection
    static constexpr float JOG_DISTANCE = 100.0f;   // mm per jog command (will be cancelled)
};

} // namespace dw
