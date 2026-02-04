#include "log.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace dw {
namespace log {

namespace {

Level g_minLevel = Level::Info;
std::ofstream g_logFile;
std::mutex g_logMutex;

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

const char* levelToString(Level level) {
    switch (level) {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO ";
        case Level::Warning:
            return "WARN ";
        case Level::Error:
            return "ERROR";
    }
    return "?????";
}

const char* levelToColor(Level level) {
    switch (level) {
        case Level::Debug:
            return "\033[36m";  // Cyan
        case Level::Info:
            return "\033[32m";  // Green
        case Level::Warning:
            return "\033[33m";  // Yellow
        case Level::Error:
            return "\033[31m";  // Red
    }
    return "\033[0m";
}

void logMessage(Level level, std::string_view message) {
    if (level < g_minLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_logMutex);

    std::string timestamp = getCurrentTimestamp();
    const char* levelStr = levelToString(level);
    const char* color = levelToColor(level);
    const char* reset = "\033[0m";

    // Console output with color
    std::cerr << color << "[" << timestamp << "] [" << levelStr << "] " << reset
              << message << std::endl;

    // File output without color
    if (g_logFile.is_open()) {
        g_logFile << "[" << timestamp << "] [" << levelStr << "] " << message
                  << std::endl;
    }
}

}  // namespace

void setLevel(Level level) {
    g_minLevel = level;
}

Level getLevel() {
    return g_minLevel;
}

void debug(std::string_view message) {
    logMessage(Level::Debug, message);
}

void info(std::string_view message) {
    logMessage(Level::Info, message);
}

void warning(std::string_view message) {
    logMessage(Level::Warning, message);
}

void error(std::string_view message) {
    logMessage(Level::Error, message);
}

void setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
    g_logFile.open(path, std::ios::app);
    if (!g_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
    }
}

void closeLogFile() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
}

}  // namespace log
}  // namespace dw
