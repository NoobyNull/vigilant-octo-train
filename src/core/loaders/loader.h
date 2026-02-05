#pragma once

#include "../mesh/mesh.h"
#include "../types.h"

#include <memory>
#include <string>

namespace dw {

// Load result with optional error message
struct LoadResult {
    MeshPtr mesh;
    std::string error;

    bool success() const { return mesh != nullptr; }
    operator bool() const { return success(); }
};

// Abstract mesh loader interface
class MeshLoader {
public:
    virtual ~MeshLoader() = default;

    // Load mesh from file
    virtual LoadResult load(const Path& path) = 0;

    // Load mesh from byte buffer (avoids re-reading file from disk)
    // Default implementation returns an error — override in subclasses.
    virtual LoadResult loadFromBuffer(const ByteBuffer& data) {
        (void)data;
        return LoadResult{nullptr, "loadFromBuffer not supported by this loader"};
    }

    // Check if this loader supports the given file extension
    virtual bool supports(const std::string& extension) const = 0;

    // Get supported extensions
    virtual std::vector<std::string> extensions() const = 0;
};

}  // namespace dw
