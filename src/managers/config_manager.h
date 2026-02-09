#pragma once

// Digital Workshop - Config Manager
// Handles config file watching, config applying (theme, render settings,
// log level), UI scale restart detection, workspace state saving,
// settings app spawning, and app relaunch. Extracted from Application
// (god class decomposition Wave 3).

#include <functional>
#include <memory>

#include "../core/types.h"

struct SDL_Window;

namespace dw {

class EventBus;
class UIManager;
class ConfigWatcher;

class ConfigManager {
  public:
    ConfigManager(EventBus* eventBus, UIManager* uiManager);
    ~ConfigManager();

    // Disable copy/move
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    // Initialize config watching
    void init(SDL_Window* window);

    // Call each frame to poll config watcher
    void poll(uint64_t ticksMs);

    // Shutdown (stops watcher)
    void shutdown();

    // Save workspace state (window size, panel visibility, last model)
    void saveWorkspaceState();

    // Spawn external settings app
    void spawnSettingsApp();

    // Relaunch the application (save config, spawn new instance, quit)
    void relaunchApp();

    // Set quit callback (for relaunch to quit current instance)
    void setQuitCallback(std::function<void()> cb) { m_quitCallback = std::move(cb); }

  private:
    void onConfigFileChanged();
    void applyConfig();

    EventBus* m_eventBus;
    UIManager* m_uiManager;
    SDL_Window* m_window = nullptr;

    std::unique_ptr<ConfigWatcher> m_configWatcher;
    float m_lastAppliedUiScale = 1.0f;
    std::function<void()> m_quitCallback;
};

} // namespace dw
