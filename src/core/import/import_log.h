#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "../types.h"

namespace dw {

struct ImportLogRecord {
    std::string timestamp;
    std::string status; // "DONE" or "DUP"
    std::string sourcePath;
    std::string hash;
};

class ImportLog {
  public:
    explicit ImportLog(const Path& logPath);

    void appendDone(const Path& sourcePath, const std::string& hash);
    void appendDup(const Path& sourcePath, const std::string& hash);

    std::unordered_set<std::string> buildSkipSet() const;
    std::vector<ImportLogRecord> readAll() const;
    bool exists() const;
    void remove();

  private:
    void appendLine(const std::string& status, const Path& sourcePath, const std::string& hash);
    Path m_logPath;
};

} // namespace dw
