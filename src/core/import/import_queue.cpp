#include "import_queue.h"

#include "../config/config.h"
#include "../database/connection_pool.h"
#include "../database/gcode_repository.h"
#include "../database/model_repository.h"
#include "../import/file_handler.h"
#include "../library/library_manager.h"
#include "../loaders/gcode_loader.h"
#include "../loaders/loader_factory.h"
#include "../mesh/hash.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

ImportQueue::ImportQueue(ConnectionPool& pool, LibraryManager* libraryManager)
    : m_pool(pool), m_libraryManager(libraryManager) {}

ImportQueue::~ImportQueue() {
    m_shutdown.store(true);
    if (m_threadPool) {
        m_threadPool->shutdown();
    }
}

void ImportQueue::enqueue(const std::vector<Path>& paths) {
    if (paths.empty()) {
        return;
    }

    // Reset batch summary
    {
        std::lock_guard<std::mutex> lock(m_summaryMutex);
        m_batchSummary = ImportBatchSummary{};
        m_batchSummary.totalFiles = static_cast<int>(paths.size());
    }

    // Get thread count from config
    auto tier = Config::instance().getParallelismTier();
    size_t threadCount = calculateThreadCount(tier);

    // Lazy-create or recreate ThreadPool with current thread count
    if (!m_threadPool || m_threadPool->isIdle()) {
        m_threadPool = std::make_unique<ThreadPool>(threadCount);
    }

    // Warn if thread count exceeds connection pool size
    // Note: ConnectionPool size is fixed at construction, so we can't resize it here
    // This is a wiring concern for Application to ensure pool has adequate size
    log::infof("Import", "Starting batch import: %zu files, %zu workers", paths.size(),
               threadCount);

    // Reset progress
    m_progress.reset();
    m_progress.totalFiles.store(static_cast<int>(paths.size()));
    m_progress.active.store(true);
    m_cancelRequested.store(false);

    // Set remaining task counter for batch completion detection
    m_remainingTasks.store(static_cast<int>(paths.size()));

    // Enqueue each file as a task
    for (const auto& path : paths) {
        ImportTask task;
        task.sourcePath = path;
        task.extension = file::getExtension(path);
        task.importType = importTypeFromExtension(task.extension);

        // Enqueue to thread pool - capture task by value (moved into lambda)
        m_threadPool->enqueue(
            [this, task = std::move(task)]() mutable { processTask(std::move(task)); });
    }
}

void ImportQueue::enqueue(const Path& path) {
    enqueue(std::vector<Path>{path});
}

void ImportQueue::cancel() {
    m_cancelRequested.store(true);
    log::info("Import", "Cancelled by user");
}

bool ImportQueue::isActive() const {
    return m_progress.active.load();
}

const ImportProgress& ImportQueue::progress() const {
    return m_progress;
}

std::vector<ImportTask> ImportQueue::pollCompleted() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ImportTask> result = std::move(m_completed);
    m_completed.clear();
    return result;
}

void ImportQueue::taskFinished(i64 modelId) {
    // Find and invoke callback if set
    if (m_onComplete) {
        // Create a minimal task with the modelId for the callback
        ImportTask task;
        task.modelId = modelId;
        task.stage = ImportStage::Done;
        m_onComplete(task);
    }
}

void ImportQueue::setOnComplete(ImportCallback callback) {
    m_onComplete = std::move(callback);
}

const ImportBatchSummary& ImportQueue::batchSummary() const {
    std::lock_guard<std::mutex> lock(m_summaryMutex);
    return m_batchSummary;
}

void ImportQueue::setOnBatchComplete(SummaryCallback callback) {
    m_onBatchComplete = std::move(callback);
}

// --- Worker task processing (runs on ThreadPool worker) ---

void ImportQueue::processTask(ImportTask task) {
    // Check for cancellation at start
    if (m_cancelRequested.load()) {
        task.stage = ImportStage::Failed;
        task.error = "Cancelled";

        // Update summary
        {
            std::lock_guard<std::mutex> lock(m_summaryMutex);
            m_batchSummary.failedCount++;
            m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
        }

        m_progress.failedFiles.fetch_add(1);
        m_progress.completedFiles.fetch_add(1);

        // Check if this was the last task
        if (m_remainingTasks.fetch_sub(1) == 1) {
            m_progress.active.store(false);
            if (m_onBatchComplete) {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_onBatchComplete(m_batchSummary);
            }
        }
        return;
    }

    // Acquire a pooled connection for this task
    ScopedConnection conn(m_pool);

    // Create repositories based on import type
    ModelRepository modelRepo(*conn);
    GCodeRepository gcodeRepo(*conn);

    // Update progress with current file name (thread-safe)
    m_progress.setCurrentFileName(file::getStem(task.sourcePath));

    // Stage 1: Reading
    task.stage = ImportStage::Reading;
    m_progress.currentStage.store(ImportStage::Reading);

    auto fileData = file::readBinary(task.sourcePath);
    if (!fileData) {
        task.stage = ImportStage::Failed;
        task.error = "Failed to read file: " + task.sourcePath.string();
        log::errorf("Import", "%s", task.error.c_str());

        // Update summary
        {
            std::lock_guard<std::mutex> lock(m_summaryMutex);
            m_batchSummary.failedCount++;
            m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
        }

        m_progress.failedFiles.fetch_add(1);
        m_progress.completedFiles.fetch_add(1);

        // Check if this was the last task
        if (m_remainingTasks.fetch_sub(1) == 1) {
            m_progress.active.store(false);
            if (m_onBatchComplete) {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_onBatchComplete(m_batchSummary);
            }
        }
        return;
    }
    task.fileData = std::move(*fileData);

    // Stage 2: Hashing
    task.stage = ImportStage::Hashing;
    m_progress.currentStage.store(ImportStage::Hashing);

    task.fileHash = hash::computeBuffer(task.fileData);

    // Stage 3: Check duplicate (use appropriate repository)
    task.stage = ImportStage::CheckingDuplicate;
    m_progress.currentStage.store(ImportStage::CheckingDuplicate);

    bool isDuplicate = false;
    std::string duplicateName;

    if (task.importType == ImportType::GCode) {
        auto existing = gcodeRepo.findByHash(task.fileHash);
        if (existing) {
            isDuplicate = true;
            duplicateName = existing->name;
        }
    } else {
        auto existing = modelRepo.findByHash(task.fileHash);
        if (existing) {
            isDuplicate = true;
            duplicateName = existing->name;
        }
    }

    if (isDuplicate) {
        task.isDuplicate = true;
        task.stage = ImportStage::Failed;
        task.error = "Duplicate of existing file: " + duplicateName;

        // Update summary
        {
            std::lock_guard<std::mutex> lock(m_summaryMutex);
            m_batchSummary.duplicateCount++;
            m_batchSummary.duplicateNames.push_back(file::getStem(task.sourcePath));
        }

        log::warningf("Import", "Skipping duplicate '%s'",
                      task.sourcePath.filename().string().c_str());

        // Count duplicates as both completed and failed (for backward compatibility with tests)
        m_progress.failedFiles.fetch_add(1);
        m_progress.completedFiles.fetch_add(1);

        // Check if this was the last task
        if (m_remainingTasks.fetch_sub(1) == 1) {
            m_progress.active.store(false);
            if (m_onBatchComplete) {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_onBatchComplete(m_batchSummary);
            }
        }
        return;
    }

    // Stage 4: Parsing (type-specific)
    task.stage = ImportStage::Parsing;
    m_progress.currentStage.store(ImportStage::Parsing);

    if (task.importType == ImportType::GCode) {
        // Manually create GCodeLoader for G-code files
        GCodeLoader gcodeLoader;
        auto result = gcodeLoader.loadFromBuffer(task.fileData);
        if (!result) {
            task.stage = ImportStage::Failed;
            task.error = "Parse failed: " + result.error;
            log::errorf("Import", "%s", task.error.c_str());

            // Update summary
            {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_batchSummary.failedCount++;
                m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
            }

            m_progress.failedFiles.fetch_add(1);
            m_progress.completedFiles.fetch_add(1);

            // Check if this was the last task
            if (m_remainingTasks.fetch_sub(1) == 1) {
                m_progress.active.store(false);
                if (m_onBatchComplete) {
                    std::lock_guard<std::mutex> lock(m_summaryMutex);
                    m_onBatchComplete(m_batchSummary);
                }
            }
            return;
        }

        task.mesh = result.mesh;

        // Extract and store metadata (heap-allocate to avoid header dependencies)
        task.gcodeMetadata = new GCodeMetadata(gcodeLoader.lastMetadata());

    } else {
        // Existing mesh loading via LoaderFactory
        auto loadResult = LoaderFactory::loadFromBuffer(task.fileData, task.extension);
        if (!loadResult) {
            task.stage = ImportStage::Failed;
            task.error = "Parse failed: " + loadResult.error;
            log::errorf("Import", "%s", task.error.c_str());

            // Update summary
            {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_batchSummary.failedCount++;
                m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
            }

            m_progress.failedFiles.fetch_add(1);
            m_progress.completedFiles.fetch_add(1);

            // Check if this was the last task
            if (m_remainingTasks.fetch_sub(1) == 1) {
                m_progress.active.store(false);
                if (m_onBatchComplete) {
                    std::lock_guard<std::mutex> lock(m_summaryMutex);
                    m_onBatchComplete(m_batchSummary);
                }
            }
            return;
        }
        task.mesh = loadResult.mesh;
    }

    // Capture file size before releasing buffer
    u64 actualFileSize = static_cast<u64>(task.fileData.size());

    // Release file data memory now that parsing is done
    task.fileData.clear();
    task.fileData.shrink_to_fit();

    // Stage 5: Inserting (type-specific)
    task.stage = ImportStage::Inserting;
    m_progress.currentStage.store(ImportStage::Inserting);

    Path finalPath = task.sourcePath;

    if (task.importType == ImportType::GCode) {
        // Insert G-code record
        GCodeRecord record;
        record.hash = task.fileHash;
        record.name = file::getStem(task.sourcePath);
        record.filePath = task.sourcePath;
        record.fileSize = actualFileSize;

        // Populate metadata from extracted data
        if (task.gcodeMetadata) {
            record.boundsMin = task.gcodeMetadata->boundsMin;
            record.boundsMax = task.gcodeMetadata->boundsMax;
            record.totalDistance = task.gcodeMetadata->totalDistance;
            record.estimatedTime = task.gcodeMetadata->estimatedTime;
            record.feedRates = task.gcodeMetadata->feedRates;
            record.toolNumbers = task.gcodeMetadata->toolNumbers;
        }

        auto gcodeId = gcodeRepo.insert(record);
        if (!gcodeId) {
            task.stage = ImportStage::Failed;
            task.error = "Failed to insert into database";
            log::errorf("Import", "%s for '%s'", task.error.c_str(),
                        task.sourcePath.filename().string().c_str());

            // Cleanup
            if (task.gcodeMetadata) {
                delete task.gcodeMetadata;
                task.gcodeMetadata = nullptr;
            }

            // Update summary
            {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_batchSummary.failedCount++;
                m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
            }

            m_progress.failedFiles.fetch_add(1);
            m_progress.completedFiles.fetch_add(1);

            // Check if this was the last task
            if (m_remainingTasks.fetch_sub(1) == 1) {
                m_progress.active.store(false);
                if (m_onBatchComplete) {
                    std::lock_guard<std::mutex> lock(m_summaryMutex);
                    m_onBatchComplete(m_batchSummary);
                }
            }
            return;
        }

        task.gcodeId = *gcodeId;

        log::infof("Import", "G-code '%s' inserted (id=%lld)", record.name.c_str(),
                   static_cast<long long>(*gcodeId));

        // Auto-detect model association if LibraryManager available
        if (m_libraryManager) {
            auto matchedModelId = m_libraryManager->autoDetectModelMatch(record.name);
            if (matchedModelId) {
                // Confident single match found - auto-associate
                auto modelRecord = modelRepo.findById(*matchedModelId);
                if (modelRecord) {
                    // Check if model already has operation groups
                    auto groups = m_libraryManager->getOperationGroups(*matchedModelId);
                    i64 groupId = 0;

                    if (groups.empty()) {
                        // No groups yet - create default "Imported" group
                        auto newGroupId =
                            m_libraryManager->createOperationGroup(*matchedModelId, "Imported", 0);
                        if (newGroupId) {
                            groupId = *newGroupId;
                            log::infof("Import", "Created 'Imported' group for model '%s'",
                                       modelRecord->name.c_str());
                        }
                    } else {
                        // Use first existing group
                        groupId = groups[0].id;
                    }

                    if (groupId > 0) {
                        // Add G-code to the group
                        if (m_libraryManager->addGCodeToGroup(groupId, *gcodeId, 0)) {
                            log::infof("Import", "Auto-associated '%s' with model '%s'",
                                       record.name.c_str(), modelRecord->name.c_str());
                        }
                    }
                }
            } else {
                // No match or ambiguous - standalone import
                log::infof("Import", "No model match for '%s', imported as standalone",
                           record.name.c_str());
            }
        }

    } else {
        // Precompute autoOrient on the worker thread (pure CPU, no GL)
        // Results are stored in DB so model loads skip recomputation
        if (Config::instance().getAutoOrient() && task.mesh && task.mesh->isValid()) {
            f32 orientYaw = task.mesh->autoOrient();
            task.record.orientYaw = orientYaw;
            task.record.orientMatrix = task.mesh->getOrientMatrix();
        }

        // Insert mesh model record
        ModelRecord record;
        record.hash = task.fileHash;
        record.name = file::getStem(task.sourcePath);
        record.filePath = task.sourcePath;
        record.fileFormat = task.extension;
        record.fileSize = actualFileSize;
        record.vertexCount = task.mesh->vertexCount();
        record.triangleCount = task.mesh->triangleCount();

        const auto& bounds = task.mesh->bounds();
        record.boundsMin = bounds.min;
        record.boundsMax = bounds.max;

        // Carry orient data to the DB record
        record.orientYaw = task.record.orientYaw;
        record.orientMatrix = task.record.orientMatrix;

        auto modelId = modelRepo.insert(record);
        if (!modelId) {
            task.stage = ImportStage::Failed;
            task.error = "Failed to insert into database";
            log::errorf("Import", "%s for '%s'", task.error.c_str(),
                        task.sourcePath.filename().string().c_str());

            // Update summary
            {
                std::lock_guard<std::mutex> lock(m_summaryMutex);
                m_batchSummary.failedCount++;
                m_batchSummary.errors.push_back({file::getStem(task.sourcePath), task.error});
            }

            m_progress.failedFiles.fetch_add(1);
            m_progress.completedFiles.fetch_add(1);

            // Check if this was the last task
            if (m_remainingTasks.fetch_sub(1) == 1) {
                m_progress.active.store(false);
                if (m_onBatchComplete) {
                    std::lock_guard<std::mutex> lock(m_summaryMutex);
                    m_onBatchComplete(m_batchSummary);
                }
            }
            return;
        }

        task.modelId = *modelId;
        task.record = record;
        task.record.id = *modelId;

        log::infof("Import", "Mesh '%s' inserted (id=%lld)", record.name.c_str(),
                   static_cast<long long>(*modelId));
    }

    // Stage 5.5: File Handling (apply copy/move/leave mode)
    auto mode = Config::instance().getFileHandlingMode();
    auto libraryDir = Config::instance().getLibraryDir();
    if (libraryDir.empty()) {
        libraryDir = FileHandler::defaultLibraryDir();
    }

    if (mode != FileHandlingMode::LeaveInPlace) {
        std::string error;
        Path handledPath =
            FileHandler::handleImportedFile(task.sourcePath, mode, libraryDir, error);

        if (handledPath.empty()) {
            // File handling failed - log warning but don't fail the import
            log::warningf("Import", "File handling failed for '%s': %s",
                          file::getStem(task.sourcePath).c_str(), error.c_str());
        } else {
            // Update the record's file path in DB
            finalPath = handledPath;

            if (task.importType == ImportType::GCode) {
                auto record = gcodeRepo.findById(task.gcodeId);
                if (record) {
                    record->filePath = finalPath;
                    gcodeRepo.update(*record);
                }
            } else {
                task.record.filePath = finalPath;
                modelRepo.update(task.record);
            }

            log::infof("Import", "File handled: %s -> %s",
                       task.sourcePath.filename().string().c_str(),
                       finalPath.filename().string().c_str());
        }
    }

    // Stage 6: WaitingForThumbnail (both mesh and G-code need thumbnails)
    task.stage = ImportStage::WaitingForThumbnail;

    // Move completed task to queue for main thread processing
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_completed.push_back(std::move(task));
    }

    m_progress.completedFiles.fetch_add(1);

    // Update batch summary
    {
        std::lock_guard<std::mutex> lock(m_summaryMutex);
        m_batchSummary.successCount++;
    }

    // Check if this was the last task - if so, fire batch complete callback
    if (m_remainingTasks.fetch_sub(1) == 1) {
        m_progress.active.store(false);
        log::infof("Import", "Batch complete: %d successful, %d failed, %d duplicates",
                   m_batchSummary.successCount, m_batchSummary.failedCount,
                   m_batchSummary.duplicateCount);

        if (m_onBatchComplete) {
            std::lock_guard<std::mutex> lock(m_summaryMutex);
            m_onBatchComplete(m_batchSummary);
        }
    }
}

} // namespace dw
