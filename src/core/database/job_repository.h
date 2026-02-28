#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../gcode/gcode_modal_scanner.h"
#include "../types.h"
#include "database.h"

namespace dw {

// CNC job execution record
struct JobRecord {
    i64 id = 0;
    std::string fileName;
    std::string filePath;
    int totalLines = 0;
    int lastAckedLine = 0;
    std::string status = "running"; // running, completed, aborted, interrupted
    int errorCount = 0;
    f32 elapsedSeconds = 0.0f;
    ModalState modalState;
    std::string startedAt;
    std::string endedAt;
};

// Repository for CNC job history CRUD operations
class JobRepository {
  public:
    explicit JobRepository(Database& db);

    // Create a new job record (returns id)
    std::optional<i64> insert(const JobRecord& record);

    // Update progress during streaming (called periodically)
    bool updateProgress(i64 id, int lastAckedLine, f32 elapsedSeconds);

    // Finalize a job with status, modal state snapshot, and final stats
    bool finishJob(i64 id,
                   const std::string& status,
                   int lastAckedLine,
                   f32 elapsedSeconds,
                   int errorCount,
                   const ModalState& modalState);

    // Query jobs
    std::vector<JobRecord> findRecent(int limit = 50);
    std::optional<JobRecord> findById(i64 id);
    std::vector<JobRecord> findByStatus(const std::string& status);

    // Delete
    bool remove(i64 id);
    bool clearAll();

  private:
    JobRecord rowToJob(Statement& stmt);

    Database& m_db;
};

} // namespace dw
