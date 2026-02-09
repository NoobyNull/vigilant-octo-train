// Digital Workshop - Application Implementation

#include "app/application.h"

#include <cstdio>
#include <cstdlib>

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/config/config_watcher.h"
#include "core/database/connection_pool.h"
#include "core/database/database.h"
#include "core/database/schema.h"
#include "core/events/event_bus.h"
#include "core/import/import_queue.h"
#include "core/library/library_manager.h"
#include "core/paths/app_paths.h"
#include "core/threading/main_thread_queue.h"
#include "core/utils/log.h"
#include "core/utils/thread_utils.h"
#include "managers/file_io_manager.h"
#include "managers/ui_manager.h"
#include "render/thumbnail_generator.h"
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

    // Create window -- restore size from config
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
    // Initialize main thread (record thread ID for assertions)
    dw::threading::initMainThread();

    // Initialize MainThreadQueue (must exist before ImportQueue)
    m_mainThreadQueue = std::make_unique<MainThreadQueue>();

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

    // Initialize connection pool for background workers (2 connections)
    m_connectionPool = std::make_unique<ConnectionPool>(paths::getDatabasePath(), 2);

    m_libraryManager = std::make_unique<LibraryManager>(*m_database);
    m_projectManager = std::make_unique<ProjectManager>(*m_database);
    m_workspace = std::make_unique<Workspace>();

    // Initialize thumbnail generator and connect to library manager
    m_thumbnailGenerator = std::make_unique<ThumbnailGenerator>();
    if (m_thumbnailGenerator->initialize()) {
        m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator.get());
    }

    // Initialize import queue with connection pool
    m_importQueue = std::make_unique<ImportQueue>(*m_connectionPool);

    // Initialize UI Manager (creates all panels and dialogs)
    m_uiManager = std::make_unique<UIManager>();
    m_uiManager->init(m_libraryManager.get(), m_projectManager.get());

    // Apply config render settings to viewport
    m_uiManager->applyRenderSettingsFromConfig();

    // Initialize File I/O Manager (orchestrates import, export, project operations)
    m_fileIOManager = std::make_unique<FileIOManager>(
        m_eventBus.get(), m_database.get(), m_libraryManager.get(), m_projectManager.get(),
        m_importQueue.get(), m_workspace.get(), m_uiManager->fileDialog(),
        m_thumbnailGenerator.get());

    // Wire StartPage callbacks (after both UIManager and FileIOManager exist)
    if (m_uiManager->startPage()) {
        m_uiManager->startPage()->setOnNewProject([this]() {
            m_fileIOManager->newProject([this](bool show) { m_uiManager->showStartPage() = show; });
        });
        m_uiManager->startPage()->setOnOpenProject([this]() {
            m_fileIOManager->openProject(
                [this](bool show) { m_uiManager->showStartPage() = show; });
        });
        m_uiManager->startPage()->setOnImportModel([this]() {
            m_fileIOManager->importModel();
            m_uiManager->showStartPage() = false;
        });
        m_uiManager->startPage()->setOnOpenRecentProject([this](const Path& path) {
            m_fileIOManager->openRecentProject(
                path, [this](bool show) { m_uiManager->showStartPage() = show; });
        });
    }

    // Wire panel callbacks
    if (m_uiManager->libraryPanel()) {
        // Single-click: show metadata in properties panel
        m_uiManager->libraryPanel()->setOnModelSelected([this](int64_t modelId) {
            if (!m_libraryManager)
                return;
            auto record = m_libraryManager->getModel(modelId);
            if (record && m_uiManager->propertiesPanel()) {
                m_uiManager->propertiesPanel()->setModelRecord(*record);
            }
        });

        // Double-click: load mesh into viewport
        m_uiManager->libraryPanel()->setOnModelOpened(
            [this](int64_t modelId) { onModelSelected(modelId); });
    }

    if (m_uiManager->projectPanel()) {
        m_uiManager->projectPanel()->setOnModelSelected(
            [this](int64_t modelId) { onModelSelected(modelId); });
        m_uiManager->projectPanel()->setOpenProjectCallback([this]() {
            m_fileIOManager->openProject(
                [this](bool show) { m_uiManager->showStartPage() = show; });
        });
        m_uiManager->projectPanel()->setSaveProjectCallback(
            [this]() { m_fileIOManager->saveProject(); });
    }

    if (m_uiManager->propertiesPanel()) {
        m_uiManager->propertiesPanel()->setOnMeshModified([this]() {
            // Re-upload mesh to GPU after transform operations
            auto mesh = m_workspace->getFocusedMesh();
            if (mesh && m_uiManager->viewportPanel()) {
                m_uiManager->viewportPanel()->setMesh(mesh);
            }
        });

        m_uiManager->propertiesPanel()->setOnColorChanged([this](const Color& color) {
            // Update renderer object color
            if (m_uiManager->viewportPanel()) {
                m_uiManager->viewportPanel()->renderSettings().objectColor = color;
            }
        });
    }

    // Wire UIManager action callbacks (menu bar and keyboard shortcuts)
    m_uiManager->setOnNewProject([this]() {
        m_fileIOManager->newProject([this](bool show) { m_uiManager->showStartPage() = show; });
    });
    m_uiManager->setOnOpenProject([this]() {
        m_fileIOManager->openProject([this](bool show) { m_uiManager->showStartPage() = show; });
    });
    m_uiManager->setOnSaveProject([this]() { m_fileIOManager->saveProject(); });
    m_uiManager->setOnImportModel([this]() { m_fileIOManager->importModel(); });
    m_uiManager->setOnExportModel([this]() { m_fileIOManager->exportModel(); });
    m_uiManager->setOnQuit([this]() { quit(); });
    m_uiManager->setOnSpawnSettings([this]() { spawnSettingsApp(); });

    // Set up config file watcher for live reload
    m_configWatcher = std::make_unique<ConfigWatcher>();
    m_configWatcher->setOnChanged([this]() { onConfigFileChanged(); });
    m_configWatcher->watch(Config::instance().configFilePath());
    m_lastAppliedUiScale = cfg.getUiScale();

    // Restore workspace state from config
    m_uiManager->restoreVisibilityFromConfig();

    // Restore last selected model
    {
        i64 lastModelId = cfg.getLastSelectedModelId();
        if (lastModelId > 0 && m_libraryManager) {
            auto record = m_libraryManager->getModel(lastModelId);
            if (record) {
                onModelSelected(lastModelId);
                if (m_uiManager->libraryPanel()) {
                    m_uiManager->libraryPanel()->setSelectedModelId(lastModelId);
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

auto Application::mainThreadQueue() -> MainThreadQueue& {
    return *m_mainThreadQueue;
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
        m_fileIOManager->onFilesDropped(droppedFiles);
    }
}

void Application::update() {
    // Process all pending main-thread tasks (from worker threads)
    if (m_mainThreadQueue) {
        m_mainThreadQueue->processAll();
    }

    // Process completed imports (thumbnail generation needs GL context)
    m_fileIOManager->processCompletedImports(
        m_uiManager->viewportPanel(), m_uiManager->propertiesPanel(), m_uiManager->libraryPanel(),
        [this](bool show) { m_uiManager->showStartPage() = show; });

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
    if (m_uiManager->isFirstFrame()) {
        m_uiManager->clearFirstFrame();
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr ||
            ImGui::DockBuilderGetNode(dockspaceId)->IsLeafNode()) {
            m_uiManager->setupDefaultDockLayout(dockspaceId);
        }
    }

    // Handle keyboard shortcuts
    m_uiManager->handleKeyboardShortcuts();

    // Render menu bar
    m_uiManager->renderMenuBar();

    // Render panels
    m_uiManager->renderPanels();

    // Import progress overlay
    m_uiManager->renderImportProgress(m_importQueue.get());

    // Restart required popup
    m_uiManager->renderRestartPopup([this]() { relaunchApp(); });

    // About popup
    m_uiManager->renderAboutDialog();

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

void Application::onModelSelected(int64_t modelId) {
    if (!m_libraryManager)
        return;

    auto mesh = m_libraryManager->loadMesh(modelId);
    if (!mesh)
        return;

    m_workspace->setFocusedMesh(mesh);
    if (m_uiManager->viewportPanel()) {
        m_uiManager->viewportPanel()->setMesh(mesh);
    }
    if (m_uiManager->propertiesPanel()) {
        auto record = m_libraryManager->getModel(modelId);
        std::string name = record ? record->name : "";
        m_uiManager->propertiesPanel()->setMesh(mesh, name);
    }
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

    // Render settings (live) -- delegated to UIManager
    m_uiManager->applyRenderSettingsFromConfig();

    // Log level (live)
    log::setLevel(static_cast<log::Level>(cfg.getLogLevel()));

    // UI scale requires restart
    if (cfg.getUiScale() != m_lastAppliedUiScale) {
        m_uiManager->showRestartPopup() = true;
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

    // Save panel visibility (delegated to UIManager)
    m_uiManager->saveVisibilityToConfig();

    // Save last selected model
    cfg.setLastSelectedModelId(m_uiManager->getSelectedModelId());

    cfg.save();
}

void Application::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Save workspace state before destroying anything
    saveWorkspaceState();

    // Destroy config watcher
    m_configWatcher.reset();

    // Destroy File I/O Manager (before UI Manager which owns FileDialog)
    m_fileIOManager.reset();

    // Destroy UI Manager (panels and dialogs)
    m_uiManager.reset();

    // Destroy core systems
    m_importQueue.reset();         // Joins worker thread, releases pooled connections
    m_mainThreadQueue->shutdown(); // Signal no more processing
    m_mainThreadQueue.reset();     // Destroy queue after worker joins
    m_connectionPool.reset();      // Closes pooled connections
    m_workspace.reset();
    m_thumbnailGenerator.reset();
    m_projectManager.reset();
    m_libraryManager.reset();
    m_database.reset(); // Close main thread connection
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
