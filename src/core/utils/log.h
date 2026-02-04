#pragma once

#include <cstdio>
#include <string>
#include <string_view>

namespace dw {
namespace log {

enum class Level { Debug, Info, Warning, Error };

// Set minimum log level (default: Info in release, Debug in debug builds)
void setLevel(Level level);
Level getLevel();

// Core logging functions
void debug(std::string_view message);
void info(std::string_view message);
void warning(std::string_view message);
void error(std::string_view message);

// Formatted logging (printf-style)
template <typename... Args>
void debugf(const char* format, Args... args) {
    if (getLevel() <= Level::Debug) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), format, args...);
        debug(buffer);
    }
}

template <typename... Args>
void infof(const char* format, Args... args) {
    if (getLevel() <= Level::Info) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), format, args...);
        info(buffer);
    }
}

template <typename... Args>
void warningf(const char* format, Args... args) {
    if (getLevel() <= Level::Warning) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), format, args...);
        warning(buffer);
    }
}

template <typename... Args>
void errorf(const char* format, Args... args) {
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer), format, args...);
    error(buffer);
}

// Log to file (in addition to console)
void setLogFile(const std::string& path);
void closeLogFile();

}  // namespace log
}  // namespace dw
