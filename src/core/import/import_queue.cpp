#include "import_queue.h"

#include "../loaders/loader_factory.h"
#include "../mesh/hash.h"
#include "../utils/file_utils.h"
#include "../utils/log.h"

#include <cstring>

namespace dw {

ImportQueue::ImportQueue(Database& db)
    : m_db(db)
    , m_modelRepo(db) {}

ImportQueue::~ImportQueue() {
    m_shutdown.store(true);
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void ImportQueue::enqueue(const std::vector<Path>& paths) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& path : paths) {
        ImportTask task;
        task.sourcePath = path;
        task.extension = file::getExtension(path);
        m_pending.push_back(std::move(task));
    }

    m_progress.totalFiles.fetch_add(static_cast<int>(paths.size()));
    m_progress.active.store(true);
    m_cancelRequested.store(false);

    // Start worker if not already running
    if (m_worker.joinable()) {
        m_worker.join();  // Previous batch finished, clean up
    }
    m_worker = std::thread(&ImportQueue::workerLoop, this);
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

// --- Background thread ---

void ImportQueue::workerLoop() {
    log::info("Import", "Worker thread started");

    while (!m_shutdown.load()) {
        ImportTask task;

        // Grab next pending task
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_pending.empty()) {
                break;  // No more work
            }
            task = std::move(m_pending.front());
            m_pending.erase(m_pending.begin());
        }

        // Check for cancellation between tasks
        if (m_cancelRequested.load()) {
            task.stage = ImportStage::Failed;
            task.error = "Cancelled";
            m_progress.failedFiles.fetch_add(1);
            m_progress.completedFiles.fetch_add(1);
            continue;
        }

        // Update progress with current file name
        auto filename = file::getStem(task.sourcePath);
        std::strncpy(m_progress.currentFileName, filename.c_str(),
                      sizeof(m_progress.currentFileName) - 1);
        m_progress.currentFileName[sizeof(m_progress.currentFileName) - 1] = '\0';

        processTask(task);

        bool failed = (task.stage == ImportStage::Failed);

        // Move completed task to appropriate place
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (task.stage == ImportStage::WaitingForThumbnail) {
                m_completed.push_back(std::move(task));
            }
        }

        if (failed) {
            m_progress.failedFiles.fetch_add(1);
        }
        m_progress.completedFiles.fetch_add(1);
    }

    // Drain remaining tasks if cancelled
    if (m_cancelRequested.load()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int remaining = static_cast<int>(m_pending.size());
        m_progress.failedFiles.fetch_add(remaining);
        m_progress.completedFiles.fetch_add(remaining);
        m_pending.clear();
    }

    m_progress.active.store(false);
    log::infof("Import", "Worker finished (%d/%d completed, %d failed)",
               m_progress.completedFiles.load(),
               m_progress.totalFiles.load(),
               m_progress.failedFiles.load());
}

void ImportQueue::processTask(ImportTask& task) {
    // Stage 1: Reading
    task.stage = ImportStage::Reading;
    m_progress.currentStage.store(ImportStage::Reading);

    auto fileData = file::readBinary(task.sourcePath);
    if (!fileData) {
        task.stage = ImportStage::Failed;
        task.error = "Failed to read file: " + task.sourcePath.string();
        log::errorf("Import", "%s", task.error.c_str());
        return;
    }
    task.fileData = std::move(*fileData);

    // Stage 2: Hashing
    task.stage = ImportStage::Hashing;
    m_progress.currentStage.store(ImportStage::Hashing);

    task.fileHash = hash::computeBuffer(task.fileData);

    // Stage 3: Check duplicate
    task.stage = ImportStage::CheckingDuplicate;
    m_progress.currentStage.store(ImportStage::CheckingDuplicate);

    auto existing = m_modelRepo.findByHash(task.fileHash);
    if (existing) {
        task.isDuplicate = true;
        task.stage = ImportStage::Failed;
        task.error = "Duplicate of existing model: " + existing->name;
        log::warningf("Import", "Skipping duplicate '%s'",
                      task.sourcePath.filename().string().c_str());
        return;
    }

    // Stage 4: Parsing
    task.stage = ImportStage::Parsing;
    m_progress.currentStage.store(ImportStage::Parsing);

    auto loadResult = LoaderFactory::loadFromBuffer(task.fileData, task.extension);
    if (!loadResult) {
        task.stage = ImportStage::Failed;
        task.error = "Parse failed: " + loadResult.error;
        log::errorf("Import", "%s", task.error.c_str());
        return;
    }
    task.mesh = loadResult.mesh;

    // Capture file size before releasing buffer
    u64 actualFileSize = static_cast<u64>(task.fileData.size());

    // Release file data memory now that parsing is done
    task.fileData.clear();
    task.fileData.shrink_to_fit();

    // Stage 5: Inserting
    task.stage = ImportStage::Inserting;
    m_progress.currentStage.store(ImportStage::Inserting);

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

    auto modelId = m_modelRepo.insert(record);
    if (!modelId) {
        task.stage = ImportStage::Failed;
        task.error = "Failed to insert into database";
        log::errorf("Import", "%s for '%s'", task.error.c_str(),
                    task.sourcePath.filename().string().c_str());
        return;
    }

    task.modelId = *modelId;
    task.record = record;
    task.record.id = *modelId;
    task.stage = ImportStage::WaitingForThumbnail;

    log::infof("Import", "'%s' ready for thumbnail (id=%lld)",
               record.name.c_str(), static_cast<long long>(*modelId));
}

}  // namespace dw
