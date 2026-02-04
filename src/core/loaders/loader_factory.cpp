#include "loader_factory.h"

#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include "obj_loader.h"
#include "stl_loader.h"

namespace dw {

std::unique_ptr<MeshLoader> LoaderFactory::getLoader(const Path& path) {
    std::string ext = file::getExtension(path);
    return getLoaderByExtension(ext);
}

std::unique_ptr<MeshLoader> LoaderFactory::getLoaderByExtension(const std::string& ext) {
    std::string lower = str::toLower(ext);

    if (lower == "stl") {
        return std::make_unique<STLLoader>();
    }
    if (lower == "obj") {
        return std::make_unique<OBJLoader>();
    }

    return nullptr;
}

LoadResult LoaderFactory::load(const Path& path) {
    auto loader = getLoader(path);
    if (!loader) {
        return LoadResult{nullptr, "Unsupported file format"};
    }

    return loader->load(path);
}

bool LoaderFactory::isSupported(const std::string& extension) {
    auto loader = getLoaderByExtension(extension);
    return loader != nullptr;
}

std::vector<std::string> LoaderFactory::supportedExtensions() {
    return {"stl", "obj"};
}

}  // namespace dw
