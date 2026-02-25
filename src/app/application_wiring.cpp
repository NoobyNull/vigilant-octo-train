// Application wiring — panel callbacks, menu setup, dialog setup.

#include "app/application.h"

#include <string>
#include <thread>
#include <vector>

#include "app/workspace.h"
#include "core/config/config.h"
#include "core/database/connection_pool.h"
#include "core/database/model_repository.h"
#include "core/database/tool_database.h"
#include "core/database/cut_plan_repository.h"
#include "core/optimizer/cut_list_file.h"
#include "core/database/gcode_repository.h"
#include "core/cnc/cnc_controller.h"
#include "core/export/project_export_manager.h"
#include "core/import/background_tagger.h"
#include "core/import/import_queue.h"
#include "core/library/library_manager.h"
#include "core/loaders/loader_factory.h"
#include "core/paths/path_resolver.h"
#include "core/loaders/texture_loader.h"
#include "core/materials/gemini_descriptor_service.h"
#include "core/materials/gemini_material_service.h"
#include "core/materials/material_archive.h"
#include "core/materials/material_manager.h"
#include "core/threading/main_thread_queue.h"
#include "core/utils/log.h"
#include "managers/config_manager.h"
#include "managers/file_io_manager.h"
#include "managers/ui_manager.h"
#include "render/texture.h"
#include "ui/dialogs/import_options_dialog.h"
#include "ui/dialogs/import_summary_dialog.h"
#include "ui/dialogs/maintenance_dialog.h"
#include "ui/dialogs/progress_dialog.h"
#include "ui/dialogs/tag_image_dialog.h"
#include "ui/dialogs/tagger_shutdown_dialog.h"
#include "ui/panels/cost_panel.h"
#include "ui/panels/cut_optimizer_panel.h"
#include "ui/panels/gcode_panel.h"
#include "ui/panels/library_panel.h"
#include "ui/panels/materials_panel.h"
#include "ui/panels/project_panel.h"
#include "ui/panels/properties_panel.h"
#include "ui/panels/start_page.h"
#include "ui/panels/cnc_status_panel.h"
#include "ui/panels/cnc_jog_panel.h"
#include "ui/panels/cnc_console_panel.h"
#include "ui/panels/cnc_wcs_panel.h"
#include "ui/panels/cnc_tool_panel.h"
#include "ui/panels/cnc_job_panel.h"
#include "ui/panels/cnc_safety_panel.h"
#include "ui/panels/tool_browser_panel.h"
#include "ui/panels/viewport_panel.h"
#include "ui/widgets/toast.h"

namespace dw {

void Application::initWiring() {
    // Wire StatusBar cancel button to ImportQueue
    m_uiManager->setImportCancelCallback([this]() {
        if (m_importQueue)
            m_importQueue->cancel();
    });

    // Wire ImportQueue callbacks for UI feedback
    m_importQueue->setOnBatchComplete([this](const ImportBatchSummary& summary) {
        m_mainThreadQueue->enqueue([this, summary]() {
            if (Config::instance().getShowImportErrorToasts()) {
                if (summary.failedCount > 0)
                    ToastManager::instance().show(ToastType::Error,
                                                  "Import Errors",
                                                  std::to_string(summary.failedCount) +
                                                      " file(s) failed to import");
                if (summary.successCount > 0)
                    ToastManager::instance().show(ToastType::Success,
                                                  "Import Complete",
                                                  std::to_string(summary.successCount) +
                                                      " file(s) imported successfully");
            }
            if (summary.duplicateCount > 0)
                m_uiManager->showImportSummary(summary);

            // Start background tagger if "manage + tag" mode was selected
            if (m_importQueue->queueForTagging() && m_backgroundTagger &&
                !m_backgroundTagger->isActive()) {
                auto apiKey = Config::instance().getGeminiApiKey();
                if (!apiKey.empty())
                    m_backgroundTagger->start(apiKey);
            }
        });
    });

    // Wire import options dialog and confirm callback
    m_fileIOManager->setImportOptionsDialog(m_uiManager->importOptionsDialog());
    if (m_uiManager->importOptionsDialog()) {
        m_uiManager->importOptionsDialog()->setOnConfirm(
            [this](FileHandlingMode mode, bool tagAfterImport, const std::vector<Path>& paths) {
                if (m_importQueue && !paths.empty()) {
                    m_importQueue->setQueueForTagging(tagAfterImport);
                    m_importQueue->enqueue(paths, mode);
                }
            });
    }

    // Wire re-import callback for duplicate review
    if (m_uiManager->importSummaryDialog()) {
        m_uiManager->importSummaryDialog()->setOnReimport(
            [this](std::vector<DuplicateRecord> selected) {
                if (m_importQueue && !selected.empty())
                    m_importQueue->enqueueForReimport(selected);
            });
    }

    // Wire StartPage callbacks
    if (m_uiManager->startPage()) {
        auto* sp = m_uiManager->startPage();
        auto hideStart = [this](bool show) {
            m_uiManager->showStartPage() = show;
        };
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
        m_uiManager->libraryPanel()->setProjectManager(m_projectManager.get());
        m_uiManager->libraryPanel()->setOnGCodeAddToProject(
            [this](const std::vector<int64_t>& gcodeIds) {
                if (!m_projectManager || !m_projectManager->currentProject() || !m_gcodeRepo)
                    return;
                i64 pid = m_projectManager->currentProject()->id();
                for (int64_t gid : gcodeIds)
                    if (!m_gcodeRepo->isInProject(pid, gid))
                        m_gcodeRepo->addToProject(pid, gid);
            });

        m_uiManager->libraryPanel()->setOnModelSelected([this](int64_t modelId) {
            if (!m_libraryManager)
                return;
            auto record = m_libraryManager->getModel(modelId);
            if (record && m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->setModelRecord(*record);
        });
        m_uiManager->libraryPanel()->setOnModelOpened(
            [this](int64_t modelId) { onModelSelected(modelId); });

        m_uiManager->libraryPanel()->setOnRegenerateThumbnail(
            [this](const std::vector<int64_t>& modelIds) {
                if (!m_libraryManager || modelIds.empty())
                    return;

                // Single item: lightweight path (no progress dialog)
                if (modelIds.size() == 1) {
                    int64_t modelId = modelIds[0];
                    auto record = m_libraryManager->getModel(modelId);
                    if (!record) {
                        ToastManager::instance().show(ToastType::Error,
                                                      "Thumbnail Failed",
                                                      "Model not found in database");
                        return;
                    }
                    Path filePath = PathResolver::resolve(record->filePath, PathCategory::Support);
                    std::string modelName = record->name;
                    ToastManager::instance().show(ToastType::Info,
                                                  "Regenerating Thumbnail",
                                                  modelName);
                    std::thread([this, filePath, modelId, modelName]() {
                        auto result = LoaderFactory::load(filePath);
                        if (!result) {
                            m_mainThreadQueue->enqueue([modelName, error = result.error]() {
                                ToastManager::instance().show(
                                    ToastType::Error,
                                    "Thumbnail Failed",
                                    modelName + ": " +
                                        (error.empty() ? "failed to load file" : error));
                            });
                            return;
                        }
                        auto mesh = result.mesh;
                        m_mainThreadQueue->enqueue([this, mesh, modelId, modelName]() {
                            bool ok = generateMaterialThumbnail(modelId, *mesh);
                            if (m_uiManager->libraryPanel()) {
                                m_uiManager->libraryPanel()->invalidateThumbnail(modelId);
                                m_uiManager->libraryPanel()->refresh();
                            }
                            if (ok) {
                                ToastManager::instance().show(ToastType::Success,
                                                              "Thumbnail Updated",
                                                              modelName);
                            } else {
                                ToastManager::instance().show(ToastType::Error,
                                                              "Thumbnail Failed",
                                                              modelName + ": generation failed");
                            }
                        });
                    }).detach();
                    return;
                }

                // Batch path: progress dialog + single coordinator thread
                auto* progressDlg = m_uiManager->progressDialog();
                if (!progressDlg)
                    return;

                // Snapshot IDs and resolve file paths on main thread
                struct BatchItem {
                    int64_t id;
                    Path filePath;
                    std::string name;
                };
                auto items = std::make_shared<std::vector<BatchItem>>();
                items->reserve(modelIds.size());
                for (int64_t id : modelIds) {
                    auto record = m_libraryManager->getModel(id);
                    if (record) {
                        auto resolved = PathResolver::resolve(record->filePath, PathCategory::Support);
                        items->push_back({id, resolved, record->name});
                    }
                }
                if (items->empty())
                    return;

                progressDlg->start("Regenerating Thumbnails", static_cast<int>(items->size()));

                // Single coordinator thread processes items sequentially
                std::thread([this, items, progressDlg]() {
                    for (const auto& item : *items) {
                        if (progressDlg->isCancelled())
                            break;

                        auto result = LoaderFactory::load(item.filePath);
                        if (!result) {
                            m_mainThreadQueue->enqueue([name = item.name, error = result.error]() {
                                ToastManager::instance().show(
                                    ToastType::Error,
                                    "Thumbnail Failed",
                                    name + ": " + (error.empty() ? "failed to load file" : error));
                            });
                            progressDlg->advance(item.name);
                            continue;
                        }

                        auto mesh = result.mesh;
                        auto modelId = item.id;
                        auto modelName = item.name;

                        // GL work must happen on the main thread
                        m_mainThreadQueue->enqueue([this, mesh, modelId, modelName]() {
                            bool ok = generateMaterialThumbnail(modelId, *mesh);
                            if (m_uiManager->libraryPanel()) {
                                m_uiManager->libraryPanel()->invalidateThumbnail(modelId);
                            }
                            if (!ok) {
                                ToastManager::instance().show(ToastType::Error,
                                                              "Thumbnail Failed",
                                                              modelName + ": generation failed");
                            }
                        });
                        progressDlg->advance(item.name);
                    }

                    // Finish on main thread
                    m_mainThreadQueue->enqueue([this, progressDlg]() {
                        progressDlg->finish();
                        if (m_uiManager->libraryPanel()) {
                            m_uiManager->libraryPanel()->refresh();
                        }
                        ToastManager::instance().show(ToastType::Success,
                                                      "Thumbnails Updated",
                                                      "Batch regeneration complete");
                    });
                }).detach();
            });

        m_uiManager->libraryPanel()->setOnAssignDefaultMaterial([this](int64_t modelId) {
            i64 defaultMatId = Config::instance().getDefaultMaterialId();
            if (defaultMatId <= 0 || !m_materialManager)
                return;
            auto mat = m_materialManager->getMaterial(defaultMatId);
            if (!mat)
                return;
            // Open the model (which will trigger material assignment via onModelSelected)
            // then assign the default material
            m_materialManager->assignMaterialToModel(defaultMatId, modelId);
        });

        // Tag Image dialog: request callback -- spawn async Gemini classification
        auto* tagDlg = m_uiManager->tagImageDialog();
        tagDlg->setOnRequest([this, tagDlg](int64_t modelId) {
            if (!m_descriptorService || !m_mainThreadQueue)
                return;
            std::string apiKey = Config::instance().getGeminiApiKey();
            if (apiKey.empty()) {
                log::warning("App", "Gemini API key not configured");
                DescriptorResult err;
                err.error = "Gemini API key not configured";
                tagDlg->setResult(err);
                return;
            }
            auto record = m_libraryManager->getModel(modelId);
            if (!record || record->thumbnailPath.empty()) {
                DescriptorResult err;
                err.error = "Model has no thumbnail";
                tagDlg->setResult(err);
                return;
            }

            auto* svc = m_descriptorService.get();
            auto* mtq = m_mainThreadQueue.get();
            std::string thumbPath = record->thumbnailPath.string();

            std::thread([svc, mtq, tagDlg, thumbPath, apiKey]() {
                auto result = svc->describe(thumbPath, apiKey);
                mtq->enqueue([tagDlg, result]() { tagDlg->setResult(result); });
            }).detach();
        });

        // Tag Image dialog: save callback -- persist edited results
        tagDlg->setOnSave([this](int64_t modelId, const DescriptorResult& result) {
            auto* libMgr = m_libraryManager.get();
            libMgr->updateDescriptor(
                modelId, result.title, result.description, result.hoverNarrative);
            auto existing = libMgr->getModel(modelId);
            if (existing) {
                auto tags = existing->tags;
                for (const auto& kw : result.keywords)
                    tags.push_back(kw);
                for (const auto& assoc : result.associations)
                    tags.push_back(assoc);
                libMgr->updateTags(modelId, tags);
            }
            if (!result.categories.empty())
                libMgr->resolveAndAssignCategories(modelId, result.categories);
            m_uiManager->libraryPanel()->refresh();
            m_uiManager->libraryPanel()->invalidateThumbnail(modelId);
            if (m_uiManager->propertiesPanel()) {
                auto updated = libMgr->getModel(modelId);
                if (updated)
                    m_uiManager->propertiesPanel()->setModelRecord(*updated);
            }
            ToastManager::instance().show(ToastType::Success, "Tagged", result.title);
            log::infof("App",
                       "Tagged model %lld as: %s",
                       static_cast<long long>(modelId),
                       result.title.c_str());
        });

        // "Tag Image" context menu action
        m_uiManager->libraryPanel()->setOnTagImage([this,
                                                    tagDlg](const std::vector<int64_t>& modelIds) {
            if (modelIds.empty()) {
                return;
            }

            // Single selection: open interactive dialog
            if (modelIds.size() == 1) {
                auto record = m_libraryManager->getModel(modelIds[0]);
                if (!record) {
                    return;
                }
                GLuint tex = m_uiManager->libraryPanel()->getThumbnailTextureForModel(modelIds[0]);
                tagDlg->open(*record, tex);
                return;
            }

            // Multi selection: fire-and-forget batch tagging
            std::string apiKey = Config::instance().getGeminiApiKey();
            if (apiKey.empty()) {
                log::warning("App", "Gemini API key not configured");
                return;
            }

            auto* svc = m_descriptorService.get();
            auto* libMgr = m_libraryManager.get();
            auto* mtq = m_mainThreadQueue.get();
            auto* libPanel = m_uiManager->libraryPanel();
            auto* propPanel = m_uiManager->propertiesPanel();
            size_t count = modelIds.size();

            for (int64_t modelId : modelIds) {
                auto record = m_libraryManager->getModel(modelId);
                if (!record || record->thumbnailPath.empty()) {
                    continue;
                }

                std::string thumbPath = record->thumbnailPath.string();
                std::string modelName = record->name;

                std::thread([svc,
                             libMgr,
                             mtq,
                             libPanel,
                             propPanel,
                             modelId,
                             modelName,
                             thumbPath,
                             apiKey,
                             count]() {
                    auto result = svc->describe(thumbPath, apiKey);
                    mtq->enqueue(
                        [libMgr, libPanel, propPanel, modelId, modelName, result, count]() {
                            if (result.success) {
                                libMgr->updateDescriptor(modelId,
                                                         result.title,
                                                         result.description,
                                                         result.hoverNarrative);
                                auto existing = libMgr->getModel(modelId);
                                if (existing) {
                                    auto tags = existing->tags;
                                    for (const auto& kw : result.keywords) {
                                        tags.push_back(kw);
                                    }
                                    for (const auto& assoc : result.associations) {
                                        tags.push_back(assoc);
                                    }
                                    libMgr->updateTags(modelId, tags);
                                }
                                if (!result.categories.empty()) {
                                    libMgr->resolveAndAssignCategories(modelId, result.categories);
                                }
                                libPanel->refresh();
                                libPanel->invalidateThumbnail(modelId);
                                ToastManager::instance().show(ToastType::Success,
                                                              "Tagged",
                                                              result.title);
                                log::infof("App",
                                           "Tagged %s as: %s",
                                           modelName.c_str(),
                                           result.title.c_str());
                            } else {
                                log::warningf("App",
                                              "Descriptor failed for %s: %s",
                                              modelName.c_str(),
                                              result.error.c_str());
                            }
                        });
                }).detach();
            }

            ToastManager::instance().show(ToastType::Info,
                                          "Tagging",
                                          "Classifying " + std::to_string(count) + " models...");
        });
    }

    if (m_uiManager->projectPanel()) {
        auto hideStart = [this](bool show) {
            m_uiManager->showStartPage() = show;
        };
        m_uiManager->projectPanel()->setOnModelSelected(
            [this](int64_t modelId) { onModelSelected(modelId); });
        m_uiManager->projectPanel()->setOpenProjectCallback(
            [this, hideStart]() { m_fileIOManager->openProject(hideStart); });
        m_uiManager->projectPanel()->setSaveProjectCallback(
            [this]() { m_fileIOManager->saveProject(); });
        m_uiManager->projectPanel()->setOnOpenRecentProject([this, hideStart](const Path& path) {
            m_fileIOManager->openRecentProject(path, hideStart);
        });
        m_uiManager->projectPanel()->setExportProjectCallback(
            [this]() { m_fileIOManager->exportProjectArchive(); });

        // Cross-panel navigation from ProjectPanel
        m_uiManager->projectPanel()->setOnGCodeSelected([this](i64 gcodeId) {
            if (!m_gcodeRepo) return;
            auto rec = m_gcodeRepo->findById(gcodeId);
            if (rec && m_uiManager->gcodePanel()) {
                m_uiManager->gcodePanel()->setOpen(true);
                m_uiManager->gcodePanel()->loadFile(
                    PathResolver::resolve(rec->filePath, PathCategory::GCode).string());
            }
        });
        m_uiManager->projectPanel()->setOnMaterialSelected([this](i64 materialId) {
            if (m_uiManager->materialsPanel()) {
                m_uiManager->materialsPanel()->setOpen(true);
                m_uiManager->materialsPanel()->selectMaterial(materialId);
            }
        });
        m_uiManager->projectPanel()->setOnCostSelected([this](i64 estimateId) {
            if (m_uiManager->costPanel()) {
                m_uiManager->costPanel()->setOpen(true);
                m_uiManager->costPanel()->selectEstimate(estimateId);
            }
        });
        m_uiManager->projectPanel()->setOnCutPlanSelected([this](i64 planId) {
            if (!m_cutPlanRepo || !m_cutListFile) return;
            auto rec = m_cutPlanRepo->findById(planId);
            if (rec && m_uiManager->cutOptimizerPanel()) {
                // Convert DB record to CutListFile::LoadResult for the panel
                CutListFile::LoadResult lr;
                lr.name = rec->name;
                lr.algorithm = rec->algorithm;
                lr.allowRotation = rec->allowRotation;
                lr.kerf = rec->kerf;
                lr.margin = rec->margin;
                if (!rec->sheetConfigJson.empty())
                    lr.sheet = CutPlanRepository::jsonToSheet(rec->sheetConfigJson);
                if (!rec->partsJson.empty())
                    lr.parts = CutPlanRepository::jsonToParts(rec->partsJson);
                if (!rec->resultJson.empty())
                    lr.result = CutPlanRepository::jsonToCutPlan(rec->resultJson);
                m_uiManager->cutOptimizerPanel()->setOpen(true);
                m_uiManager->cutOptimizerPanel()->loadCutPlan(lr);
            }
        });
    }
    // Wire CutOptimizerPanel and GCodePanel persistence
    if (auto* cop = m_uiManager->cutOptimizerPanel()) {
        cop->setCutListFile(m_cutListFile.get());
        cop->setProjectManager(m_projectManager.get());
        cop->setModelRepository(m_modelRepo.get());
        cop->setOnAddToCost([this](const std::string& name, int qty, float rate, float total) {
            if (!m_costRepo) return;
            // Create a new cost estimate with the cut plan material cost
            CostEstimate estimate;
            estimate.name = "Cut Plan — " + name;
            if (m_projectManager && m_projectManager->currentProject())
                estimate.projectId = m_projectManager->currentProject()->id();
            CostItem item;
            item.name = name + " sheets";
            item.category = CostCategory::Material;
            item.quantity = static_cast<f64>(qty);
            item.rate = static_cast<f64>(rate);
            item.total = static_cast<f64>(total);
            estimate.items.push_back(item);
            estimate.recalculate();
            m_costRepo->insert(estimate);
        });
    }
    if (auto* gcp = m_uiManager->gcodePanel()) {
        gcp->setGCodeRepository(m_gcodeRepo.get());
        gcp->setProjectManager(m_projectManager.get());
        gcp->setCncController(m_cncController.get());
        gcp->setToolDatabase(m_toolDatabase.get());

        // Wire CNC callbacks to update gcode panel and all CNC panels
        auto* csp = m_uiManager->cncStatusPanel();
        auto* jogp = m_uiManager->cncJogPanel();
        auto* conp = m_uiManager->cncConsolePanel();
        auto* wcsp = m_uiManager->cncWcsPanel();
        auto* jobp = m_uiManager->cncJobPanel();
        auto* ctp = m_uiManager->cncToolPanel();
        auto* safetyp = m_uiManager->cncSafetyPanel();

        // Set CncController on new panels
        if (jogp) jogp->setCncController(m_cncController.get());
        if (conp) conp->setCncController(m_cncController.get());
        if (wcsp) wcsp->setCncController(m_cncController.get());
        if (safetyp) safetyp->setCncController(m_cncController.get());

        CncCallbacks cncCb;
        cncCb.onConnectionChanged =
            [gcp, csp, jogp, conp, wcsp, jobp, safetyp](
                bool connected, const std::string& version) {
            gcp->onGrblConnected(connected, version);
            if (csp) csp->onConnectionChanged(connected, version);
            if (jogp) jogp->onConnectionChanged(connected, version);
            if (conp) conp->onConnectionChanged(connected, version);
            if (wcsp) wcsp->onConnectionChanged(connected, version);
            if (jobp && !connected) jobp->setStreaming(false);
            if (safetyp) safetyp->onConnectionChanged(connected, version);
            if (safetyp && !connected) {
                safetyp->setStreaming(false);
                safetyp->setProgram({});
            }
        };
        cncCb.onStatusUpdate = [gcp, csp, jogp, wcsp, jobp, ctp, safetyp](const MachineStatus& status) {
            gcp->onGrblStatus(status);
            if (csp) csp->onStatusUpdate(status);
            if (jogp) jogp->onStatusUpdate(status);
            if (wcsp) wcsp->onStatusUpdate(status);
            if (jobp) {
                jobp->onStatusUpdate(status);
                // Push recommended feed rate from calculator on each status update
                if (ctp && ctp->hasCalcResult())
                    jobp->setRecommendedFeedRate(ctp->getRecommendedFeedRate());
            }
            if (safetyp) safetyp->onStatusUpdate(status);
        };
        cncCb.onLineAcked = [gcp](const LineAck& ack) {
            gcp->onGrblLineAcked(ack);
        };
        cncCb.onProgressUpdate = [gcp, jobp, safetyp](const StreamProgress& progress) {
            gcp->onGrblProgress(progress);
            bool streaming = (progress.totalLines > 0 &&
                              progress.ackedLines < progress.totalLines);
            if (jobp) {
                jobp->onProgressUpdate(progress);
                jobp->setStreaming(streaming);
            }
            if (safetyp) {
                safetyp->setStreaming(streaming);
            }
        };
        cncCb.onAlarm = [gcp, conp](int code, const std::string& desc) {
            gcp->onGrblAlarm(code, desc);
            if (conp) conp->onAlarm(code, desc);
        };
        cncCb.onError = [gcp, conp](const std::string& message) {
            gcp->onGrblError(message);
            if (conp) conp->onError(message);
        };
        cncCb.onRawLine = [gcp, conp, wcsp](const std::string& line, bool isSent) {
            gcp->onGrblRawLine(line, isSent);
            if (conp) conp->onRawLine(line, isSent);
            if (wcsp) wcsp->onRawLine(line, isSent);
        };
        m_cncController->setCallbacks(cncCb);
    }
    if (auto* tbp = m_uiManager->toolBrowserPanel()) {
        tbp->setToolDatabase(m_toolDatabase.get());
        tbp->setMaterialManager(m_materialManager.get());
        tbp->setFileDialog(m_uiManager->fileDialog());
    }
    if (auto* ctp = m_uiManager->cncToolPanel()) {
        ctp->setToolDatabase(m_toolDatabase.get());
        ctp->setMaterialManager(m_materialManager.get());
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
        m_uiManager->propertiesPanel()->setOnGrainDirectionChanged([this](float degrees) {
            auto mesh = m_workspace->getFocusedMesh();
            if (!mesh)
                return;
            mesh->generatePlanarUVs(degrees);
            if (m_uiManager->viewportPanel())
                m_uiManager->viewportPanel()->setMesh(mesh);
        });
        m_uiManager->propertiesPanel()->setOnMaterialRemoved([this]() {
            if (m_materialManager && m_focusedModelId > 0)
                m_materialManager->clearMaterialAssignment(m_focusedModelId);
            m_activeMaterialTexture.reset();
            m_activeMaterialId = -1;
            if (m_uiManager->viewportPanel()) {
                m_uiManager->viewportPanel()->setMaterialTexture(nullptr);
            }
        });
    }

    // Wire MaterialsPanel callbacks
    if (m_uiManager->materialsPanel()) {
        m_uiManager->materialsPanel()->setOnMaterialAssigned(
            [this](int64_t materialId) { assignMaterialToCurrentModel(materialId); });

        m_uiManager->materialsPanel()->setOnGenerate([this](const std::string& prompt) {
            std::string apiKey = Config::instance().getGeminiApiKey();
            if (apiKey.empty()) {
                log::warning("Application",
                             "Gemini API key not set. Configure it in Settings > General.");
                ToastManager::instance().show(ToastType::Warning,
                                              "API Key Missing",
                                              "Set your Gemini API key in Settings.");
                if (m_uiManager->materialsPanel())
                    m_uiManager->materialsPanel()->setGenerating(false);
                return;
            }

            std::thread([this, prompt, apiKey]() {
                auto result = m_geminiService->generate(prompt, apiKey);
                m_mainThreadQueue->enqueue([this, result]() {
                    if (result.success) {
                        log::infof("Application",
                                   "AI generated material: %s",
                                   result.record.name.c_str());
                        ToastManager::instance().show(ToastType::Success,
                                                      "Material Generated",
                                                      "Review and save: " + result.record.name);
                        if (m_uiManager->materialsPanel()) {
                            m_uiManager->materialsPanel()->setGeneratedResult(result.record,
                                                                              result.dwmatPath);
                        }
                    } else {
                        log::errorf("Application",
                                    "Material generation failed: %s",
                                    result.error.c_str());
                        ToastManager::instance().show(ToastType::Error,
                                                      "Generation Failed",
                                                      result.error);
                        if (m_uiManager->materialsPanel())
                            m_uiManager->materialsPanel()->setGenerating(false);
                    }
                });
            }).detach();
        });
    }

    // Wire UIManager action callbacks (menu bar and keyboard shortcuts)
    auto hideStart = [this](bool show) {
        m_uiManager->showStartPage() = show;
    };
    m_uiManager->setOnNewProject([this, hideStart]() { m_fileIOManager->newProject(hideStart); });
    m_uiManager->setOnOpenProject([this, hideStart]() { m_fileIOManager->openProject(hideStart); });
    m_uiManager->setOnSaveProject([this]() { m_fileIOManager->saveProject(); });
    m_uiManager->setOnImportModel([this]() { m_fileIOManager->importModel(); });
    m_uiManager->setOnExportModel([this]() { m_fileIOManager->exportModel(); });
    m_uiManager->setOnImportProjectArchive(
        [this, hideStart]() { m_fileIOManager->importProjectArchive(hideStart); });
    m_uiManager->setOnQuit([this]() { quit(); });
    m_uiManager->setOnSpawnSettings([this]() { m_configManager->spawnSettingsApp(); });

    // Wire Tools menu
    m_uiManager->setOnLibraryMaintenance([this]() {
        if (m_uiManager->maintenanceDialog())
            m_uiManager->maintenanceDialog()->open();
    });
    if (m_uiManager->maintenanceDialog()) {
        m_uiManager->maintenanceDialog()->setOnRun([this]() -> MaintenanceReport {
            auto report = m_libraryManager->runMaintenance();
            if (m_uiManager->libraryPanel())
                m_uiManager->libraryPanel()->refresh();
            int total = report.categoriesSplit + report.categoriesRemoved + report.tagsDeduped +
                        report.thumbnailsCleared + report.ftsRebuilt;
            if (total > 0) {
                ToastManager::instance().show(ToastType::Success,
                                              "Maintenance Complete",
                                              std::to_string(total) + " issue(s) fixed");
            } else {
                ToastManager::instance().show(ToastType::Info,
                                              "Maintenance Complete",
                                              "No issues found");
            }
            return report;
        });
    }

    // Wire tagger shutdown dialog
    if (m_uiManager->taggerShutdownDialog()) {
        m_uiManager->taggerShutdownDialog()->setOnQuit([this]() { m_running = false; });
    }
}
void Application::onModelSelected(int64_t modelId) {
    if (!m_libraryManager) return;

    if (m_focusedModelId > 0 && m_uiManager->viewportPanel()) {
        auto camState = m_uiManager->viewportPanel()->getCameraState();
        ModelRepository repo(*m_database);
        repo.updateCameraState(m_focusedModelId, camState);
    }

    auto record = m_libraryManager->getModel(modelId);
    if (!record) return;
    m_focusedModelId = modelId;

    if (m_materialManager) {
        auto assignedMaterial = m_materialManager->getModelMaterial(modelId);
        if (assignedMaterial) {
            loadMaterialTextureForModel(modelId);
            if (m_uiManager->propertiesPanel())
                m_uiManager->propertiesPanel()->setMaterial(*assignedMaterial);
        } else {
            i64 defaultId = Config::instance().getDefaultMaterialId();
            if (defaultId > 0 && m_materialManager->getMaterial(defaultId)) {
                assignMaterialToCurrentModel(defaultId);
            } else {
                auto allMaterials = m_materialManager->getAllMaterials();
                if (!allMaterials.empty()) {
                    assignMaterialToCurrentModel(allMaterials.front().id);
                } else {
                    m_activeMaterialTexture.reset();
                    m_activeMaterialId = -1;
                    if (m_uiManager->propertiesPanel())
                        m_uiManager->propertiesPanel()->clearMaterial();
                }
            }
        }
    }

    uint64_t gen = ++m_loadingState.generation;
    m_loadingState.set(record->name);
    if (m_loadThread.joinable()) m_loadThread.join();

    Path filePath = PathResolver::resolve(record->filePath, PathCategory::Support);
    std::string name = record->name;
    auto storedOrientYaw = record->orientYaw;
    auto storedOrientMatrix = record->orientMatrix;
    auto storedCamera = record->cameraState;

    m_loadThread = std::thread(
        [this, filePath, name, gen, modelId, storedOrientYaw, storedOrientMatrix, storedCamera]() {
            auto loadResult = LoaderFactory::load(filePath);
            if (!loadResult) { m_loadingState.reset(); return; }
            loadResult.mesh->setName(name);
            f32 orientYaw = 0.0f;
            if (Config::instance().getAutoOrient()) {
                if (storedOrientYaw && storedOrientMatrix) {
                    loadResult.mesh->applyStoredOrient(*storedOrientMatrix);
                    orientYaw = *storedOrientYaw;
                } else {
                    orientYaw = loadResult.mesh->autoOrient();
                    ScopedConnection conn(*m_connectionPool);
                    ModelRepository repo(*conn);
                    repo.updateOrient(modelId, orientYaw, loadResult.mesh->getOrientMatrix());
                }
            }
            auto mesh = loadResult.mesh;
            m_mainThreadQueue->enqueue([this, mesh, name, gen, orientYaw, storedCamera]() {
                if (gen != m_loadingState.generation.load()) return;
                m_loadingState.reset();
                m_workspace->setFocusedMesh(mesh);
                if (m_uiManager->viewportPanel())
                    m_uiManager->viewportPanel()->setPreOrientedMesh(mesh, orientYaw, storedCamera);
                if (m_uiManager->propertiesPanel())
                    m_uiManager->propertiesPanel()->setMesh(mesh, name);
                if (m_uiManager->materialsPanel())
                    m_uiManager->materialsPanel()->setModelLoaded(true);
            });
        });
}

void Application::assignMaterialToCurrentModel(int64_t materialId) {
    if (!m_materialManager || !m_workspace)
        return;
    auto mesh = m_workspace->getFocusedMesh();
    if (!mesh)
        return;
    auto material = m_materialManager->getMaterial(materialId);
    if (!material)
        return;
    if (m_focusedModelId > 0)
        m_materialManager->assignMaterialToModel(materialId, m_focusedModelId);

    // Load and upload material texture if archive path exists
    m_activeMaterialTexture.reset();
    if (!material->archivePath.empty()) {
        Path archPath = PathResolver::resolve(material->archivePath, PathCategory::Materials);
        auto matData = MaterialArchive::load(archPath.string());
        if (matData && !matData->textureData.empty()) {
            // Decode texture PNG from memory
            auto textureOpt = TextureLoader::loadPNGFromMemory(matData->textureData.data(),
                                                               matData->textureData.size());
            if (textureOpt) {
                m_activeMaterialTexture = std::make_unique<Texture>();
                m_activeMaterialTexture->upload(textureOpt->pixels.data(),
                                                textureOpt->width,
                                                textureOpt->height);
            }
        }
    }

    if (mesh->needsUVGeneration())
        mesh->generatePlanarUVs(material->grainDirectionDeg);

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
        Path archPath = PathResolver::resolve(material->archivePath, PathCategory::Materials);
        auto matData = MaterialArchive::load(archPath.string());
        if (matData && !matData->textureData.empty()) {
            auto textureOpt = TextureLoader::loadPNGFromMemory(matData->textureData.data(),
                                                               matData->textureData.size());
            if (textureOpt) {
                m_activeMaterialTexture = std::make_unique<Texture>();
                m_activeMaterialTexture->upload(textureOpt->pixels.data(),
                                                textureOpt->width,
                                                textureOpt->height);
            }
        }
    }

    if (m_uiManager && m_uiManager->viewportPanel())
        m_uiManager->viewportPanel()->setMaterialTexture(m_activeMaterialTexture.get());
}

bool Application::generateMaterialThumbnail(int64_t modelId, Mesh& mesh) {
    if (Config::instance().getAutoOrient()) {
        auto record = m_libraryManager->getModel(modelId);
        if (record && record->orientYaw && record->orientMatrix)
            mesh.applyStoredOrient(*record->orientMatrix);
        else
            static_cast<void>(mesh.autoOrient());
    }

    std::unique_ptr<Texture> tex;
    i64 matId = Config::instance().getDefaultMaterialId();
    std::optional<MaterialRecord> mat;
    if (matId > 0 && m_materialManager)
        mat = m_materialManager->getMaterial(matId);
    if (!mat && m_materialManager) {
        auto all = m_materialManager->getAllMaterials();
        if (!all.empty()) mat = all.front();
    }
    if (mat) {
        if (!mat->archivePath.empty()) {
            auto matData = MaterialArchive::load(mat->archivePath.string());
            if (matData && !matData->textureData.empty()) {
                auto decoded = TextureLoader::loadPNGFromMemory(matData->textureData.data(),
                                                                matData->textureData.size());
                if (decoded) {
                    tex = std::make_unique<Texture>();
                    tex->upload(decoded->pixels.data(), decoded->width, decoded->height);
                }
            }
        }
        if (mesh.needsUVGeneration())
            mesh.generatePlanarUVs(mat->grainDirectionDeg);
    }
    return m_libraryManager->generateThumbnail(modelId, mesh, tex.get(), 0.0f, 0.0f);
}

} // namespace dw
