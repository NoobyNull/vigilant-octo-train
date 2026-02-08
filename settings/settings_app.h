#pragma once

#include "../src/core/config/config.h"
#include "../src/core/config/input_binding.h"
#include "../src/core/types.h"
#include "../src/ui/widgets/binding_recorder.h"

struct SDL_Window;

namespace dw {

class SettingsApp {
  public:
    SettingsApp();
    ~SettingsApp();

    SettingsApp(const SettingsApp&) = delete;
    SettingsApp& operator=(const SettingsApp&) = delete;

    bool init();
    int run();

  private:
    void processEvents();
    void render();
    void shutdown();

    // Tabs
    void renderGeneralTab();
    void renderAppearanceTab();
    void renderRenderingTab();
    void renderPathsTab();
    void renderBindingsTab();
    void renderAboutTab();

    // Apply current settings to config and save
    void applySettings();

    // Apply theme preview in this window
    void applyThemePreview();

    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    bool m_running = false;
    bool m_initialized = false;
    bool m_dirty = false;

    // Cached settings (edited in UI)
    int m_themeIndex = 0;
    f32 m_uiScale = 1.0f;
    bool m_showGrid = true;
    bool m_showAxis = true;
    bool m_autoOrient = true;
    int m_navStyle = 0;
    int m_logLevel = 1;
    bool m_showStartPage = true;

    // Render settings
    Vec3 m_lightDir{-0.5f, -1.0f, -0.3f};
    Vec3 m_lightColor{1.0f, 1.0f, 1.0f};
    Vec3 m_ambient{0.2f, 0.2f, 0.2f};
    float m_objectColor[3]{0.4f, 0.6f, 0.8f};
    f32 m_shininess = 32.0f;

    // Input bindings
    std::array<InputBinding, static_cast<int>(BindAction::COUNT)> m_bindings;
    BindingRecorder m_bindingRecorder;

    static constexpr int WINDOW_WIDTH = 520;
    static constexpr int WINDOW_HEIGHT = 480;
};

} // namespace dw
