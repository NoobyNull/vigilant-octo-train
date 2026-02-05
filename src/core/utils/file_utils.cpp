#include "file_utils.h"

#include "log.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace dw {
namespace file {

Result<std::string> readText(const Path& path) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        log::errorf("FileIO", "Failed to open for reading: %s", path.string().c_str());
        return std::nullopt;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

Result<ByteBuffer> readBinary(const Path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        log::errorf("FileIO", "Failed to open for reading: %s", path.string().c_str());
        return std::nullopt;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    ByteBuffer buffer(static_cast<usize>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(size))) {
        log::errorf("FileIO", "Failed to read: %s", path.string().c_str());
        return std::nullopt;
    }

    return buffer;
}

bool writeText(const Path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        log::errorf("FileIO", "Failed to open for writing: %s", path.string().c_str());
        return false;
    }

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    return file.good();
}

bool writeBinary(const Path& path, const ByteBuffer& data) {
    return writeBinary(path, data.data(), data.size());
}

bool writeBinary(const Path& path, const void* data, usize size) {
    std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        log::errorf("FileIO", "Failed to open for writing: %s", path.string().c_str());
        return false;
    }

    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

bool exists(const Path& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

bool isFile(const Path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool isDirectory(const Path& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool createDirectory(const Path& path) {
    std::error_code ec;
    bool result = fs::create_directory(path, ec);
    if (ec) {
        log::errorf("FileIO", "Failed to create directory: %s (%s)", path.string().c_str(),
                    ec.message().c_str());
    }
    return result || file::exists(path);
}

bool createDirectories(const Path& path) {
    std::error_code ec;
    bool result = fs::create_directories(path, ec);
    if (ec) {
        log::errorf("FileIO", "Failed to create directories: %s (%s)", path.string().c_str(),
                    ec.message().c_str());
    }
    return result || file::exists(path);
}

bool remove(const Path& path) {
    std::error_code ec;
    bool result = fs::remove(path, ec);
    if (ec) {
        log::errorf("FileIO", "Failed to remove: %s (%s)", path.string().c_str(),
                    ec.message().c_str());
    }
    return result;
}

bool copy(const Path& from, const Path& to) {
    std::error_code ec;
    fs::copy(from, to, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        log::errorf("FileIO", "Failed to copy %s to %s: %s", from.string().c_str(),
                    to.string().c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

bool move(const Path& from, const Path& to) {
    std::error_code ec;
    fs::rename(from, to, ec);
    if (ec) {
        log::errorf("FileIO", "Failed to move %s to %s: %s", from.string().c_str(),
                    to.string().c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

Result<u64> getFileSize(const Path& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec) {
        return std::nullopt;
    }
    return static_cast<u64>(size);
}

Result<i64> getModificationTime(const Path& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) {
        return std::nullopt;
    }
    // C++17 compatible conversion: use file_time_type duration
    auto duration = ftime.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return static_cast<i64>(seconds.count());
}

std::vector<Path> listFiles(const Path& directory) {
    std::vector<Path> files;
    std::error_code ec;

    if (!fs::is_directory(directory, ec)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    return files;
}

std::vector<Path> listFiles(const Path& directory, const std::string& extension) {
    std::vector<Path> files;
    std::error_code ec;

    if (!fs::is_directory(directory, ec)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (entry.is_regular_file()) {
            std::string ext = getExtension(entry.path());
            if (ext == extension) {
                files.push_back(entry.path());
            }
        }
    }

    return files;
}

std::string getExtension(const Path& path) {
    std::string ext = path.extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

std::string getStem(const Path& path) {
    return path.stem().string();
}

Path makeAbsolute(const Path& path) {
    std::error_code ec;
    return fs::absolute(path, ec);
}

Path getParent(const Path& path) {
    return path.parent_path();
}

Path currentDirectory() {
    std::error_code ec;
    return fs::current_path(ec);
}

Path homeDirectory() {
    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home);
    }
#ifdef _WIN32
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile) {
        return Path(userprofile);
    }
#endif
    return currentDirectory();
}

std::vector<std::string> listEntries(const Path& directory) {
    std::vector<std::string> entries;
    std::error_code ec;

    if (!fs::is_directory(directory, ec)) {
        return entries;
    }

    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        entries.push_back(entry.path().filename().string());
    }

    return entries;
}

u64 fileSize(const Path& path) {
    auto result = getFileSize(path);
    return result.value_or(0);
}

}  // namespace file
}  // namespace dw
