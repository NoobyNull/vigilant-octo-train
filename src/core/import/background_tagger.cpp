#include "background_tagger.h"

#include <cstring>

#include "../database/connection_pool.h"
#include "../database/model_repository.h"
#include "../library/library_manager.h"
#include "../materials/gemini_descriptor_service.h"
#include "../utils/log.h"

namespace dw {

BackgroundTagger::BackgroundTagger(ConnectionPool& pool,
                                   LibraryManager* libraryMgr,
                                   GeminiDescriptorService* descriptorSvc)
    : m_pool(pool), m_libraryMgr(libraryMgr), m_descriptorSvc(descriptorSvc) {}

BackgroundTagger::~BackgroundTagger() {
    stop();
    join();
}

void BackgroundTagger::start(const std::string& apiKey) {
    if (m_progress.active.load())
        return;

    // Join any previous thread
    join();

    m_apiKey = apiKey;
    m_stopRequested.store(false);
    m_progress.totalUntagged.store(0);
    m_progress.completed.store(0);
    m_progress.failed.store(0);
    m_progress.active.store(true);
    {
        std::lock_guard<std::mutex> lock(m_progress.nameMutex);
        m_progress.currentModel[0] = '\0';
    }

    m_thread = std::thread([this]() { workerLoop(); });
}

void BackgroundTagger::stop() {
    m_stopRequested.store(true);
}

void BackgroundTagger::join() {
    if (m_thread.joinable())
        m_thread.join();
}

bool BackgroundTagger::isActive() const {
    return m_progress.active.load();
}

const TaggerProgress& BackgroundTagger::progress() const {
    return m_progress;
}

void BackgroundTagger::workerLoop() {
    ScopedConnection conn(m_pool);
    ModelRepository repo(*conn);

    // Count total untagged
    int total = repo.countByTagStatus(0);
    m_progress.totalUntagged.store(total);
    log::infof("Tagger", "Starting background tagging: %d untagged models", total);

    while (!m_stopRequested.load()) {
        auto model = repo.findNextUntagged();
        if (!model) {
            log::info("Tagger", "No more untagged models");
            break;
        }

        // Update current model name for UI
        {
            std::lock_guard<std::mutex> lock(m_progress.nameMutex);
            std::strncpy(m_progress.currentModel,
                         model->name.c_str(),
                         sizeof(m_progress.currentModel) - 1);
            m_progress.currentModel[sizeof(m_progress.currentModel) - 1] = '\0';
        }

        // Mark in-progress
        repo.updateTagStatus(model->id, 1);

        // Check stop before expensive API call
        if (m_stopRequested.load()) {
            repo.updateTagStatus(model->id, 0); // reset
            break;
        }

        // Call Gemini API (blocking)
        auto result = m_descriptorSvc->describe(model->thumbnailPath.string(), m_apiKey);

        // Check stop after API call
        if (m_stopRequested.load()) {
            repo.updateTagStatus(model->id, 0); // reset
            break;
        }

        if (result.success) {
            // Persist descriptor
            m_libraryMgr->updateDescriptor(
                model->id, result.title, result.description, result.hoverNarrative);

            // Merge keywords + associations into tags
            auto existing = m_libraryMgr->getModel(model->id);
            if (existing) {
                auto tags = existing->tags;
                for (const auto& kw : result.keywords)
                    tags.push_back(kw);
                for (const auto& assoc : result.associations)
                    tags.push_back(assoc);
                m_libraryMgr->updateTags(model->id, tags);
            }

            // Assign categories
            if (!result.categories.empty())
                m_libraryMgr->resolveAndAssignCategories(model->id, result.categories);

            repo.updateTagStatus(model->id, 2); // tagged
            m_progress.completed.fetch_add(1);
            log::infof("Tagger", "Tagged '%s' as: %s", model->name.c_str(), result.title.c_str());
        } else {
            repo.updateTagStatus(model->id, 3); // failed
            m_progress.failed.fetch_add(1);
            log::warningf("Tagger",
                          "Failed to tag '%s': %s",
                          model->name.c_str(),
                          result.error.c_str());
        }
    }

    m_progress.active.store(false);
    log::infof("Tagger",
               "Background tagging finished: %d tagged, %d failed",
               m_progress.completed.load(),
               m_progress.failed.load());
}

} // namespace dw
