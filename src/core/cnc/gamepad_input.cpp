// GamepadInput -- Maps SDL_GameController axes to CNC jog movement
// and buttons to start/pause/stop/home actions.

#include "core/cnc/gamepad_input.h"
#include "core/cnc/cnc_controller.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <SDL.h>

#include "core/utils/log.h"

namespace dw {

GamepadInput::GamepadInput() {
    // SDL_Init(SDL_INIT_GAMECONTROLLER) should already be called by Application
    tryOpen();
}

GamepadInput::~GamepadInput() {
    close();
}

void GamepadInput::tryOpen() {
    if (m_controller) return;

    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            m_controller = SDL_GameControllerOpen(i);
            if (m_controller) {
                m_deviceIndex = i;
                log::infof("GamepadInput", "Opened controller: %s",
                           SDL_GameControllerName(m_controller));
                return;
            }
        }
    }
}

void GamepadInput::close() {
    if (m_controller) {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
        m_deviceIndex = -1;
    }
}

std::string GamepadInput::controllerName() const {
    if (!m_controller) return "None";
    const char* name = SDL_GameControllerName(m_controller);
    return name ? name : "Unknown";
}

void GamepadInput::update(float dt) {
    if (!m_enabled) return;

    // Try to open if not connected
    if (!m_controller) {
        tryOpen();
        if (!m_controller) return;
    }

    // Check if still attached (hot-unplug detection)
    if (!SDL_GameControllerGetAttached(m_controller)) {
        log::info("GamepadInput", "Controller disconnected");
        close();
        return;
    }

    processAxes(dt);
    processButtons();
}

void GamepadInput::processAxes(float dt) {
    if (!m_cnc || !m_cnc->isConnected()) return;

    // Read axes (range: -32768 to 32767)
    float lx = static_cast<float>(
        SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX)) / 32767.0f;
    float ly = static_cast<float>(
        SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY)) / 32767.0f;
    float ry = static_cast<float>(
        SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTY)) / 32767.0f;

    // Apply deadzone
    auto applyDeadzone = [](float v) -> float {
        if (std::fabs(v) < DEADZONE) return 0.0f;
        float sign = v > 0 ? 1.0f : -1.0f;
        return sign * (std::fabs(v) - DEADZONE) / (1.0f - DEADZONE);
    };

    m_axisX = applyDeadzone(lx);
    m_axisY = applyDeadzone(-ly); // Invert Y (stick up = positive Y)
    m_axisZ = applyDeadzone(-ry); // Invert Y (stick up = Z+)

    // Rate-limited jog commands
    m_jogTimer += dt;
    if (m_jogTimer < JOG_INTERVAL) return;
    m_jogTimer = 0.0f;

    bool hasMotion = (m_axisX != 0.0f || m_axisY != 0.0f || m_axisZ != 0.0f);
    if (!hasMotion) return;

    // Scale feed rate by stick magnitude
    float magnitude = std::sqrt(m_axisX * m_axisX + m_axisY * m_axisY);
    if (m_axisZ != 0.0f) magnitude = std::max(magnitude, std::fabs(m_axisZ));
    float feed = JOG_FEED_SLOW + (JOG_FEED_FAST - JOG_FEED_SLOW) * std::min(magnitude, 1.0f);

    // Build incremental jog command
    char cmd[128];
    std::snprintf(cmd, sizeof(cmd), "$J=G91 G21 X%.3f Y%.3f Z%.3f F%.0f",
                  static_cast<double>(m_axisX * JOG_DISTANCE * JOG_INTERVAL),
                  static_cast<double>(m_axisY * JOG_DISTANCE * JOG_INTERVAL),
                  static_cast<double>(m_axisZ * JOG_DISTANCE * JOG_INTERVAL),
                  static_cast<double>(feed));
    m_cnc->sendCommand(cmd);
}

void GamepadInput::processButtons() {
    if (!m_cnc) return;

    // Button mapping:
    // A = Cycle Start / Resume
    // B = Feed Hold / Pause
    // Back/Select = Soft Reset / Abort
    // Guide/Home = Home ($H)

    bool a = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A) != 0;
    bool b = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B) != 0;
    bool start = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START) != 0;
    bool back = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_BACK) != 0;
    bool home = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_GUIDE) != 0;

    // Rising edge detection (button pressed this frame, not last)
    if (a && !m_prevA) {
        m_cnc->cycleStart();
    }
    if (b && !m_prevB) {
        m_cnc->feedHold();
    }
    if (back && !m_prevBack) {
        m_cnc->softReset();
    }
    if (home && !m_prevHome && m_cnc->isConnected()) {
        m_cnc->sendCommand("$H");
    }

    m_prevA = a;
    m_prevB = b;
    m_prevStart = start;
    m_prevBack = back;
    m_prevHome = home;
}

} // namespace dw
