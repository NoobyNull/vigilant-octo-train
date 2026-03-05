#pragma once

#include <cstdio>
#include <cstring>
#include <string>

namespace dw {

/// Clear a fixed-size char buffer (zero-fill).
template <size_t N>
inline void clearBuffer(char (&buf)[N]) {
    std::memset(buf, 0, N);
}

/// Fill a fixed-size char buffer from a string, clearing it first.
/// Safely null-terminates by copying at most N-1 characters.
template <size_t N>
inline void fillBuffer(char (&buf)[N], const std::string& value) {
    std::memset(buf, 0, N);
    std::strncpy(buf, value.c_str(), N - 1);
}

/// Fill a fixed-size char buffer using printf-style formatting, clearing first.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
template <size_t N, typename... Args>
inline void formatBuffer(char (&buf)[N], const char* fmt, Args... args) {
    std::memset(buf, 0, N);
    std::snprintf(buf, N, fmt, args...);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

} // namespace dw
