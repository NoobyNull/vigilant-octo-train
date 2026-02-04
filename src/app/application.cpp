// Digital Workshop - Application Implementation

#include "app/application.h"

#include "app/workspace.h"
#include "core/database/database.h"
#include "core/database/schema.h"
#include "core/library/library_manager.h"
#include "core/paths/app_paths.h"
#include "core/project/project.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/viewport_panel.h"
#include "ui/theme.h"
#include "version.h"

#include <cstdio>

#include <SDL.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

namespace dw {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

auto Application::init() -> bool {
    if (m_initialized) {
        return true;
    }

    // Ensure application directories exist
    paths::ensureDirectoriesExist();

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // OpenGL 3.3 Core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    auto windowFlags = static_cast<SDL_WindowFlags>(
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    m_window = SDL_CreateWindow(
        WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DEFAULT_WIDTH, DEFAULT_HEIGHT, windowFlags);

    if (m_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);
    if (m_glContext == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1); // VSync

    // Load OpenGL functions via GLAD
    int gladVersion = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (gladVersion == 0) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        return false;
    }

    std::printf("OpenGL %s\n", glGetString(GL_VERSION));

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Apply polished theme
    Theme::applyDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize core systems
    m_database = std::make_unique<Database>();
    if (!m_database->open(paths::getDatabasePath())) {
        std::fprintf(stderr, "Failed to open database\n");
        return false;
    }

    // Initialize database schema
    if (!Schema::initialize(*m_database)) {
        std::fprintf(stderr, "Failed to initialize database schema\n");
        return false;
    }

    m_libraryManager = std::make_unique<LibraryManager>(*m_database);
    m_projectManager = std::make_unique<ProjectManager>(*m_database);
    m_workspace = std::make_unique<Workspace>();

    // Initialize UI panels
    m_viewportPanel = std::make_unique<ViewportPanel>();
    m_libraryPanel = std::make_unique<LibraryPanel>(m_libraryManager.get());
    m_propertiesPanel = std::make_unique<PropertiesPanel>();
    m_projectPanel = std::make_unique<ProjectPanel>(m_projectManager.get());

    // Set up callbacks
    m_libraryPanel->setOnModelSelected([this](int64_t modelId) {
        onModelSelected(modelId);
    });

    m_projectPanel->setOnModelSelected([this](int64_t modelId) {
        onModelSelected(modelId);
    });

    m_initialized = true;
    std::printf("Digital Workshop %s initialized\n", VERSION);
    return true;
}

auto Application::run() -> int {
    if (!m_initialized) {
        std::fprintf(stderr, "Application not initialized\n");
        return 1;
    }

    m_running = true;

    while (m_running) {
        processEvents();
        update();
        render();
    }

    return 0;
}

void Application::quit() {
    m_running = false;
}

void Application::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            quit();
        }

        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window)) {
            quit();
        }
    }
}

void Application::update() {
    // Future: Update workspace, animations, etc.
}

void Application::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Create dockspace over entire window
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // Render menu bar
    renderMenuBar();

    // Render panels
    renderPanels();

    // Rendering
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

void Application::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                onNewProject();
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                onOpenProject();
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                onSaveProject();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Model", "Ctrl+I")) {
                onImportModel();
            }
            if (ImGui::MenuItem("Export Model", "Ctrl+E")) {
                onExportModel();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                quit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
            ImGui::MenuItem("Library", nullptr, &m_showLibrary);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Project", nullptr, &m_showProject);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // TODO: Show about dialog
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Application::renderPanels() {
    if (m_showViewport && m_viewportPanel) {
        m_viewportPanel->render();
    }

    if (m_showLibrary && m_libraryPanel) {
        m_libraryPanel->render();
    }

    if (m_showProperties && m_propertiesPanel) {
        m_propertiesPanel->render();
    }

    if (m_showProject && m_projectPanel) {
        m_projectPanel->render();
    }
}

void Application::onImportModel() {
    // TODO: Show file dialog and import
}

void Application::onExportModel() {
    // TODO: Show file dialog and export
}

void Application::onNewProject() {
    auto project = m_projectManager->create("New Project");
    m_projectManager->setCurrentProject(project);
    if (m_projectPanel) {
        m_projectPanel->refresh();
    }
}

void Application::onOpenProject() {
    // TODO: Show project open dialog
}

void Application::onSaveProject() {
    auto project = m_projectManager->currentProject();
    if (project) {
        m_projectManager->save(*project);
    }
}

void Application::onModelSelected(int64_t modelId) {
    // Load mesh from library and display in viewport
    if (m_libraryManager) {
        auto mesh = m_libraryManager->loadMesh(modelId);
        if (mesh) {
            m_workspace->setFocusedMesh(mesh);
            if (m_viewportPanel) {
                m_viewportPanel->setMesh(mesh);
            }
            if (m_propertiesPanel) {
                auto record = m_libraryManager->getModel(modelId);
                std::string name = record ? record->name : "";
                m_propertiesPanel->setMesh(mesh, name);
            }
        }
    }
}

void Application::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Destroy panels first
    m_viewportPanel.reset();
    m_libraryPanel.reset();
    m_propertiesPanel.reset();
    m_projectPanel.reset();

    // Destroy core systems
    m_workspace.reset();
    m_projectManager.reset();
    m_libraryManager.reset();
    m_database.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_glContext != nullptr) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
    m_initialized = false;
}

} // namespace dw
