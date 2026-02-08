#include "app_paths.h"

#include <cstdlib>

#include "../utils/file_utils.h"
#include "../utils/log.h"

#ifdef _WIN32
    #include <shlobj.h>
    #include <windows.h>
#else
    #include <pwd.h>
    #include <unistd.h>
#endif

namespace dw {
namespace paths {

namespace {

const char* APP_NAME = "digitalworkshop";
const char* APP_DISPLAY_NAME = "DigitalWorkshop";

#ifdef _WIN32
Path getWindowsKnownFolder(REFKNOWNFOLDERID folderId) {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, nullptr, &path))) {
        Path result(path);
        CoTaskMemFree(path);
        return result;
    }
    return Path();
}
#endif

Path getHomeDir() {
#ifdef _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return Path(userProfile);
    }
    return getWindowsKnownFolder(FOLDERID_Profile);
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return Path(home);
    }
    // Fallback: look up home dir from passwd entry
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return Path(pw->pw_dir);
    }
    log::error("Paths", "Cannot determine home directory: $HOME unset and getpwuid failed");
    return Path("/tmp");
#endif
}

} // namespace

const char* getAppName() {
    return APP_NAME;
}

Path getConfigDir() {
#ifdef _WIN32
    Path appData = getWindowsKnownFolder(FOLDERID_RoamingAppData);
    if (appData.empty()) {
        appData = getHomeDir() / "AppData" / "Roaming";
    }
    return appData / APP_DISPLAY_NAME;
#elif defined(__APPLE__)
    return getHomeDir() / "Library" / "Application Support" / APP_DISPLAY_NAME;
#else
    // Linux - XDG Base Directory
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && xdgConfig[0] != '\0') {
        return Path(xdgConfig) / APP_NAME;
    }
    return getHomeDir() / ".config" / APP_NAME;
#endif
}

Path getDataDir() {
#ifdef _WIN32
    Path localAppData = getWindowsKnownFolder(FOLDERID_LocalAppData);
    if (localAppData.empty()) {
        localAppData = getHomeDir() / "AppData" / "Local";
    }
    return localAppData / APP_DISPLAY_NAME;
#elif defined(__APPLE__)
    return getHomeDir() / "Library" / "Application Support" / APP_DISPLAY_NAME;
#else
    // Linux - XDG Base Directory
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && xdgData[0] != '\0') {
        return Path(xdgData) / APP_NAME;
    }
    return getHomeDir() / ".local" / "share" / APP_NAME;
#endif
}

Path getDefaultProjectsDir() {
#ifdef _WIN32
    Path documents = getWindowsKnownFolder(FOLDERID_Documents);
    if (documents.empty()) {
        documents = getHomeDir() / "Documents";
    }
    return documents / APP_DISPLAY_NAME;
#elif defined(__APPLE__)
    return getHomeDir() / "Documents" / APP_DISPLAY_NAME;
#else
    // Linux - XDG User Directory or fallback
    const char* xdgDocuments = std::getenv("XDG_DOCUMENTS_DIR");
    if (xdgDocuments && xdgDocuments[0] != '\0') {
        return Path(xdgDocuments) / APP_DISPLAY_NAME;
    }
    return getHomeDir() / "Documents" / APP_DISPLAY_NAME;
#endif
}

Path getCacheDir() {
#ifdef _WIN32
    return getDataDir() / "cache";
#elif defined(__APPLE__)
    return getHomeDir() / "Library" / "Caches" / APP_DISPLAY_NAME;
#else
    const char* xdgCache = std::getenv("XDG_CACHE_HOME");
    if (xdgCache && xdgCache[0] != '\0') {
        return Path(xdgCache) / APP_NAME;
    }
    return getHomeDir() / ".cache" / APP_NAME;
#endif
}

Path getThumbnailDir() {
    return getCacheDir() / "thumbnails";
}

Path getDatabasePath() {
    return getDataDir() / "library.db";
}

Path getLogPath() {
    return getDataDir() / "digital_workshop.log";
}

bool ensureDirectoriesExist() {
    bool success = true;

    auto ensureDir = [&success](const Path& path, const char* name) {
        if (!file::createDirectories(path)) {
            log::errorf("Paths", "Failed to create %s directory: %s", name, path.string().c_str());
            success = false;
        } else {
            log::debugf("Paths", "Ensured %s directory: %s", name, path.string().c_str());
        }
    };

    ensureDir(getConfigDir(), "config");
    ensureDir(getDataDir(), "data");
    ensureDir(getCacheDir(), "cache");
    ensureDir(getThumbnailDir(), "thumbnail");
    ensureDir(getDefaultProjectsDir(), "projects");

    return success;
}

} // namespace paths
} // namespace dw
