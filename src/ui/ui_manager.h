#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../core/types.h"

struct SDL_Window;
union SDL_Event;

namespace dw {

// Menu item callback
using MenuCallback = std::function<void()>;

// Menu item definition
struct MenuItem {
    std::string label;
    std::string shortcut;
    MenuCallback callback;
    bool separator = false;
    bool enabled = true;
    bool* checked = nullptr; // For toggle items
};

// Menu definition
struct Menu {
    std::string label;
    std::vector<MenuItem> items;
};

// UI Manager - handles ImGui integration
class UIManager {
  public:
    UIManager() = default;
    ~UIManager();

    // Initialize/shutdown
    bool initialize(SDL_Window* window, void* glContext);
    void shutdown();

    // Frame handling
    void beginFrame();
    void endFrame();

    // Process SDL events
    void processEvent(const SDL_Event& event);

    // Main menu bar
    void addMenu(const Menu& menu);
    void clearMenus();
    void renderMenuBar();

    // Docking
    void setupDocking();
    bool isDockingEnabled() const { return m_dockingEnabled; }

    // Style
    void applyDarkTheme();
    void applyLightTheme();
    void setFontScale(f32 scale);

    // Utility
    bool wantCaptureKeyboard() const;
    bool wantCaptureMouse() const;

  private:
    void initializeStyle();

    SDL_Window* m_window = nullptr;
    std::vector<Menu> m_menus;
    bool m_dockingEnabled = true;
    bool m_initialized = false;
};

} // namespace dw
