// Digital Workshop - Application Implementation

#include "app/application.h"

#include <cstdio>
#include <cstdlib>

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/config/config_watcher.h"
#include "core/database/database.h"
#include "core/database/schema.h"
#include "core/events/event_bus.h"
#include "core/export/model_exporter.h"
#include "core/import/import_queue.h"
#include "core/import/import_task.h"
#include "core/library/library_manager.h"
#include "core/loaders/loader_factory.h"
#include "core/paths/app_paths.h"
#include "core/project/project.h"
#include "core/utils/log.h"
#include "render/thumbnail_generator.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/dialogs/lighting_dialog.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/panels/cut_optimizer_panel.h"
#include "ui/panels/gcode_panel.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/start_page.h"
#include "ui/panels/viewport_panel.h"
#include "ui/theme.h"
#include "version.h"

#ifdef _WIN32
    #include <shellapi.h>
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

#include <SDL.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>

namespace dw {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::init() {
    if (m_initialized) {
        return true;
    }

    // Ensure application directories exist
    paths::ensureDirectoriesExist();

    // Load configuration
    Config::instance().load();

    // Apply log level from config
    log::setLevel(static_cast<log::Level>(Config::instance().getLogLevel()));

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

    // Create window — restore size from config
    auto& cfg = Config::instance();
    int startWidth = cfg.getWindowWidth();
    int startHeight = cfg.getWindowHeight();
    if (startWidth <= 0)
        startWidth = DEFAULT_WIDTH;
    if (startHeight <= 0)
        startHeight = DEFAULT_HEIGHT;

    auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                                    SDL_WINDOW_ALLOW_HIGHDPI);

    m_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                startWidth, startHeight, windowFlags);

    if (m_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Restore maximized state
    if (cfg.getWindowMaximized()) {
        SDL_MaximizeWindow(m_window);
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

    // Persist ImGui layout to config directory instead of CWD
    static std::string imguiIniPath = (paths::getConfigDir() / "imgui.ini").string();
    io.IniFilename = imguiIniPath.c_str();

    // Apply theme from config
    switch (Config::instance().getThemeIndex()) {
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

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize core systems
    // Initialize EventBus (foundation for decoupled communication)
    m_eventBus = std::make_unique<EventBus>();

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

    // Initialize thumbnail generator and connect to library manager
    m_thumbnailGenerator = std::make_unique<ThumbnailGenerator>();
    if (m_thumbnailGenerator->initialize()) {
        m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator.get());
    }

    // Initialize import queue
    m_importQueue = std::make_unique<ImportQueue>(*m_database);

    // Initialize UI panels
    m_viewportPanel = std::make_unique<ViewportPanel>();
    m_libraryPanel = std::make_unique<LibraryPanel>(m_libraryManager.get());
    m_propertiesPanel = std::make_unique<PropertiesPanel>();
    m_projectPanel = std::make_unique<ProjectPanel>(m_projectManager.get());
    m_gcodePanel = std::make_unique<GCodePanel>();
    m_cutOptimizerPanel = std::make_unique<CutOptimizerPanel>();

    // Initialize start page
    m_startPage = std::make_unique<StartPage>();
    m_startPage->setOnNewProject([this]() {
        onNewProject();
        m_showStartPage = false;
    });
    m_startPage->setOnOpenProject([this]() { onOpenProject(); });
    m_startPage->setOnImportModel([this]() {
        onImportModel();
        m_showStartPage = false;
    });
    m_startPage->setOnOpenRecentProject([this](const Path& path) { onOpenRecentProject(path); });

    // Apply config render settings to viewport
    {
        auto& rs = m_viewportPanel->renderSettings();
        rs.lightDir = cfg.getRenderLightDir();
        rs.lightColor = cfg.getRenderLightColor();
        rs.ambient = cfg.getRenderAmbient();
        rs.objectColor = cfg.getRenderObjectColor();
        rs.shininess = cfg.getRenderShininess();
        rs.showGrid = cfg.getShowGrid();
        rs.showAxis = cfg.getShowAxis();
    }

    // Set up callbacks
    // Single-click: show metadata in properties panel
    m_libraryPanel->setOnModelSelected([this](int64_t modelId) {
        if (!m_libraryManager)
            return;
        auto record = m_libraryManager->getModel(modelId);
        if (record && m_propertiesPanel) {
            m_propertiesPanel->setModelRecord(*record);
        }
    });

    // Double-click: load mesh into viewport
    m_libraryPanel->setOnModelOpened([this](int64_t modelId) { onModelSelected(modelId); });

    m_projectPanel->setOnModelSelected([this](int64_t modelId) { onModelSelected(modelId); });

    m_propertiesPanel->setOnMeshModified([this]() {
        // Re-upload mesh to GPU after transform operations
        auto mesh = m_workspace->getFocusedMesh();
        if (mesh && m_viewportPanel) {
            m_viewportPanel->setMesh(mesh);
        }
    });

    m_propertiesPanel->setOnColorChanged([this](const Color& color) {
        // Update renderer object color
        if (m_viewportPanel) {
            m_viewportPanel->renderSettings().objectColor = color;
        }
    });

    m_projectPanel->setOpenProjectCallback([this]() { onOpenProject(); });

    m_projectPanel->setSaveProjectCallback([this]() { onSaveProject(); });

    // Initialize dialogs
    m_fileDialog = std::make_unique<FileDialog>();
    m_lightingDialog = std::make_unique<LightingDialog>();
    if (m_viewportPanel) {
        m_lightingDialog->setSettings(&m_viewportPanel->renderSettings());
    }

    // Connect file dialog to G-code panel
    if (m_gcodePanel) {
        m_gcodePanel->setFileDialog(m_fileDialog.get());
    }

    // Set up config file watcher for live reload
    m_configWatcher = std::make_unique<ConfigWatcher>();
    m_configWatcher->setOnChanged([this]() { onConfigFileChanged(); });
    m_configWatcher->watch(Config::instance().configFilePath());
    m_lastAppliedUiScale = cfg.getUiScale();

    // Restore workspace state from config
    m_showViewport = cfg.getShowViewport();
    m_showLibrary = cfg.getShowLibrary();
    m_showProperties = cfg.getShowProperties();
    m_showProject = cfg.getShowProject();
    m_showGCode = cfg.getShowGCode();
    m_showCutOptimizer = cfg.getShowCutOptimizer();
    m_showStartPage = cfg.getShowStartPage();

    // Restore last selected model
    {
        i64 lastModelId = cfg.getLastSelectedModelId();
        if (lastModelId > 0 && m_libraryManager) {
            auto record = m_libraryManager->getModel(lastModelId);
            if (record) {
                onModelSelected(lastModelId);
                if (m_libraryPanel) {
                    m_libraryPanel->setSelectedModelId(lastModelId);
                }
            }
        }
    }

    m_initialized = true;
    std::printf("Digital Workshop %s initialized\n", VERSION);
    return true;
}

int Application::run() {
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

auto Application::eventBus() -> EventBus& {
    return *m_eventBus;
}

void Application::processEvents() {
    std::vector<std::string> droppedFiles;

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            quit();
        }

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window)) {
            quit();
        }

        if (event.type == SDL_DROPFILE && event.drop.file != nullptr) {
            droppedFiles.emplace_back(event.drop.file);
            SDL_free(event.drop.file);
        }
    }

    if (!droppedFiles.empty()) {
        onFilesDropped(droppedFiles);
    }
}

void Application::update() {
    processCompletedImports();

    // Poll config watcher for live settings reload
    if (m_configWatcher) {
        m_configWatcher->poll(SDL_GetTicks64());
    }
}

void Application::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Create dockspace over entire window
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // On first frame, set up default dock layout if no imgui.ini was loaded
    if (m_firstFrame) {
        m_firstFrame = false;
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr ||
            ImGui::DockBuilderGetNode(dockspaceId)->IsLeafNode()) {
            setupDefaultDockLayout(dockspaceId);
        }
    }

    // Handle keyboard shortcuts
    handleKeyboardShortcuts();

    // Render menu bar
    renderMenuBar();

    // Render panels
    renderPanels();

    // Import progress overlay
    renderImportProgress();

    // Restart required popup
    renderRestartPopup();

    // About popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About Digital Workshop", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Digital Workshop");
        ImGui::Text("Version %s", VERSION);
        ImGui::Separator();
        ImGui::TextWrapped("A 3D model management application for CNC and 3D printing workflows.");
        ImGui::Spacing();
        ImGui::Text("Libraries:");
        ImGui::BulletText("SDL2 - Window management");
        ImGui::BulletText("Dear ImGui - User interface");
        ImGui::BulletText("OpenGL 3.3 - 3D rendering");
        ImGui::BulletText("SQLite3 - Database");
        ImGui::Separator();
        ImGui::TextDisabled("Built with C++17");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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
            ImGui::MenuItem("Start Page", nullptr, &m_showStartPage);
            ImGui::Separator();
            ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
            ImGui::MenuItem("Library", nullptr, &m_showLibrary);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Project", nullptr, &m_showProject);
            ImGui::Separator();
            ImGui::MenuItem("G-code Viewer", nullptr, &m_showGCode);
            ImGui::MenuItem("Cut Optimizer", nullptr, &m_showCutOptimizer);
            ImGui::Separator();
            if (ImGui::MenuItem("Lighting Settings", "Ctrl+L")) {
                if (m_lightingDialog) {
                    m_lightingDialog->open();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Settings", "Ctrl+,")) {
                spawnSettingsApp();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Digital Workshop")) {
                showAboutDialog();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Application::renderPanels() {
    // Start page
    if (m_showStartPage && m_startPage) {
        m_startPage->render();
    }

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

    if (m_showGCode && m_gcodePanel) {
        m_gcodePanel->render();
    }

    if (m_showCutOptimizer && m_cutOptimizerPanel) {
        m_cutOptimizerPanel->render();
    }

    // Render dialogs
    if (m_fileDialog) {
        m_fileDialog->render();
    }

    if (m_lightingDialog) {
        m_lightingDialog->render();
    }
}

void Application::onImportModel() {
    if (m_fileDialog) {
        m_fileDialog->showOpenMulti("Import Models", FileDialog::modelFilters(),
                                    [this](const std::vector<std::string>& paths) {
                                        if (paths.empty())
                                            return;

                                        std::vector<Path> importPaths;
                                        importPaths.reserve(paths.size());
                                        for (const auto& p : paths) {
                                            importPaths.emplace_back(p);
                                        }

                                        if (m_importQueue) {
                                            m_importQueue->enqueue(importPaths);
                                        }
                                    });
    }
}

void Application::onExportModel() {
    auto mesh = m_workspace->getFocusedMesh();
    if (!mesh) {
        MessageDialog::warning("No Model", "No model selected to export.");
        return;
    }

    if (m_fileDialog) {
        m_fileDialog->showSave("Export Model", FileDialog::modelFilters(), "model.stl",
                               [this, mesh](const std::string& path) {
                                   if (path.empty())
                                       return;

                                   ModelExporter exporter;
                                   auto result = exporter.exportMesh(*mesh, path);

                                   if (result.success) {
                                       MessageDialog::info("Export Complete",
                                                           "Model exported to:\n" + path);
                                   } else {
                                       MessageDialog::error("Export Failed", result.error);
                                   }
                               });
    }
}

void Application::onNewProject() {
    auto project = m_projectManager->create("New Project");
    m_projectManager->setCurrentProject(project);
    m_showStartPage = false;
}

void Application::onOpenProject() {
    if (!m_fileDialog)
        return;

    m_fileDialog->showOpen("Open Project", FileDialog::projectFilters(),
                           [this](const std::string& path) {
                               if (path.empty())
                                   return;

                               // Search existing projects for one matching this file path
                               auto projects = m_projectManager->listProjects();
                               for (const auto& record : projects) {
                                   if (record.filePath == Path(path)) {
                                       auto project = m_projectManager->open(record.id);
                                       if (project) {
                                           m_projectManager->setCurrentProject(project);
                                           Config::instance().addRecentProject(Path(path));
                                           Config::instance().save();
                                           m_showStartPage = false;
                                           return;
                                       }
                                   }
                               }

                               // No existing project found at that path - create a new one
                               // and associate the file path
                               Path filePath(path);
                               std::string name = filePath.stem().string();
                               auto project = m_projectManager->create(name);
                               if (project) {
                                   project->setFilePath(filePath);
                                   m_projectManager->setCurrentProject(project);
                                   Config::instance().addRecentProject(filePath);
                                   Config::instance().save();
                                   m_showStartPage = false;
                               }
                           });
}

void Application::onSaveProject() {
    auto project = m_projectManager->currentProject();
    if (!project) {
        MessageDialog::warning("No Project", "No project is currently open.");
        return;
    }

    // If project has no file path, show save dialog to pick one
    if (project->filePath().empty()) {
        if (m_fileDialog) {
            std::string defaultName = project->name() + ".dwp";
            m_fileDialog->showSave("Save Project", FileDialog::projectFilters(), defaultName,
                                   [this, project](const std::string& path) {
                                       if (path.empty())
                                           return;
                                       project->setFilePath(Path(path));
                                       m_projectManager->save(*project);
                                       Config::instance().addRecentProject(Path(path));
                                       Config::instance().save();
                                   });
        }
    } else {
        m_projectManager->save(*project);
        if (!project->filePath().empty()) {
            Config::instance().addRecentProject(project->filePath());
            Config::instance().save();
        }
    }
}

void Application::onModelSelected(int64_t modelId) {
    if (!m_libraryManager)
        return;

    auto mesh = m_libraryManager->loadMesh(modelId);
    if (!mesh)
        return;

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

void Application::onFilesDropped(const std::vector<std::string>& paths) {
    if (!m_importQueue)
        return;

    std::vector<Path> importPaths;
    for (const auto& p : paths) {
        Path path{p};
        auto ext = path.extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);

        if (LoaderFactory::isSupported(ext)) {
            importPaths.push_back(path);
        }
    }

    if (!importPaths.empty()) {
        m_importQueue->enqueue(importPaths);
    }
}

void Application::processCompletedImports() {
    if (!m_importQueue)
        return;

    auto completed = m_importQueue->pollCompleted();
    if (!completed.empty()) {
        m_showStartPage = false;
    }
    for (auto& task : completed) {
        // Generate thumbnail on main thread (needs GL context)
        // Use LibraryManager which writes the file AND updates the DB
        if (m_thumbnailGenerator && task.mesh && m_libraryManager) {
            m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator.get());
            m_libraryManager->generateThumbnail(task.modelId, *task.mesh);
        }

        // Select the last imported model and refresh library
        if (m_libraryPanel) {
            m_libraryPanel->refresh();
        }

        // Focus the newly imported mesh directly (no re-read from disk)
        if (task.mesh) {
            m_workspace->setFocusedMesh(task.mesh);
            if (m_viewportPanel) {
                m_viewportPanel->setMesh(task.mesh);
            }
            if (m_propertiesPanel) {
                m_propertiesPanel->setMesh(task.mesh, task.record.name);
            }
        }
    }
}

void Application::onOpenRecentProject(const Path& path) {
    if (!m_projectManager)
        return;

    auto projects = m_projectManager->listProjects();
    for (const auto& record : projects) {
        if (record.filePath == path) {
            auto project = m_projectManager->open(record.id);
            if (project) {
                m_projectManager->setCurrentProject(project);
                Config::instance().addRecentProject(path);
                Config::instance().save();
                m_showStartPage = false;
                return;
            }
        }
    }

    // Project not found in DB — create new from path
    std::string name = path.stem().string();
    auto project = m_projectManager->create(name);
    if (project) {
        project->setFilePath(path);
        m_projectManager->setCurrentProject(project);
        Config::instance().addRecentProject(path);
        Config::instance().save();
        m_showStartPage = false;
    }
}

void Application::showAboutDialog() {
    ImGui::OpenPopup("About Digital Workshop");
}

void Application::handleKeyboardShortcuts() {
    auto& io = ImGui::GetIO();

    // Only handle shortcuts when not typing in a text field
    if (io.WantTextInput)
        return;

    bool ctrl = io.KeyCtrl;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
        onNewProject();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        onOpenProject();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        onSaveProject();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_I)) {
        onImportModel();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_E)) {
        onExportModel();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Comma)) {
        spawnSettingsApp();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_L)) {
        if (m_lightingDialog) {
            m_lightingDialog->open();
        }
    }
}

void Application::renderImportProgress() {
    if (!m_importQueue || !m_importQueue->isActive())
        return;

    const auto& prog = m_importQueue->progress();

    // Overlay in bottom-right corner
    const float padding = 16.0f;
    auto* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos(viewport->WorkPos.x + viewport->WorkSize.x - padding,
                     viewport->WorkPos.y + viewport->WorkSize.y - padding);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Importing...", nullptr, flags)) {
        int completed = prog.completedFiles.load();
        int total = prog.totalFiles.load();
        int failed = prog.failedFiles.load();

        // Current file and stage
        ImGui::Text("%s", prog.getCurrentFileName().c_str());
        ImGui::TextDisabled("%s", importStageName(prog.currentStage.load()));

        // Overall progress bar
        float fraction =
            total > 0 ? static_cast<float>(completed) / static_cast<float>(total) : 0.0f;
        char overlay[64];
        std::snprintf(overlay, sizeof(overlay), "%d / %d", completed, total);
        ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);

        if (failed > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%d failed", failed);
        }

        if (ImGui::Button("Cancel")) {
            m_importQueue->cancel();
        }
    }
    ImGui::End();
}

void Application::onConfigFileChanged() {
    auto& cfg = Config::instance();
    cfg.load();
    applyConfig();
}

void Application::applyConfig() {
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

    // Render settings (live)
    if (m_viewportPanel) {
        auto& rs = m_viewportPanel->renderSettings();
        rs.lightDir = cfg.getRenderLightDir();
        rs.lightColor = cfg.getRenderLightColor();
        rs.ambient = cfg.getRenderAmbient();
        rs.objectColor = cfg.getRenderObjectColor();
        rs.shininess = cfg.getRenderShininess();
        rs.showGrid = cfg.getShowGrid();
        rs.showAxis = cfg.getShowAxis();
    }

    // Log level (live)
    log::setLevel(static_cast<log::Level>(cfg.getLogLevel()));

    // UI scale requires restart
    if (cfg.getUiScale() != m_lastAppliedUiScale) {
        m_showRestartPopup = true;
    }
}

void Application::spawnSettingsApp() {
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
    // Resolve path relative to the running executable
    char exePath[1024] = {};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        exePath[len] = '\0';
        std::string dir(exePath);
        auto slash = dir.rfind('/');
        if (slash != std::string::npos) {
            dir = dir.substr(0, slash);
        }
        std::string cmd = dir + "/dw_settings &";
        std::system(cmd.c_str());
        return;
    }
    std::system("dw_settings &");
#endif
}

void Application::relaunchApp() {
    // Save current config before relaunching
    Config::instance().save();

#ifdef _WIN32
    wchar_t selfPath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        ShellExecuteW(nullptr, L"open", selfPath, nullptr, nullptr, SW_SHOWNORMAL);
        quit();
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
        quit();
    }
#endif
}

void Application::renderRestartPopup() {
    if (!m_showRestartPopup)
        return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Restart Required", &m_showRestartPopup,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("UI Scale has been changed.");
        ImGui::Text("A restart is required to apply this setting.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Relaunch Now", ImVec2(140, 0))) {
            m_showRestartPopup = false;
            relaunchApp();
        }
        ImGui::SameLine();
        if (ImGui::Button("Later", ImVec2(140, 0))) {
            m_showRestartPopup = false;
        }
    }
    ImGui::End();
}

void Application::setupDefaultDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    // Split: left sidebar (20%) | center+right
    ImGuiID dockLeft = 0;
    ImGuiID dockCenterRight = 0;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenterRight);

    // Split left sidebar: library (top 60%) | project (bottom 40%)
    ImGuiID dockLeftTop = 0;
    ImGuiID dockLeftBottom = 0;
    ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.40f, &dockLeftBottom, &dockLeftTop);

    // Split center+right: center | right sidebar (22%) for properties
    ImGuiID dockCenter = 0;
    ImGuiID dockRight = 0;
    ImGui::DockBuilderSplitNode(dockCenterRight, ImGuiDir_Right, 0.22f, &dockRight, &dockCenter);

    // Dock windows into their positions
    ImGui::DockBuilderDockWindow("Library", dockLeftTop);
    ImGui::DockBuilderDockWindow("Project", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    ImGui::DockBuilderDockWindow("Properties", dockRight);

    ImGui::DockBuilderFinish(dockspaceId);
}

void Application::saveWorkspaceState() {
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

    // Save panel visibility
    cfg.setShowViewport(m_showViewport);
    cfg.setShowLibrary(m_showLibrary);
    cfg.setShowProperties(m_showProperties);
    cfg.setShowProject(m_showProject);
    cfg.setShowGCode(m_showGCode);
    cfg.setShowCutOptimizer(m_showCutOptimizer);
    cfg.setShowStartPage(m_showStartPage);

    // Save last selected model
    if (m_libraryPanel) {
        cfg.setLastSelectedModelId(m_libraryPanel->selectedModelId());
    }

    cfg.save();
}

void Application::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Save workspace state before destroying anything
    saveWorkspaceState();

    // Destroy dialogs and watcher
    m_fileDialog.reset();
    m_lightingDialog.reset();
    m_configWatcher.reset();

    // Destroy panels
    m_viewportPanel.reset();
    m_libraryPanel.reset();
    m_propertiesPanel.reset();
    m_projectPanel.reset();
    m_gcodePanel.reset();
    m_cutOptimizerPanel.reset();
    m_startPage.reset();

    // Destroy core systems
    m_importQueue.reset(); // Joins worker thread
    m_workspace.reset();
    m_thumbnailGenerator.reset();
    m_projectManager.reset();
    m_libraryManager.reset();
    m_database.reset();
    m_eventBus.reset(); // Destroy after all subscribers

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
