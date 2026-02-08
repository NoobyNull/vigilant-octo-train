#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dw {
namespace str {

// Trim whitespace
std::string trim(std::string_view s);
std::string trimLeft(std::string_view s);
std::string trimRight(std::string_view s);

// Case conversion
std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);

// Split string by delimiter
std::vector<std::string> split(std::string_view s, char delimiter);
std::vector<std::string> split(std::string_view s, std::string_view delimiter);

// Join strings with delimiter
std::string join(const std::vector<std::string>& parts, std::string_view delimiter);

// Check prefix/suffix
bool startsWith(std::string_view s, std::string_view prefix);
bool endsWith(std::string_view s, std::string_view suffix);

// Contains check
bool contains(std::string_view s, std::string_view substring);
bool containsIgnoreCase(std::string_view s, std::string_view substring);

// Replace all occurrences
std::string replace(std::string_view s, std::string_view from, std::string_view to);

// Format file size for display
std::string formatFileSize(uint64_t bytes);

// Format number with thousands separators
std::string formatNumber(int64_t number);

// Parse number from string
bool parseInt(std::string_view s, int& out);
bool parseInt64(std::string_view s, int64_t& out);
bool parseFloat(std::string_view s, float& out);
bool parseDouble(std::string_view s, double& out);

// Escape SQL LIKE wildcards (% and _) with backslash
// Use with: WHERE col LIKE ? ESCAPE '\'
std::string escapeLike(std::string_view s);

// Escape a string for embedding in JSON (handles ", \, control chars)
std::string escapeJsonString(std::string_view s);

} // namespace str
} // namespace dw
