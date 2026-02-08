#include "database.h"

#include <cstring>

#include <sqlite3.h>

#include "../utils/log.h"

namespace dw {

// Statement implementation

Statement::Statement(sqlite3_stmt* stmt) : m_stmt(stmt) {}

Statement::~Statement() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
    }
}

Statement::Statement(Statement&& other) noexcept : m_stmt(other.m_stmt) {
    other.m_stmt = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (m_stmt) {
            sqlite3_finalize(m_stmt);
        }
        m_stmt = other.m_stmt;
        other.m_stmt = nullptr;
    }
    return *this;
}

bool Statement::bindInt(int index, i64 value) {
    return sqlite3_bind_int64(m_stmt, index, value) == SQLITE_OK;
}

bool Statement::bindDouble(int index, f64 value) {
    return sqlite3_bind_double(m_stmt, index, value) == SQLITE_OK;
}

bool Statement::bindText(int index, const std::string& value) {
    return sqlite3_bind_text(m_stmt, index, value.c_str(), static_cast<int>(value.size()),
                             SQLITE_TRANSIENT) == SQLITE_OK;
}

bool Statement::bindBlob(int index, const void* data, int size) {
    return sqlite3_bind_blob(m_stmt, index, data, size, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool Statement::bindNull(int index) {
    return sqlite3_bind_null(m_stmt, index) == SQLITE_OK;
}

bool Statement::step() {
    int result = sqlite3_step(m_stmt);
    return result == SQLITE_ROW;
}

bool Statement::execute() {
    int result = sqlite3_step(m_stmt);
    return result == SQLITE_DONE;
}

void Statement::reset() {
    sqlite3_reset(m_stmt);
    sqlite3_clear_bindings(m_stmt);
}

i64 Statement::getInt(int column) const {
    return sqlite3_column_int64(m_stmt, column);
}

f64 Statement::getDouble(int column) const {
    return sqlite3_column_double(m_stmt, column);
}

std::string Statement::getText(int column) const {
    const unsigned char* text = sqlite3_column_text(m_stmt, column);
    if (text) {
        return std::string(reinterpret_cast<const char*>(text));
    }
    return "";
}

ByteBuffer Statement::getBlob(int column) const {
    const void* data = sqlite3_column_blob(m_stmt, column);
    int size = sqlite3_column_bytes(m_stmt, column);
    if (data && size > 0) {
        ByteBuffer buffer(static_cast<usize>(size));
        std::memcpy(buffer.data(), data, static_cast<usize>(size));
        return buffer;
    }
    return {};
}

bool Statement::isNull(int column) const {
    return sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
}

int Statement::columnCount() const {
    return sqlite3_column_count(m_stmt);
}

std::string Statement::columnName(int column) const {
    const char* name = sqlite3_column_name(m_stmt, column);
    return name ? name : "";
}

// Database implementation

Database::~Database() {
    close();
}

bool Database::open(const Path& path) {
    if (m_db) {
        close();
    }

    int result = sqlite3_open(path.string().c_str(), &m_db);
    if (result != SQLITE_OK) {
        log::errorf("Database", "Failed to open: %s", sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // Enable foreign keys
    if (!execute("PRAGMA foreign_keys = ON")) {
        log::warning("Database", "Failed to enable foreign keys");
    }

    // Use WAL mode for better concurrency
    if (!execute("PRAGMA journal_mode = WAL")) {
        log::warning("Database", "Failed to set WAL mode");
    }

    log::infof("Database", "Opened: %s", path.string().c_str());
    return true;
}

void Database::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        log::debug("Database", "Closed");
    }
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        log::errorf("Database", "SQL error: %s\nQuery: %s", errMsg, sql.c_str());
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

Statement Database::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int result =
        sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);

    if (result != SQLITE_OK) {
        log::errorf("Database", "Failed to prepare statement: %s\nQuery: %s", sqlite3_errmsg(m_db),
                    sql.c_str());
        return Statement();
    }

    return Statement(stmt);
}

bool Database::beginTransaction() {
    return execute("BEGIN TRANSACTION");
}

bool Database::commit() {
    return execute("COMMIT");
}

bool Database::rollback() {
    return execute("ROLLBACK");
}

i64 Database::lastInsertId() const {
    return sqlite3_last_insert_rowid(m_db);
}

int Database::changesCount() const {
    return sqlite3_changes(m_db);
}

std::string Database::lastError() const {
    return sqlite3_errmsg(m_db);
}

// Transaction implementation

Transaction::Transaction(Database& db) : m_db(db) {
    if (!m_db.beginTransaction()) {
        log::error("Transaction", "Failed to begin transaction");
    }
}

Transaction::~Transaction() {
    if (!m_committed) {
        (void)m_db.rollback();
    }
}

bool Transaction::commit() {
    if (!m_committed) {
        m_committed = m_db.commit();
    }
    return m_committed;
}

void Transaction::rollback() {
    if (!m_committed) {
        (void)m_db.rollback();
        m_committed = true; // Prevent double rollback in destructor
    }
}

} // namespace dw
