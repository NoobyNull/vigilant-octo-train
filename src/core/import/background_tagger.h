#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace dw {

class ConnectionPool;
class LibraryManager;
class GeminiDescriptorService;

struct TaggerProgress {
    std::atomic<int> totalUntagged{0};
    std::atomic<int> completed{0};
    std::atomic<int> failed{0};
    std::atomic<bool> active{false};
    std::mutex nameMutex;
    char currentModel[256]{};
};

class BackgroundTagger {
  public:
    BackgroundTagger(ConnectionPool& pool,
                     LibraryManager* libraryMgr,
                     GeminiDescriptorService* descriptorSvc);
    ~BackgroundTagger();

    void start(const std::string& apiKey);
    void stop();
    void join();
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] const TaggerProgress& progress() const;

  private:
    void workerLoop();

    ConnectionPool& m_pool;
    LibraryManager* m_libraryMgr;
    GeminiDescriptorService* m_descriptorSvc;

    std::thread m_thread;
    std::atomic<bool> m_stopRequested{false};
    std::string m_apiKey;
    TaggerProgress m_progress;
};

} // namespace dw
