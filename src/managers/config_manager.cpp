// Digital Workshop - Config Manager Implementation

#include "managers/config_manager.h"

#include <cstdlib>
#include <string>

#ifndef _WIN32
    #include <unistd.h>
#endif

#include "core/config/config.h"
#include "core/config/config_watcher.h"
#include "core/utils/log.h"
#include "managers/ui_manager.h"
#include "ui/panels/viewport_panel.h"
#include "ui/theme.h"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <shellapi.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

#include <SDL.h>

namespace dw {

ConfigManager::ConfigManager(EventBus* eventBus, UIManager* uiManager)
    : m_eventBus(eventBus), m_uiManager(uiManager) {}

ConfigManager::~ConfigManager() {
    shutdown();
}

void ConfigManager::init(SDL_Window* window) {
    m_window = window;

    // Snapshot restart-sensitive settings before applying, so applyConfig()
    // doesn't immediately detect a mismatch on first launch.
    m_lastAppliedUiScale = Config::instance().getUiScale();
    m_lastAppliedFloatingWindows = Config::instance().getEnableFloatingWindows();

    // Apply initial config (theme, render settings, log level)
    applyConfig();

    // Set up config file watcher for live reload
    m_configWatcher = std::make_unique<ConfigWatcher>();
    m_configWatcher->setOnChanged([this]() { onConfigFileChanged(); });
    m_configWatcher->watch(Config::instance().configFilePath());
}

void ConfigManager::poll(uint64_t ticksMs) {
    if (m_configWatcher) {
        m_configWatcher->poll(ticksMs);
    }
}

void ConfigManager::shutdown() {
    m_configWatcher.reset();
}

void ConfigManager::onConfigFileChanged() {
    auto& cfg = Config::instance();
    cfg.load();
    applyConfig();
}

void ConfigManager::applyConfig() {
    auto& cfg = Config::instance();

    // Theme (live)
    switch (cfg.getThemeIndex()) {
    case 1:
        Theme::applyLight();
        break;
    case 2:
        Theme::applyHighContrast();
        break;
    default:
        Theme::applyDark();
        break;
    }

    // Render settings (live) -- delegated to UIManager
    m_uiManager->applyRenderSettingsFromConfig();

    // Log level (live)
    log::setLevel(static_cast<log::Level>(cfg.getLogLevel()));

    // UI scale and floating windows require restart
    if (cfg.getUiScale() != m_lastAppliedUiScale ||
        cfg.getEnableFloatingWindows() != m_lastAppliedFloatingWindows) {
        m_uiManager->showRestartPopup() = true;
    }
}

void ConfigManager::spawnSettingsApp() {
#ifdef _WIN32
    wchar_t exePath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::wstring dir(exePath);
        auto slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            dir = dir.substr(0, slash);
        }
        std::wstring settingsPath = dir + L"\\dw_settings.exe";
        ShellExecuteW(nullptr, L"open", settingsPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
#elif defined(__linux__)
    // Resolve path relative to the running executable, launch via fork/exec
    // to avoid shell injection from directory names with special characters
    char exePath[1024] = {};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        exePath[len] = '\0';
        std::string dir(exePath);
        auto slash = dir.rfind('/');
        if (slash != std::string::npos) {
            dir = dir.substr(0, slash);
        }
        std::string settingsPath = dir + "/dw_settings";
        pid_t pid = fork();
        if (pid == 0) {
            execlp(settingsPath.c_str(), settingsPath.c_str(), nullptr);
            _exit(127);
        }
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execlp("dw_settings", "dw_settings", nullptr);
        _exit(127);
    }
#endif
}

void ConfigManager::relaunchApp() {
    // Save current config before relaunching
    Config::instance().save();

#ifdef _WIN32
    wchar_t selfPath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        ShellExecuteW(nullptr, L"open", selfPath, nullptr, nullptr, SW_SHOWNORMAL);
        if (m_quitCallback) {
            m_quitCallback();
        }
    }
#elif defined(__linux__)
    // Get path to self
    char selfPath[4096] = {};
    ssize_t len = readlink("/proc/self/exe", selfPath, sizeof(selfPath) - 1);
    if (len > 0) {
        selfPath[len] = '\0';
        // Spawn new instance and quit
        std::string cmd = std::string(selfPath) + " &";
        std::system(cmd.c_str());
        if (m_quitCallback) {
            m_quitCallback();
        }
    }
#endif
}

void ConfigManager::saveWorkspaceState() {
    auto& cfg = Config::instance();

    // Save window size and maximized state
    Uint32 flags = SDL_GetWindowFlags(m_window);
    bool maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
    cfg.setWindowMaximized(maximized);

    if (!maximized) {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(m_window, &w, &h);
        cfg.setWindowSize(w, h);
    }

    // Save panel visibility (delegated to UIManager)
    m_uiManager->saveVisibilityToConfig();

    // Save last selected model
    cfg.setLastSelectedModelId(m_uiManager->getSelectedModelId());

    cfg.save();
}

} // namespace dw
