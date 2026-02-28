// Digital Workshop - Application Implementation
// Thin coordinator: SDL/GL lifecycle, manager creation, event loop, shutdown.
// Panel/callback wiring lives in application_wiring.cpp.

#include "app/application.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>
#include <SDL.h>

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/database/connection_pool.h"
#include "core/database/cost_repository.h"
#include "core/database/cut_plan_repository.h"
#include "core/optimizer/cut_list_file.h"
#include "core/database/database.h"
#include "core/database/gcode_repository.h"
#include "core/database/job_repository.h"
#include "core/database/model_repository.h"
#include "core/database/tool_database.h"
#include "core/database/schema.h"
#include "core/export/project_export_manager.h"
#include "core/cnc/cnc_controller.h"
#include "core/cnc/serial_port.h"
#include "core/cnc/gamepad_input.h"
#include "core/cnc/macro_manager.h"
#include "core/graph/graph_manager.h"
#include "core/import/background_tagger.h"
#include "core/import/import_log.h"
#include "core/import/import_queue.h"
#include "core/library/library_manager.h"
#include "core/materials/gemini_descriptor_service.h"
#include "core/materials/gemini_material_service.h"
#include "core/materials/material_manager.h"
#include "core/paths/app_paths.h"
#include "core/project/project.h"
#include "core/storage/storage_manager.h"
#include "core/threading/main_thread_queue.h"
#include "core/threading/thread_pool.h"
#include "core/utils/log.h"
#include "core/utils/thread_utils.h"
#include "managers/config_manager.h"
#include "managers/file_io_manager.h"
#include "managers/ui_manager.h"
#include "render/thumbnail_generator.h"
#include "ui/panels/gcode_panel.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/viewport_panel.h"
#include "ui/theme.h"
#include "ui/widgets/toast.h"
#include "version.h"

namespace dw {

// Explicit destructors needed for unique_ptr of incomplete types
Application::Application() {}
Application::~Application() {
    shutdown();
}

bool Application::init() {
    if (m_initialized)
        return true;

    paths::ensureDirectoriesExist();
    Config::instance().load();
    log::setLevel(static_cast<log::Level>(Config::instance().getLogLevel()));

    // Multi-viewport requires X11 — Wayland SDL2 backend lacks platform viewport support
    if (Config::instance().getEnableFloatingWindows()) {
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
    }

    // Request per-monitor DPI awareness on Windows
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
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

    // Create window (restore size from config)
    auto& cfg = Config::instance();
    int startWidth = cfg.getWindowWidth();
    int startHeight = cfg.getWindowHeight();
    if (startWidth <= 0)
        startWidth = DEFAULT_WIDTH;
    if (startHeight <= 0)
        startHeight = DEFAULT_HEIGHT;

    auto windowFlags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                                    SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow(WINDOW_TITLE,
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                startWidth,
                                startHeight,
                                windowFlags);
    if (m_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    if (cfg.getWindowMaximized())
        SDL_MaximizeWindow(m_window);

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);
    if (m_glContext == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1);

    int gladVersion = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (gladVersion == 0) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        return false;
    }
    std::printf("OpenGL %s\n", glGetString(GL_VERSION));

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (Config::instance().getEnableFloatingWindows()) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    static std::string imguiIniPath = (paths::getConfigDir() / "imgui.ini").string();
    io.IniFilename = imguiIniPath.c_str();

    // Detect DPI scale and combine with user's UI scale setting
    m_dpiScale = detectDpiScale();
    m_uiScale = m_dpiScale * cfg.getUiScale();
    m_displayIndex = SDL_GetWindowDisplayIndex(m_window);

    // Load fonts at scaled size
    rebuildFontAtlas(m_uiScale);

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    if (Config::instance().getEnableFloatingWindows()) {
        bool platformOk = (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) != 0;
        bool rendererOk = (io.BackendFlags & ImGuiBackendFlags_RendererHasViewports) != 0;
        if (!platformOk || !rendererOk) {
            log::errorf("Application",
                        "Floating windows: platform=%s renderer=%s — viewports disabled",
                        platformOk ? "ok" : "NO",
                        rendererOk ? "ok" : "NO");
            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }
    }

    // Initialize core systems
    dw::threading::initMainThread();
    m_mainThreadQueue = std::make_unique<MainThreadQueue>();

    m_database = std::make_unique<Database>();
    if (!m_database->open(paths::getDatabasePath())) {
        std::fprintf(stderr, "Failed to open database\n");
        return false;
    }
    if (!Schema::initialize(*m_database)) {
        std::fprintf(stderr, "Failed to initialize database schema\n");
        return false;
    }

    // Size ConnectionPool for parallel workers + main thread
    // Calculate max thread count from config, add 2 for main thread + overhead
    auto tier = Config::instance().getParallelismTier();
    size_t maxWorkers = calculateThreadCount(tier);
    size_t poolSize = std::max(static_cast<size_t>(4), maxWorkers + 2);
    m_connectionPool = std::make_unique<ConnectionPool>(paths::getDatabasePath(), poolSize);

    // Initialize GraphQLite extension for Cypher graph queries (ORG-03/04/05)
    m_graphManager = std::make_unique<GraphManager>(*m_database);
    {
        // Resolve extension directory relative to the running executable
        std::error_code ec;
        Path exeDir;
#ifdef __linux__
        Path exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec)
            exeDir = exePath.parent_path();
#endif
        if (exeDir.empty())
            exeDir = std::filesystem::current_path();

        if (!m_graphManager->initialize(exeDir)) {
            log::warning("Application",
                         "GraphQLite extension not available -- graph queries disabled");
        }
    }

    m_libraryManager = std::make_unique<LibraryManager>(*m_database);
    m_libraryManager->setGraphManager(m_graphManager.get());
    m_projectManager = std::make_unique<ProjectManager>(*m_database);
    m_materialManager = std::make_unique<MaterialManager>(*m_database);
    m_materialManager->seedDefaults();
    m_modelRepo = std::make_unique<ModelRepository>(*m_database);
    m_gcodeRepo = std::make_unique<GCodeRepository>(*m_database);
    m_jobRepo = std::make_unique<JobRepository>(*m_database);
    m_cutPlanRepo = std::make_unique<CutPlanRepository>(*m_database);
    // Mark any 'running' jobs as interrupted (app crashed during previous session)
    {
        auto running = m_jobRepo->findByStatus("running");
        for (auto& job : running) {
            m_jobRepo->finishJob(
                job.id, "interrupted", job.lastAckedLine, job.elapsedSeconds, job.errorCount, job.modalState);
        }
        if (!running.empty())
            log::infof("App", "Marked %zu interrupted job(s) from prior session", running.size());
    }

    m_cutListFile = std::make_unique<CutListFile>();
    m_cutListFile->setDirectory(paths::getDataDir() / "cutlists");
    m_costRepo = std::make_unique<CostRepository>(*m_database);
    m_geminiService = std::make_unique<GeminiMaterialService>();
    m_descriptorService = std::make_unique<GeminiDescriptorService>();
    m_workspace = std::make_unique<Workspace>();

    m_thumbnailGenerator = std::make_unique<ThumbnailGenerator>();
    if (m_thumbnailGenerator->initialize()) {
        m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator.get());
    }

    // Content-addressable blob store (STOR-01/02/03)
    m_storageManager = std::make_unique<StorageManager>(
        Config::instance().getSupportDir() / "blobs");

    // Clean up orphaned temp files from prior crashes (STOR-03)
    int orphansCleaned = m_storageManager->cleanupOrphanedTempFiles();
    if (orphansCleaned > 0) {
        log::infof("App", "Cleaned up %d orphaned temp file(s) from prior session", orphansCleaned);
    }

    // Project export/import manager (.dwproj archives) (EXPORT-01/02)
    m_projectExportManager = std::make_unique<ProjectExportManager>(*m_database);

    // CNC tool database (Vectric .vtdb format)
    m_toolDatabase = std::make_unique<ToolDatabase>();
    if (!m_toolDatabase->open(paths::getToolDatabasePath())) {
        log::error("Application", "Failed to open tool database");
        return false;
    }

    // CNC controller (multi-firmware support: GRBL, grblHAL, FluidNC, Smoothieware)
    m_cncController = std::make_unique<CncController>(m_mainThreadQueue.get());

    // CNC macro manager (SQLite-backed macro storage)
    m_macroManager = std::make_unique<MacroManager>(paths::getMacroDatabasePath().string());
    m_macroManager->ensureBuiltIns();

    // CNC gamepad input (SDL_GameController for jog/actions)
    m_gamepadInput = std::make_unique<GamepadInput>();
    m_gamepadInput->setCncController(m_cncController.get());

    m_importQueue = std::make_unique<ImportQueue>(*m_connectionPool,
                                                  m_libraryManager.get(),
                                                  m_storageManager.get());

    m_importLog = std::make_unique<ImportLog>(Config::instance().getSupportDir() / ".import-log");
    m_importQueue->setImportLog(m_importLog.get());

    m_backgroundTagger = std::make_unique<BackgroundTagger>(
        *m_connectionPool, m_libraryManager.get(), m_descriptorService.get());

    // Initialize managers
    m_uiManager = std::make_unique<UIManager>();
    m_uiManager->init(
        m_libraryManager.get(), m_projectManager.get(), m_materialManager.get(),
        m_costRepo.get(), m_modelRepo.get(), m_gcodeRepo.get(), m_cutPlanRepo.get());

    m_fileIOManager = std::make_unique<FileIOManager>(m_database.get(),
                                                      m_libraryManager.get(),
                                                      m_projectManager.get(),
                                                      m_importQueue.get(),
                                                      m_workspace.get(),
                                                      m_uiManager->fileDialog(),
                                                      m_thumbnailGenerator.get(),
                                                      m_projectExportManager.get());
    m_fileIOManager->setProgressDialog(m_uiManager->progressDialog());
    m_fileIOManager->setMainThreadQueue(m_mainThreadQueue.get());
    m_fileIOManager->setDescriptorService(m_descriptorService.get());

    m_fileIOManager->setThumbnailCallback(
        [this](int64_t modelId, Mesh& mesh) { return generateMaterialThumbnail(modelId, mesh); });

    m_fileIOManager->setGCodeCallback([this](const std::string& path) {
        if (auto* gcp = m_uiManager->gcodePanel()) {
            gcp->setOpen(true);
            gcp->loadFile(path);
        }
    });

    m_configManager = std::make_unique<ConfigManager>(m_uiManager.get());
    m_configManager->init(m_window);
    m_configManager->setQuitCallback([this]() { quit(); });

    // Wire all panel callbacks, menu actions, dialog setup
    initWiring();

    // Restore workspace state
    m_uiManager->restoreVisibilityFromConfig();
    i64 lastModelId = cfg.getLastSelectedModelId();
    if (lastModelId > 0 && m_libraryManager) {
        auto record = m_libraryManager->getModel(lastModelId);
        if (record) {
            onModelSelected(lastModelId);
            if (m_uiManager->libraryPanel())
                m_uiManager->libraryPanel()->setSelectedModelId(lastModelId);
        }
    }

    // Detect incomplete import from prior session
    if (m_importLog->exists()) {
        m_mainThreadQueue->enqueue([]() {
            log::info("App", "Previous import log found — resume available from library panel");
        });
    }

    // Auto-start CNC simulator (always-connected mode)
    m_cncController->connectSimulator();
    m_uiManager->showCncPanels(true);

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
    if (m_backgroundTagger && m_backgroundTagger->isActive()) {
        m_backgroundTagger->stop();
        m_uiManager->showTaggerShutdownDialog(&m_backgroundTagger->progress());
        return;
    }
    m_running = false;
}

auto Application::mainThreadQueue() -> MainThreadQueue& {
    return *m_mainThreadQueue;
}

void Application::processEvents() {
    std::vector<std::string> droppedFiles;
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            quit();
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window))
            quit();
        // Detect monitor change for DPI scaling
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MOVED) {
            int newDisplay = SDL_GetWindowDisplayIndex(m_window);
            if (newDisplay != m_displayIndex) {
                m_displayIndex = newDisplay;
                float newDpi = detectDpiScale();
                if (std::abs(newDpi - m_dpiScale) > 0.01f) {
                    m_dpiScale = newDpi;
                    float newScale = m_dpiScale * Config::instance().getUiScale();
                    rebuildFontAtlas(newScale);
                }
            }
        }
        if (event.type == SDL_DROPFILE && event.drop.file != nullptr) {
            droppedFiles.emplace_back(event.drop.file);
            SDL_free(event.drop.file);
        }
    }
    if (!droppedFiles.empty())
        m_fileIOManager->onFilesDropped(droppedFiles);
}

void Application::update() {
    if (m_mainThreadQueue)
        m_mainThreadQueue->processAll();
    m_fileIOManager->processCompletedImports(m_uiManager->viewportPanel(),
                                             m_uiManager->propertiesPanel(),
                                             m_uiManager->libraryPanel(),
                                             [this](bool show) {
                                                 m_uiManager->showStartPage() = show;
                                             });
    // Update simulation in gcode panel each frame
    if (m_uiManager && m_uiManager->gcodePanel())
        m_uiManager->gcodePanel()->updateSimulation(ImGui::GetIO().DeltaTime);

    // Poll gamepad input each frame
    if (m_gamepadInput)
        m_gamepadInput->update(ImGui::GetIO().DeltaTime);

    // Periodic serial port scan — update available ports for menu bar Connect button
    u64 ticks = SDL_GetTicks64();
    if (ticks - m_lastPortScanMs >= 2000) {
        m_lastPortScanMs = ticks;
        auto ports = listSerialPorts();
        m_uiManager->setAvailablePorts(ports);

        // Notify user once when a new device appears during simulation
        if (!ports.empty() && m_cncController->isSimulating() && m_lastConnectedPort.empty()) {
            std::string portList;
            for (const auto& p : ports) {
                if (!portList.empty()) portList += ", ";
                portList += p;
            }
            ToastManager::instance().show(
                ToastType::Info, "CNC Device Detected", portList, 5.0f);
            // Set sentinel so we only toast once per appearance
            m_lastConnectedPort = "__notified__";
        }
        if (ports.empty() && m_lastConnectedPort == "__notified__") {
            m_lastConnectedPort.clear();
        }
    }

    m_configManager->poll(ticks);
}

void Application::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;
    if (!Config::instance().getEnableFloatingWindows())
        dockFlags |= ImGuiDockNodeFlags_NoUndocking;
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);
    if (m_uiManager->isFirstFrame()) {
        m_uiManager->clearFirstFrame();
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr ||
            ImGui::DockBuilderGetNode(dockspaceId)->IsLeafNode())
            m_uiManager->setupDefaultDockLayout(dockspaceId);
    }

    m_uiManager->handleKeyboardShortcuts();
    m_uiManager->renderMenuBar();
    m_uiManager->renderPanels();
    m_uiManager->renderBackgroundUI(ImGui::GetIO().DeltaTime, &m_loadingState);
    m_uiManager->renderRestartPopup([this]() { m_configManager->relaunchApp(); });
    m_uiManager->renderAboutDialog();

    ImGui::Render();
    int displayW = 0, displayH = 0;
    SDL_GL_GetDrawableSize(m_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(m_window, m_glContext);
    }
}

void Application::shutdown() {
    if (!m_initialized)
        return;

    // Join load thread before destroying anything it references
    if (m_loadThread.joinable())
        m_loadThread.join();

    // Save current camera state before shutdown
    if (m_focusedModelId > 0 && m_uiManager && m_uiManager->viewportPanel() && m_database) {
        auto camState = m_uiManager->viewportPanel()->getCameraState();
        ModelRepository repo(*m_database);
        repo.updateCameraState(m_focusedModelId, camState);
    }

    m_configManager->saveWorkspaceState();

    // Shutdown managers in reverse creation order
    m_configManager.reset();
    m_fileIOManager.reset();
    m_uiManager.reset();

    // Stop background tagger cleanly
    if (m_backgroundTagger) {
        m_backgroundTagger->stop();
        m_backgroundTagger->join();
    }
    m_backgroundTagger.reset();
    m_importLog.reset();

    // Destroy core systems
    m_gamepadInput.reset();  // Must be destroyed before CncController
    m_toolDatabase.reset();
    m_cncController.reset();
    m_descriptorService.reset();
    m_geminiService.reset();
    m_costRepo.reset();
    m_cutPlanRepo.reset();
    m_cutListFile.reset();
    m_jobRepo.reset();
    m_gcodeRepo.reset();
    m_modelRepo.reset();
    m_importQueue.reset();
    m_storageManager.reset();
    m_mainThreadQueue->shutdown();
    m_mainThreadQueue.reset();
    m_connectionPool.reset();
    m_workspace.reset();
    m_thumbnailGenerator.reset();
    m_projectManager.reset();
    m_libraryManager.reset();
    m_projectExportManager.reset();
    m_graphManager.reset(); // Must be destroyed before m_database
    m_database.reset();

    // Destroy any multi-viewport platform windows before backend shutdown
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::DestroyPlatformWindows();
    }

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

float Application::detectDpiScale() const {
    int windowW = 0, drawableW = 0;
    SDL_GetWindowSize(m_window, &windowW, nullptr);
    SDL_GL_GetDrawableSize(m_window, &drawableW, nullptr);
    if (windowW > 0 && drawableW > 0) {
        return static_cast<float>(drawableW) / static_cast<float>(windowW);
    }
    return 1.0f;
}

void Application::rebuildFontAtlas(float scale) {
    auto& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Load Inter at scaled size
    {
#include "ui/fonts/fa_solid_900.h"
#include "ui/fonts/inter_regular.h"
        const float fontSize = 16.0f * scale;
        const float iconSize = 14.0f * scale;

        ImFontConfig fontCfg;
        fontCfg.OversampleH = 2;
        fontCfg.OversampleV = 1;
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(inter_regular_compressed_data_base85,
                                                       fontSize,
                                                       &fontCfg);

        static const ImWchar iconRanges[] = {0xf000, 0xf8ff, 0};
        ImFontConfig iconCfg;
        iconCfg.MergeMode = true;
        iconCfg.PixelSnapH = true;
        iconCfg.GlyphMinAdvanceX = iconSize;
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(
            fa_solid_900_compressed_data_base85, iconSize, &iconCfg, iconRanges);
    }

    // Scale style values to match font size
    Theme::applyBaseStyle();
    ImGui::GetStyle().ScaleAllSizes(scale);

    // ImGui 1.92+ with RendererHasTextures: backend builds and uploads atlas automatically
    m_uiScale = scale;
}

} // namespace dw
