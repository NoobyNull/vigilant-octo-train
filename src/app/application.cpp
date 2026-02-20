// Digital Workshop - Application Implementation
// Thin coordinator: SDL/GL lifecycle, manager creation, event loop,
// and onModelSelected business logic.

#include "app/application.h"

#include <cstdio>

#include <SDL.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/database/connection_pool.h"
#include "core/database/database.h"
#include "core/database/schema.h"
#include "core/events/event_bus.h"
#include "core/import/import_queue.h"
#include "core/library/library_manager.h"
#include "core/loaders/loader_factory.h"
#include "core/loaders/texture_loader.h"
#include "core/materials/material_archive.h"
#include "core/materials/material_manager.h"
#include "core/paths/app_paths.h"
#include "core/threading/main_thread_queue.h"
#include "core/threading/thread_pool.h"
#include "core/utils/log.h"
#include "core/utils/thread_utils.h"
#include "managers/config_manager.h"
#include "managers/file_io_manager.h"
#include "managers/ui_manager.h"
#include "render/texture.h"
#include "render/thumbnail_generator.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/materials_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/start_page.h"
#include "ui/panels/viewport_panel.h"
#include "ui/widgets/toast.h"
#include "version.h"

namespace dw {

Application::Application() = default;
Application::~Application() {
    shutdown();
}

bool Application::init() {
    if (m_initialized)
        return true;

    paths::ensureDirectoriesExist();
    Config::instance().load();
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
    m_window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                startWidth, startHeight, windowFlags);
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
    static std::string imguiIniPath = (paths::getConfigDir() / "imgui.ini").string();
    io.IniFilename = imguiIniPath.c_str();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize core systems
    dw::threading::initMainThread();
    m_mainThreadQueue = std::make_unique<MainThreadQueue>();
    m_eventBus = std::make_unique<EventBus>();

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

    m_libraryManager = std::make_unique<LibraryManager>(*m_database);
    m_projectManager = std::make_unique<ProjectManager>(*m_database);
    m_materialManager = std::make_unique<MaterialManager>(*m_database);
    m_materialManager->seedDefaults();
    m_workspace = std::make_unique<Workspace>();

    m_thumbnailGenerator = std::make_unique<ThumbnailGenerator>();
    if (m_thumbnailGenerator->initialize()) {
        m_libraryManager->setThumbnailGenerator(m_thumbnailGenerator.get());
    }

    m_importQueue = std::make_unique<ImportQueue>(*m_connectionPool, m_libraryManager.get());

    // Initialize managers
    m_uiManager = std::make_unique<UIManager>();
    m_uiManager->init(m_libraryManager.get(), m_projectManager.get(), m_materialManager.get());

    m_fileIOManager = std::make_unique<FileIOManager>(
        m_eventBus.get(), m_database.get(), m_libraryManager.get(), m_projectManager.get(),
        m_importQueue.get(), m_workspace.get(), m_uiManager->fileDialog(),
        m_thumbnailGenerator.get());

    m_configManager = std::make_unique<ConfigManager>(m_eventBus.get(), m_uiManager.get());
    m_configManager->init(m_window);
    m_configManager->setQuitCallback([this]() { quit(); });

    // Wire StatusBar cancel button to ImportQueue
    m_uiManager->setImportCancelCallback([this]() {
        if (m_importQueue) {
            m_importQueue->cancel();
        }
    });

    // Wire ImportQueue callbacks for UI feedback
    m_importQueue->setOnBatchComplete([this](const ImportBatchSummary& summary) {
        // Post to main thread for UI updates
        m_mainThreadQueue->enqueue([this, summary]() {
            // Show error toasts if config allows and there are failures
            if (Config::instance().getShowImportErrorToasts()) {
                if (summary.hasIssues()) {
                    if (summary.failedCount > 0) {
                        std::string msg =
                            std::to_string(summary.failedCount) + " file(s) failed to import";
                        ToastManager::instance().show(ToastType::Error, "Import Errors", msg);
                    }
                    if (summary.duplicateCount > 0) {
                        std::string msg =
                            std::to_string(summary.duplicateCount) + " duplicate(s) skipped";
                        ToastManager::instance().show(ToastType::Warning, "Duplicates", msg);
                    }
                } else if (summary.successCount > 0) {
                    std::string msg =
                        std::to_string(summary.successCount) + " file(s) imported successfully";
                    ToastManager::instance().show(ToastType::Success, "Import Complete", msg);
                }
            }
        });
    });

    // Wire StartPage callbacks
    if (m_uiManager->startPage()) {
        auto* sp = m_uiManager->startPage();
        auto hideStart = [this](bool show) { m_uiManager->showStartPage() = show; };
        sp->setOnNewProject([this, hideStart]() { m_fileIOManager->newProject(hideStart); });
        sp->setOnOpenProject([this, hideStart]() { m_fileIOManager->openProject(hideStart); });
        sp->setOnImportModel([this]() {
            m_fileIOManager->importModel();
            m_uiManager->showStartPage() = false;
        });
        sp->setOnOpenRecentProject([this, hideStart](const Path& path) {
            m_fileIOManager->openRecentProject(path, hideStart);
        });
    }

    // Wire panel callbacks
    if (m_uiManager->libraryPanel()) {
        m_uiManager->libraryPanel()->setOnModelSelected([this](int64_t modelId) {
            if (!m_libraryManager)
                return;
            auto record = m_libraryManager->getModel(modelId);
            if (record && m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->setModelRecord(*record);
        });
        m_uiManager->libraryPanel()->setOnModelOpened(
            [this](int64_t modelId) { onModelSelected(modelId); });
    }

    if (m_uiManager->projectPanel()) {
        auto hideStart = [this](bool show) { m_uiManager->showStartPage() = show; };
        m_uiManager->projectPanel()->setOnModelSelected(
            [this](int64_t modelId) { onModelSelected(modelId); });
        m_uiManager->projectPanel()->setOpenProjectCallback(
            [this, hideStart]() { m_fileIOManager->openProject(hideStart); });
        m_uiManager->projectPanel()->setSaveProjectCallback(
            [this]() { m_fileIOManager->saveProject(); });
    }

    if (m_uiManager->propertiesPanel()) {
        m_uiManager->propertiesPanel()->setOnMeshModified([this]() {
            auto mesh = m_workspace->getFocusedMesh();
            if (mesh && m_uiManager->viewportPanel())
                m_uiManager->viewportPanel()->setMesh(mesh);
        });
        m_uiManager->propertiesPanel()->setOnColorChanged([this](const Color& color) {
            if (m_uiManager->viewportPanel())
                m_uiManager->viewportPanel()->renderSettings().objectColor = color;
        });
    }

    // Wire MaterialsPanel callbacks
    if (m_uiManager->materialsPanel()) {
        m_uiManager->materialsPanel()->setOnMaterialAssigned(
            [this](int64_t materialId) { assignMaterialToCurrentModel(materialId); });
    }

    // Wire UIManager action callbacks (menu bar and keyboard shortcuts)
    auto hideStart = [this](bool show) { m_uiManager->showStartPage() = show; };
    m_uiManager->setOnNewProject([this, hideStart]() { m_fileIOManager->newProject(hideStart); });
    m_uiManager->setOnOpenProject([this, hideStart]() { m_fileIOManager->openProject(hideStart); });
    m_uiManager->setOnSaveProject([this]() { m_fileIOManager->saveProject(); });
    m_uiManager->setOnImportModel([this]() { m_fileIOManager->importModel(); });
    m_uiManager->setOnExportModel([this]() { m_fileIOManager->exportModel(); });
    m_uiManager->setOnQuit([this]() { quit(); });
    m_uiManager->setOnSpawnSettings([this]() { m_configManager->spawnSettingsApp(); });

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
        if (event.type == SDL_QUIT)
            quit();
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(m_window))
            quit();
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
    m_fileIOManager->processCompletedImports(
        m_uiManager->viewportPanel(), m_uiManager->propertiesPanel(), m_uiManager->libraryPanel(),
        [this](bool show) { m_uiManager->showStartPage() = show; });
    m_configManager->poll(SDL_GetTicks64());
}

void Application::render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
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
}

void Application::onModelSelected(int64_t modelId) {
    if (!m_libraryManager)
        return;

    // Get model record on main thread (fast DB read)
    auto record = m_libraryManager->getModel(modelId);
    if (!record)
        return;

    // Track focused model ID for material assignment
    m_focusedModelId = modelId;

    // Check if this model has a material assigned — load texture proactively
    if (m_materialManager) {
        auto assignedMaterial = m_materialManager->getModelMaterial(modelId);
        if (assignedMaterial) {
            // Load material texture for this model
            loadMaterialTextureForModel(modelId);
            // Update PropertiesPanel material display
            if (m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->setMaterial(*assignedMaterial);
        } else {
            // No material assigned — clear active texture and material display
            m_activeMaterialTexture.reset();
            m_activeMaterialId = -1;
            if (m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->clearMaterial();
        }
    }

    // Bump generation to invalidate any in-flight load
    uint64_t gen = ++m_loadingState.generation;
    m_loadingState.set(record->name);

    // Join previous load thread if still running
    if (m_loadThread.joinable())
        m_loadThread.join();

    // Capture what we need for the worker
    Path filePath = record->filePath;
    std::string name = record->name;

    m_loadThread = std::thread([this, filePath, name, gen]() {
        auto loadResult = LoaderFactory::load(filePath);
        if (!loadResult) {
            m_loadingState.reset();
            return;
        }
        loadResult.mesh->setName(name);
        auto mesh = loadResult.mesh;

        // Post result to main thread via MainThreadQueue
        m_mainThreadQueue->enqueue([this, mesh, name, gen]() {
            // Check generation — if user clicked another model, discard
            if (gen != m_loadingState.generation.load())
                return;
            m_loadingState.reset();

            m_workspace->setFocusedMesh(mesh);
            if (m_uiManager->viewportPanel())
                m_uiManager->viewportPanel()->setMesh(mesh);
            if (m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->setMesh(mesh, name);
        });
    });
}

void Application::assignMaterialToCurrentModel(int64_t materialId) {
    if (!m_materialManager || !m_workspace)
        return;

    // Get the currently focused mesh
    auto mesh = m_workspace->getFocusedMesh();
    if (!mesh)
        return;

    // Get material record
    auto material = m_materialManager->getMaterial(materialId);
    if (!material)
        return;

    // Persist the assignment to the database
    if (m_focusedModelId > 0) {
        m_materialManager->assignMaterialToModel(materialId, m_focusedModelId);
    }

    // Load and upload material texture if archive path exists
    m_activeMaterialTexture.reset();
    if (!material->archivePath.empty()) {
        auto matData = MaterialArchive::load(material->archivePath.string());
        if (matData && !matData->textureData.empty()) {
            // Decode texture PNG from memory
            auto textureOpt = TextureLoader::loadPNGFromMemory(matData->textureData.data(),
                                                               matData->textureData.size());
            if (textureOpt) {
                m_activeMaterialTexture = std::make_unique<Texture>();
                m_activeMaterialTexture->upload(textureOpt->pixels.data(), textureOpt->width,
                                                textureOpt->height);
            }
        }
    }

    // Generate UVs if needed
    if (mesh->needsUVGeneration()) {
        mesh->generatePlanarUVs(material->grainDirectionDeg);
    }

    // Store active material ID
    m_activeMaterialId = materialId;

    // Update PropertiesPanel to show material info
    if (m_uiManager->propertiesPanel()) {
        m_uiManager->propertiesPanel()->setMaterial(*material);
    }

    // Update viewport material texture pointer
    if (m_uiManager->viewportPanel()) {
        m_uiManager->viewportPanel()->setMaterialTexture(m_activeMaterialTexture.get());
        // Re-upload mesh with updated UVs
        m_uiManager->viewportPanel()->setMesh(mesh);
    }
}

void Application::loadMaterialTextureForModel(int64_t modelId) {
    if (!m_materialManager)
        return;

    auto material = m_materialManager->getModelMaterial(modelId);
    if (!material) {
        m_activeMaterialTexture.reset();
        m_activeMaterialId = -1;
        if (m_uiManager && m_uiManager->viewportPanel())
            m_uiManager->viewportPanel()->setMaterialTexture(nullptr);
        return;
    }

    m_activeMaterialId = material->id;
    m_activeMaterialTexture.reset();

    if (!material->archivePath.empty()) {
        auto matData = MaterialArchive::load(material->archivePath.string());
        if (matData && !matData->textureData.empty()) {
            auto textureOpt = TextureLoader::loadPNGFromMemory(matData->textureData.data(),
                                                               matData->textureData.size());
            if (textureOpt) {
                m_activeMaterialTexture = std::make_unique<Texture>();
                m_activeMaterialTexture->upload(textureOpt->pixels.data(), textureOpt->width,
                                                textureOpt->height);
            }
        }
    }

    // Update viewport texture pointer
    if (m_uiManager && m_uiManager->viewportPanel())
        m_uiManager->viewportPanel()->setMaterialTexture(m_activeMaterialTexture.get());
}

void Application::shutdown() {
    if (!m_initialized)
        return;

    // Join load thread before destroying anything it references
    if (m_loadThread.joinable())
        m_loadThread.join();

    m_configManager->saveWorkspaceState();

    // Shutdown managers in reverse creation order
    m_configManager.reset();
    m_fileIOManager.reset();
    m_uiManager.reset();

    // Destroy core systems
    m_importQueue.reset();
    m_mainThreadQueue->shutdown();
    m_mainThreadQueue.reset();
    m_connectionPool.reset();
    m_workspace.reset();
    m_thumbnailGenerator.reset();
    m_projectManager.reset();
    m_libraryManager.reset();
    m_database.reset();
    m_eventBus.reset();

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
