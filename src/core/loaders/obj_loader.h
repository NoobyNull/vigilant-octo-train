#pragma once

#include "loader.h"

namespace dw {

// Wavefront OBJ file loader
class OBJLoader : public MeshLoader {
public:
    OBJLoader() = default;

    LoadResult load(const Path& path) override;
    bool supports(const std::string& extension) const override;
    std::vector<std::string> extensions() const override;
};

}  // namespace dw
