#include "import_log.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include <unistd.h>

namespace dw {

ImportLog::ImportLog(const Path& logPath) : m_logPath(logPath) {}

void ImportLog::appendDone(const Path& sourcePath, const std::string& hash) {
    appendLine("DONE", sourcePath, hash);
}

void ImportLog::appendDup(const Path& sourcePath, const std::string& hash) {
    appendLine("DUP", sourcePath, hash);
}

void ImportLog::appendLine(const std::string& status,
                           const Path& sourcePath,
                           const std::string& hash) {
    // ISO 8601 timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&time, &tm);

    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", &tm);

    // Append-only write with fsync
    std::FILE* fp = std::fopen(m_logPath.c_str(), "a");
    if (!fp)
        return;
    std::fprintf(fp, "%s %s %s %s\n", timeBuf, status.c_str(), sourcePath.c_str(), hash.c_str());
    std::fflush(fp);
    // fsync for durability
    int fd = fileno(fp);
    if (fd >= 0)
        fsync(fd);
    std::fclose(fp);
}

std::unordered_set<std::string> ImportLog::buildSkipSet() const {
    std::unordered_set<std::string> result;
    std::ifstream in(m_logPath);
    if (!in.is_open())
        return result;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        // Format: TIMESTAMP STATUS SOURCE_PATH HASH
        // Find the source path: skip timestamp (first space), skip status (second space)
        auto pos1 = line.find(' ');
        if (pos1 == std::string::npos)
            continue;
        auto pos2 = line.find(' ', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;
        auto pos3 = line.rfind(' ');
        if (pos3 == std::string::npos || pos3 <= pos2)
            continue;
        std::string sourcePath = line.substr(pos2 + 1, pos3 - pos2 - 1);
        result.insert(sourcePath);
    }
    return result;
}

std::vector<ImportLogRecord> ImportLog::readAll() const {
    std::vector<ImportLogRecord> records;
    std::ifstream in(m_logPath);
    if (!in.is_open())
        return records;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        auto pos1 = line.find(' ');
        if (pos1 == std::string::npos)
            continue;
        auto pos2 = line.find(' ', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;
        auto pos3 = line.rfind(' ');
        if (pos3 == std::string::npos || pos3 <= pos2)
            continue;

        ImportLogRecord rec;
        rec.timestamp = line.substr(0, pos1);
        rec.status = line.substr(pos1 + 1, pos2 - pos1 - 1);
        rec.sourcePath = line.substr(pos2 + 1, pos3 - pos2 - 1);
        rec.hash = line.substr(pos3 + 1);
        records.push_back(std::move(rec));
    }
    return records;
}

bool ImportLog::exists() const {
    return std::filesystem::exists(m_logPath);
}

void ImportLog::remove() {
    std::filesystem::remove(m_logPath);
}

} // namespace dw
