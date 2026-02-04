#pragma once

#include "../types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace dw {

// Forward declarations
class Database;

// RAII wrapper for prepared statement
class Statement {
public:
    Statement() = default;
    Statement(sqlite3_stmt* stmt);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    // Bind parameters (1-indexed)
    bool bindInt(int index, i64 value);
    bool bindDouble(int index, f64 value);
    bool bindText(int index, const std::string& value);
    bool bindBlob(int index, const void* data, int size);
    bool bindNull(int index);

    // Execute and step
    bool step();      // Returns true if there's a row (SQLITE_ROW)
    bool execute();   // Returns true if successful (SQLITE_DONE)
    void reset();     // Reset for reuse

    // Get column values (0-indexed)
    i64 getInt(int column) const;
    f64 getDouble(int column) const;
    std::string getText(int column) const;
    ByteBuffer getBlob(int column) const;
    bool isNull(int column) const;

    // Get column count and names
    int columnCount() const;
    std::string columnName(int column) const;

    bool isValid() const { return m_stmt != nullptr; }

private:
    sqlite3_stmt* m_stmt = nullptr;
};

// Database connection wrapper
class Database {
public:
    Database() = default;
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Open/close database
    bool open(const Path& path);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    // Execute SQL (for simple queries without results)
    bool execute(const std::string& sql);

    // Prepare statement for queries with results or parameters
    Statement prepare(const std::string& sql);

    // Transaction support
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Utility
    i64 lastInsertId() const;
    int changesCount() const;
    std::string lastError() const;

    // Get raw handle (for advanced usage)
    sqlite3* handle() const { return m_db; }

private:
    sqlite3* m_db = nullptr;
};

// Scoped transaction (RAII)
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    bool commit();
    void rollback();

private:
    Database& m_db;
    bool m_committed = false;
};

}  // namespace dw
