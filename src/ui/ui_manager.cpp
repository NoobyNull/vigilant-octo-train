#include "ui_manager.h"

#include "../core/utils/log.h"

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

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
                        if (ImGui::MenuItem(item.label.c_str(), item.shortcut.c_str(),
                                            item.checked, item.enabled)) {
                            if (item.callback) {
                                item.callback();
                            }
                        }
                    } else {
                        if (ImGui::MenuItem(item.label.c_str(), item.shortcut.c_str(),
                                            false, item.enabled)) {
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
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void UIManager::initializeStyle() {
    applyDarkTheme();
}

void UIManager::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Refined dark theme with subtle colors
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.41f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.51f, 0.52f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.36f, 0.37f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.36f, 0.37f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.60f, 0.85f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.60f, 0.85f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.50f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.40f, 0.60f, 0.85f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    // Style adjustments
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;
}

void UIManager::applyLightTheme() {
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
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

bool UIManager::openFileDialog(const std::string& title, const std::string& filters,
                                std::string& outPath) {
    // Simple file dialog using tinyfiledialogs or native SDL dialog
    // For now, return false - will be implemented in integration phase
    return false;
}

bool UIManager::saveFileDialog(const std::string& title, const std::string& filters,
                                std::string& outPath) {
    return false;
}

}  // namespace dw
