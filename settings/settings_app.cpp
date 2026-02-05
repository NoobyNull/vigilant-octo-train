#include "settings_app.h"

#include "../src/core/paths/app_paths.h"
#include "../src/core/utils/log.h"
#include "../src/ui/theme.h"

#include <cstdio>
#include <cstdlib>

#include <SDL.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

namespace dw {

SettingsApp::SettingsApp() = default;

SettingsApp::~SettingsApp() {
    shutdown();
}

bool SettingsApp::init() {
    if (m_initialized) return true;

    paths::ensureDirectoriesExist();

    // Load current config
    Config::instance().load();

    // Cache values for editing
    auto& cfg = Config::instance();
    m_themeIndex = cfg.getThemeIndex();
    m_uiScale = cfg.getUiScale();
    m_showGrid = cfg.getShowGrid();
    m_showAxis = cfg.getShowAxis();
    m_logLevel = cfg.getLogLevel();
    m_lightDir = cfg.getRenderLightDir();
    m_lightColor = cfg.getRenderLightColor();
    m_ambient = cfg.getRenderAmbient();

    Color objColor = cfg.getRenderObjectColor();
    m_objectColor[0] = objColor.r;
    m_objectColor[1] = objColor.g;
    m_objectColor[2] = objColor.b;

    m_shininess = cfg.getRenderShininess();

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow("Digital Workshop - Settings",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!m_window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1);

    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) == 0) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    applyThemePreview();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_initialized = true;
    return true;
}

int SettingsApp::run() {
    if (!m_initialized) return 1;

    m_running = true;
    while (m_running) {
        processEvents();
        render();
    }

    // Auto-save on close if dirty
    if (m_dirty) {
        applySettings();
    }

    return 0;
}

void SettingsApp::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            m_running = false;
        }
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) {
            m_running = false;
        }
    }
}

void SettingsApp::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Full-window settings panel
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    if (ImGui::Begin("##Settings", nullptr, winFlags)) {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                renderGeneralTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance")) {
                renderAppearanceTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Rendering")) {
                renderRenderingTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Paths")) {
                renderPathsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About")) {
                renderAboutTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        // Bottom bar with Apply/Close
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float contentWidth = ImGui::GetContentRegionAvail().x;
        float buttonWidth = 100.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        ImGui::SetCursorPosX(contentWidth - buttonWidth * 2 - spacing);

        if (ImGui::Button("Apply", ImVec2(buttonWidth, 0))) {
            applySettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(buttonWidth, 0))) {
            m_running = false;
        }
    }
    ImGui::End();

    ImGui::Render();
    int displayW = 0;
    int displayH = 0;
    SDL_GL_GetDrawableSize(m_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(m_window);
}

void SettingsApp::renderGeneralTab() {
    ImGui::Spacing();

    ImGui::Text("Viewport");
    ImGui::Indent();
    if (ImGui::Checkbox("Show Grid", &m_showGrid)) m_dirty = true;
    if (ImGui::Checkbox("Show Axis", &m_showAxis)) m_dirty = true;
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Logging");
    ImGui::Indent();
    const char* logLevels[] = {"Debug", "Info", "Warning", "Error"};
    if (ImGui::Combo("Log Level", &m_logLevel, logLevels, 4)) m_dirty = true;
    ImGui::Unindent();
}

void SettingsApp::renderAppearanceTab() {
    ImGui::Spacing();

    ImGui::Text("Theme");
    ImGui::Indent();
    const char* themes[] = {"Dark", "Light", "High Contrast"};
    if (ImGui::Combo("##Theme", &m_themeIndex, themes, 3)) {
        m_dirty = true;
        applyThemePreview();
    }
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("UI Scale");
    ImGui::Indent();
    if (ImGui::SliderFloat("##Scale", &m_uiScale, 0.75f, 2.0f, "%.2f")) m_dirty = true;
    if (ImGui::Button("Reset to 100%%")) {
        m_uiScale = 1.0f;
        m_dirty = true;
    }
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::TextDisabled("Note: Theme changes are applied live.");
    ImGui::TextDisabled("UI Scale changes require a restart of the main application.");
}

void SettingsApp::renderRenderingTab() {
    ImGui::Spacing();

    ImGui::Text("Light Direction");
    ImGui::Indent();
    if (ImGui::SliderFloat("X##LightDir", &m_lightDir.x, -1.0f, 1.0f)) m_dirty = true;
    if (ImGui::SliderFloat("Y##LightDir", &m_lightDir.y, -1.0f, 1.0f)) m_dirty = true;
    if (ImGui::SliderFloat("Z##LightDir", &m_lightDir.z, -1.0f, 1.0f)) m_dirty = true;
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Light Color");
    ImGui::Indent();
    float lc[3] = {m_lightColor.x, m_lightColor.y, m_lightColor.z};
    if (ImGui::ColorEdit3("##LightColor", lc)) {
        m_lightColor = {lc[0], lc[1], lc[2]};
        m_dirty = true;
    }
    ImGui::Unindent();

    ImGui::Spacing();

    ImGui::Text("Ambient Color");
    ImGui::Indent();
    float ac[3] = {m_ambient.x, m_ambient.y, m_ambient.z};
    if (ImGui::ColorEdit3("##AmbientColor", ac)) {
        m_ambient = {ac[0], ac[1], ac[2]};
        m_dirty = true;
    }
    ImGui::Unindent();

    ImGui::Spacing();

    ImGui::Text("Object Color");
    ImGui::Indent();
    if (ImGui::ColorEdit3("##ObjectColor", m_objectColor)) m_dirty = true;

    // Color presets
    ImGui::Text("Presets:");
    ImGui::SameLine();
    struct Preset { const char* name; float r, g, b; };
    static constexpr Preset presets[] = {
        {"Steel",   0.6f, 0.6f, 0.65f},
        {"Copper",  0.72f, 0.45f, 0.20f},
        {"Gold",    0.83f, 0.69f, 0.22f},
        {"Blue",    0.4f, 0.6f, 0.8f},
        {"Red",     0.8f, 0.3f, 0.3f},
        {"Green",   0.3f, 0.7f, 0.4f},
        {"White",   0.9f, 0.9f, 0.9f},
        {"Black",   0.15f, 0.15f, 0.15f},
    };
    for (const auto& p : presets) {
        ImVec4 col(p.r, p.g, p.b, 1.0f);
        if (ImGui::ColorButton(p.name, col, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20))) {
            m_objectColor[0] = p.r;
            m_objectColor[1] = p.g;
            m_objectColor[2] = p.b;
            m_dirty = true;
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::Unindent();

    ImGui::Spacing();

    ImGui::Text("Shininess");
    ImGui::Indent();
    if (ImGui::SliderFloat("##Shininess", &m_shininess, 1.0f, 128.0f, "%.0f")) m_dirty = true;
    ImGui::Unindent();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Presets");
    ImGui::Indent();
    if (ImGui::Button("Default")) {
        m_lightDir = {-0.5f, -1.0f, -0.3f};
        m_lightColor = {1.0f, 1.0f, 1.0f};
        m_ambient = {0.2f, 0.2f, 0.2f};
        m_objectColor[0] = 0.4f; m_objectColor[1] = 0.6f; m_objectColor[2] = 0.8f;
        m_shininess = 32.0f;
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Warm Studio")) {
        m_lightDir = {-0.3f, -0.8f, -0.5f};
        m_lightColor = {1.0f, 0.95f, 0.85f};
        m_ambient = {0.25f, 0.22f, 0.20f};
        m_shininess = 48.0f;
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cool Workshop")) {
        m_lightDir = {-0.5f, -1.0f, 0.2f};
        m_lightColor = {0.85f, 0.90f, 1.0f};
        m_ambient = {0.18f, 0.20f, 0.25f};
        m_shininess = 24.0f;
        m_dirty = true;
    }
    ImGui::Unindent();
}

void SettingsApp::renderPathsTab() {
    ImGui::Spacing();

    ImGui::Text("Application Paths");
    ImGui::Spacing();

    ImGui::TextDisabled("Configuration:");
    ImGui::Text("  %s", paths::getConfigDir().string().c_str());

    ImGui::Spacing();

    ImGui::TextDisabled("Application Data:");
    ImGui::Text("  %s", paths::getDataDir().string().c_str());

    ImGui::Spacing();

    ImGui::TextDisabled("User Projects:");
    ImGui::Text("  %s", paths::getDefaultProjectsDir().string().c_str());

    ImGui::Spacing();

    ImGui::TextDisabled("Database:");
    ImGui::Text("  %s", paths::getDatabasePath().string().c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Open Config Folder")) {
        std::string dir = paths::getConfigDir().string();
        std::system(("xdg-open \"" + dir + "\"").c_str());
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Projects Folder")) {
        std::string dir = paths::getDefaultProjectsDir().string();
        std::system(("xdg-open \"" + dir + "\"").c_str());
    }
}

void SettingsApp::renderAboutTab() {
    ImGui::Spacing();

    ImGui::Text("Digital Workshop");
    ImGui::TextDisabled("Settings Application");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Digital Workshop is a 3D model management application "
        "for CNC and 3D printing workflows.");

    ImGui::Spacing();

    ImGui::Text("Libraries:");
    ImGui::BulletText("SDL2 - Window management");
    ImGui::BulletText("Dear ImGui - User interface");
    ImGui::BulletText("OpenGL 3.3 - 3D rendering");
    ImGui::BulletText("SQLite3 - Database");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Built with C++17");
}

void SettingsApp::applySettings() {
    auto& cfg = Config::instance();

    cfg.setThemeIndex(m_themeIndex);
    cfg.setUiScale(m_uiScale);
    cfg.setShowGrid(m_showGrid);
    cfg.setShowAxis(m_showAxis);
    cfg.setLogLevel(m_logLevel);

    cfg.setRenderLightDir(m_lightDir);
    cfg.setRenderLightColor(m_lightColor);
    cfg.setRenderAmbient(m_ambient);
    cfg.setRenderObjectColor(Color{m_objectColor[0], m_objectColor[1], m_objectColor[2], 1.0f});
    cfg.setRenderShininess(m_shininess);

    cfg.save();
    m_dirty = false;

    log::info("Settings", "Configuration saved");
}

void SettingsApp::applyThemePreview() {
    switch (m_themeIndex) {
        case 1:  Theme::applyLight(); break;
        case 2:  Theme::applyHighContrast(); break;
        default: Theme::applyDark(); break;
    }
}

void SettingsApp::shutdown() {
    if (!m_initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
    m_initialized = false;
}

}  // namespace dw
