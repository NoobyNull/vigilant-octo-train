#include "path_resolver.h"

#include "../config/config.h"
#include "app_paths.h"

namespace dw {
namespace PathResolver {

Path categoryRoot(PathCategory cat) {
    auto& cfg = Config::instance();
    switch (cat) {
    case PathCategory::Models:
        return cfg.getModelsDir();
    case PathCategory::Projects:
        return cfg.getProjectsDir();
    case PathCategory::Materials:
        return cfg.getMaterialsDir();
    case PathCategory::GCode:
        return cfg.getGCodeDir();
    case PathCategory::Support:
        return cfg.getSupportDir();
    }
    return cfg.getSupportDir();
}

Path resolve(const Path& storedPath, PathCategory cat) {
    if (storedPath.empty()) {
        return storedPath;
    }
    if (storedPath.is_absolute()) {
        return storedPath;
    }
    return categoryRoot(cat) / storedPath;
}

Path makeStorable(const Path& absolutePath, PathCategory cat) {
    if (absolutePath.empty() || !absolutePath.is_absolute()) {
        return absolutePath;
    }
    Path root = categoryRoot(cat);
    if (root.empty()) {
        return absolutePath;
    }

    // Ensure root ends with separator for proper prefix matching
    std::string rootStr = root.string();
    if (rootStr.back() != '/') {
        rootStr += '/';
    }
    std::string pathStr = absolutePath.string();

    if (pathStr.substr(0, rootStr.size()) == rootStr) {
        return Path(pathStr.substr(rootStr.size()));
    }
    return absolutePath;
}

} // namespace PathResolver
} // namespace dw
