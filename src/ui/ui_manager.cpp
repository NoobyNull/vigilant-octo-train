#include "ui_manager.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>

#include "../core/utils/log.h"
#include "theme.h"

namespace dw {

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::initialize(SDL_Window* window, void* glContext) {
    if (m_initialized) {
        return true;
    }

    m_window = window;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Enable multi-viewport (optional, can cause issues on some systems)
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Font loading handled by Application::init() — this old UIManager is dead code

    // Apply default style
    initializeStyle();

    m_initialized = true;
    log::info("UI", "Initialized");
    return true;
}

void UIManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    m_window = nullptr;
    m_initialized = false;
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
}

void UIManager::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void UIManager::addMenu(const Menu& menu) {
    m_menus.push_back(menu);
}

void UIManager::clearMenus() {
    m_menus.clear();
}

void UIManager::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        for (const auto& menu : m_menus) {
            if (ImGui::BeginMenu(menu.label.c_str())) {
                for (const auto& item : menu.items) {
                    if (item.separator) {
                        ImGui::Separator();
                    } else if (item.checked != nullptr) {
                        if (ImGui::MenuItem(item.label.c_str(),
                                            item.shortcut.c_str(),
                                            item.checked,
                                            item.enabled)) {
                            if (item.callback) {
                                item.callback();
                            }
                        }
                    } else {
                        if (ImGui::MenuItem(
                                item.label.c_str(), item.shortcut.c_str(), false, item.enabled)) {
                            if (item.callback) {
                                item.callback();
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::setupDocking() {
    // Create fullscreen dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    windowFlags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void UIManager::initializeStyle() {
    applyDarkTheme();
}

void UIManager::applyDarkTheme() {
    Theme::applyDark();
}

void UIManager::applyLightTheme() {
    Theme::applyLight();
}

void UIManager::setFontScale(f32 scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;
}

bool UIManager::wantCaptureKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool UIManager::wantCaptureMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

} // namespace dw
