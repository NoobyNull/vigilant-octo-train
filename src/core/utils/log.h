#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

namespace dw {
namespace log {

enum class Level { Debug, Info, Warning, Error };

// Set minimum log level (default: Info in release, Debug in debug builds)
void setLevel(Level level);
Level getLevel();

// Core logging functions
void debug(std::string_view module, std::string_view message);
void info(std::string_view module, std::string_view message);
void warning(std::string_view module, std::string_view message);
void error(std::string_view module, std::string_view message);

// Internal helper -- do not call directly
namespace detail {

// Dispatch to the correct level function
void logAtLevel(Level level, std::string_view module, std::string_view message);

inline constexpr std::size_t kFormatBufSize = 1024;
inline constexpr const char kTruncationSuffix[] = "...[truncated]";

template <typename... Args>
void formatAndLog(Level level, const char* module, const char* format, Args... args) {
    char buffer[kFormatBufSize];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    int written = std::snprintf(buffer, kFormatBufSize, format, args...);
#pragma GCC diagnostic pop

    // snprintf returns the number of characters that *would* have been written
    // (excluding null terminator). If written >= buffer size, output was truncated.
    if (written >= static_cast<int>(kFormatBufSize)) {
        // Overwrite the tail of the buffer with a truncation marker so the
        // reader knows the message was cut short.
        constexpr std::size_t suffixLen = sizeof(kTruncationSuffix) - 1; // exclude '\0'
        static_assert(suffixLen < kFormatBufSize, "truncation suffix must fit in buffer");
        std::memcpy(buffer + kFormatBufSize - 1 - suffixLen, kTruncationSuffix, suffixLen);
        buffer[kFormatBufSize - 1] = '\0';
    }

    logAtLevel(level, module, buffer);
}

} // namespace detail

// Formatted logging (printf-style)
template <typename... Args> void debugf(const char* module, const char* format, Args... args) {
    if (getLevel() <= Level::Debug) {
        detail::formatAndLog(Level::Debug, module, format, args...);
    }
}

template <typename... Args> void infof(const char* module, const char* format, Args... args) {
    if (getLevel() <= Level::Info) {
        detail::formatAndLog(Level::Info, module, format, args...);
    }
}

template <typename... Args> void warningf(const char* module, const char* format, Args... args) {
    if (getLevel() <= Level::Warning) {
        detail::formatAndLog(Level::Warning, module, format, args...);
    }
}

template <typename... Args> void errorf(const char* module, const char* format, Args... args) {
    detail::formatAndLog(Level::Error, module, format, args...);
}

// Log to file (in addition to console)
void setLogFile(const std::string& path);
void closeLogFile();

} // namespace log
} // namespace dw
