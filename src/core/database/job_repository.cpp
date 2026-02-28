#include "job_repository.h"

#include "../utils/log.h"

namespace dw {

JobRepository::JobRepository(Database& db) : m_db(db) {}

std::optional<i64> JobRepository::insert(const JobRecord& record) {
    auto stmt = m_db.prepare(R"(
        INSERT INTO cnc_jobs (
            file_name, file_path, total_lines, last_acked_line, status,
            error_count, elapsed_seconds,
            modal_distance_mode, modal_coordinate_system, modal_units,
            modal_spindle_state, modal_coolant_state,
            modal_feed_rate, modal_spindle_speed
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    if (!stmt.isValid())
        return std::nullopt;

    if (!stmt.bindText(1, record.fileName) || !stmt.bindText(2, record.filePath) ||
        !stmt.bindInt(3, record.totalLines) || !stmt.bindInt(4, record.lastAckedLine) ||
        !stmt.bindText(5, record.status) || !stmt.bindInt(6, record.errorCount) ||
        !stmt.bindDouble(7, static_cast<f64>(record.elapsedSeconds)) ||
        !stmt.bindText(8, record.modalState.distanceMode) ||
        !stmt.bindText(9, record.modalState.coordinateSystem) ||
        !stmt.bindText(10, record.modalState.units) ||
        !stmt.bindText(11, record.modalState.spindleState) ||
        !stmt.bindText(12, record.modalState.coolantState) ||
        !stmt.bindDouble(13, static_cast<f64>(record.modalState.feedRate)) ||
        !stmt.bindDouble(14, static_cast<f64>(record.modalState.spindleSpeed))) {
        log::error("JobRepo", "Failed to bind insert parameters");
        return std::nullopt;
    }

    if (!stmt.execute()) {
        log::errorf("JobRepo", "Failed to insert job: %s", m_db.lastError().c_str());
        return std::nullopt;
    }

    return m_db.lastInsertId();
}

bool JobRepository::updateProgress(i64 id, int lastAckedLine, f32 elapsedSeconds) {
    auto stmt = m_db.prepare(
        "UPDATE cnc_jobs SET last_acked_line = ?, elapsed_seconds = ? WHERE id = ?");

    if (!stmt.isValid())
        return false;

    if (!stmt.bindInt(1, lastAckedLine) ||
        !stmt.bindDouble(2, static_cast<f64>(elapsedSeconds)) || !stmt.bindInt(3, id))
        return false;

    return stmt.execute();
}

bool JobRepository::finishJob(i64 id,
                              const std::string& status,
                              int lastAckedLine,
                              f32 elapsedSeconds,
                              int errorCount,
                              const ModalState& modalState) {
    auto stmt = m_db.prepare(R"(
        UPDATE cnc_jobs SET
            status = ?, last_acked_line = ?, elapsed_seconds = ?, error_count = ?,
            modal_distance_mode = ?, modal_coordinate_system = ?, modal_units = ?,
            modal_spindle_state = ?, modal_coolant_state = ?,
            modal_feed_rate = ?, modal_spindle_speed = ?,
            ended_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )");

    if (!stmt.isValid())
        return false;

    if (!stmt.bindText(1, status) || !stmt.bindInt(2, lastAckedLine) ||
        !stmt.bindDouble(3, static_cast<f64>(elapsedSeconds)) ||
        !stmt.bindInt(4, errorCount) || !stmt.bindText(5, modalState.distanceMode) ||
        !stmt.bindText(6, modalState.coordinateSystem) ||
        !stmt.bindText(7, modalState.units) ||
        !stmt.bindText(8, modalState.spindleState) ||
        !stmt.bindText(9, modalState.coolantState) ||
        !stmt.bindDouble(10, static_cast<f64>(modalState.feedRate)) ||
        !stmt.bindDouble(11, static_cast<f64>(modalState.spindleSpeed)) ||
        !stmt.bindInt(12, id))
        return false;

    return stmt.execute();
}

std::vector<JobRecord> JobRepository::findRecent(int limit) {
    std::vector<JobRecord> results;

    auto stmt =
        m_db.prepare("SELECT * FROM cnc_jobs ORDER BY started_at DESC LIMIT ?");
    if (!stmt.isValid())
        return results;

    if (!stmt.bindInt(1, limit))
        return results;

    while (stmt.step())
        results.push_back(rowToJob(stmt));

    return results;
}

std::optional<JobRecord> JobRepository::findById(i64 id) {
    auto stmt = m_db.prepare("SELECT * FROM cnc_jobs WHERE id = ?");
    if (!stmt.isValid())
        return std::nullopt;

    if (!stmt.bindInt(1, id))
        return std::nullopt;

    if (stmt.step())
        return rowToJob(stmt);

    return std::nullopt;
}

std::vector<JobRecord> JobRepository::findByStatus(const std::string& status) {
    std::vector<JobRecord> results;

    auto stmt = m_db.prepare("SELECT * FROM cnc_jobs WHERE status = ?");
    if (!stmt.isValid())
        return results;

    if (!stmt.bindText(1, status))
        return results;

    while (stmt.step())
        results.push_back(rowToJob(stmt));

    return results;
}

bool JobRepository::remove(i64 id) {
    auto stmt = m_db.prepare("DELETE FROM cnc_jobs WHERE id = ?");
    if (!stmt.isValid())
        return false;

    if (!stmt.bindInt(1, id))
        return false;

    return stmt.execute();
}

bool JobRepository::clearAll() {
    return m_db.execute("DELETE FROM cnc_jobs");
}

JobRecord JobRepository::rowToJob(Statement& stmt) {
    JobRecord r;
    r.id = stmt.getInt(0);
    r.fileName = stmt.getText(1);
    r.filePath = stmt.getText(2);
    r.totalLines = static_cast<int>(stmt.getInt(3));
    r.lastAckedLine = static_cast<int>(stmt.getInt(4));
    r.status = stmt.getText(5);
    r.errorCount = static_cast<int>(stmt.getInt(6));
    r.elapsedSeconds = static_cast<f32>(stmt.getDouble(7));
    r.modalState.distanceMode = stmt.getText(8);
    r.modalState.coordinateSystem = stmt.getText(9);
    r.modalState.units = stmt.getText(10);
    r.modalState.spindleState = stmt.getText(11);
    r.modalState.coolantState = stmt.getText(12);
    r.modalState.feedRate = static_cast<f32>(stmt.getDouble(13));
    r.modalState.spindleSpeed = static_cast<f32>(stmt.getDouble(14));
    r.startedAt = stmt.getText(15);
    r.endedAt = stmt.getText(16);
    return r;
}

} // namespace dw
