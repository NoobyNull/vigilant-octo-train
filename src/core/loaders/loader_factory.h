#pragma once

#include <memory>

#include "loader.h"

namespace dw {

// Factory for creating appropriate loader based on file extension
class LoaderFactory {
  public:
    // Get loader for a specific file
    static std::unique_ptr<MeshLoader> getLoader(const Path& path);

    // Get loader by extension
    static std::unique_ptr<MeshLoader> getLoaderByExtension(const std::string& ext);

    // Load file directly (convenience function)
    static LoadResult load(const Path& path);

    // Load from byte buffer with known extension
    static LoadResult loadFromBuffer(const ByteBuffer& data, const std::string& extension);

    // Check if a file format is supported
    static bool isSupported(const std::string& extension);

    // Get all supported extensions
    static std::vector<std::string> supportedExtensions();
};

} // namespace dw
