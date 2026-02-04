#include "string_utils.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace dw {
namespace str {

std::string trim(std::string_view s) {
    return trimRight(trimLeft(s));
}

std::string trimLeft(std::string_view s) {
    auto it = std::find_if(s.begin(), s.end(),
                           [](unsigned char c) { return !std::isspace(c); });
    return std::string(it, s.end());
}

std::string trimRight(std::string_view s) {
    auto it = std::find_if(s.rbegin(), s.rend(),
                           [](unsigned char c) { return !std::isspace(c); });
    return std::string(s.begin(), it.base());
}

std::string toLower(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::vector<std::string> split(std::string_view s, char delimiter) {
    std::vector<std::string> result;
    std::string current;

    for (char c : s) {
        if (c == delimiter) {
            if (!current.empty()) {
                result.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        result.push_back(std::move(current));
    }

    return result;
}

std::vector<std::string> split(std::string_view s, std::string_view delimiter) {
    std::vector<std::string> result;

    if (delimiter.empty()) {
        result.emplace_back(s);
        return result;
    }

    size_t pos = 0;
    size_t prev = 0;

    while ((pos = s.find(delimiter, prev)) != std::string_view::npos) {
        if (pos > prev) {
            result.emplace_back(s.substr(prev, pos - prev));
        }
        prev = pos + delimiter.length();
    }

    if (prev < s.length()) {
        result.emplace_back(s.substr(prev));
    }

    return result;
}

std::string join(const std::vector<std::string>& parts, std::string_view delimiter) {
    if (parts.empty()) {
        return "";
    }

    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter;
        result += parts[i];
    }
    return result;
}

bool startsWith(std::string_view s, std::string_view prefix) {
    if (prefix.length() > s.length()) {
        return false;
    }
    return s.substr(0, prefix.length()) == prefix;
}

bool endsWith(std::string_view s, std::string_view suffix) {
    if (suffix.length() > s.length()) {
        return false;
    }
    return s.substr(s.length() - suffix.length()) == suffix;
}

bool contains(std::string_view s, std::string_view substring) {
    return s.find(substring) != std::string_view::npos;
}

bool containsIgnoreCase(std::string_view s, std::string_view substring) {
    std::string sLower = toLower(s);
    std::string subLower = toLower(substring);
    return sLower.find(subLower) != std::string::npos;
}

std::string replace(std::string_view s, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return std::string(s);
    }

    std::string result;
    result.reserve(s.length());

    size_t pos = 0;
    size_t prev = 0;

    while ((pos = s.find(from, prev)) != std::string_view::npos) {
        result.append(s.substr(prev, pos - prev));
        result.append(to);
        prev = pos + from.length();
    }

    result.append(s.substr(prev));
    return result;
}

std::string formatFileSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream ss;
    if (unitIndex == 0) {
        ss << bytes << " " << units[unitIndex];
    } else {
        ss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    }
    return ss.str();
}

std::string formatNumber(int64_t number) {
    std::string s = std::to_string(std::abs(number));
    std::string result;
    result.reserve(s.length() + s.length() / 3);

    int count = 0;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            result.push_back(',');
        }
        result.push_back(*it);
        count++;
    }

    if (number < 0) {
        result.push_back('-');
    }

    std::reverse(result.begin(), result.end());
    return result;
}

bool parseInt(std::string_view s, int& out) {
    auto result = std::from_chars(s.data(), s.data() + s.size(), out);
    return result.ec == std::errc{} && result.ptr == s.data() + s.size();
}

bool parseFloat(std::string_view s, float& out) {
    char* end = nullptr;
    std::string str(s);
    out = std::strtof(str.c_str(), &end);
    return end == str.c_str() + str.size();
}

bool parseDouble(std::string_view s, double& out) {
    char* end = nullptr;
    std::string str(s);
    out = std::strtod(str.c_str(), &end);
    return end == str.c_str() + str.size();
}

}  // namespace str
}  // namespace dw
