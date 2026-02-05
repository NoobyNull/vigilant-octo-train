#pragma once

#include "loader.h"

namespace dw {

// Wavefront OBJ file loader
class OBJLoader : public MeshLoader {
public:
    OBJLoader() = default;

    LoadResult load(const Path& path) override;
    LoadResult loadFromBuffer(const ByteBuffer& data) override;
    bool supports(const std::string& extension) const override;
    std::vector<std::string> extensions() const override;

private:
    LoadResult parseContent(const std::string& content);
};

}  // namespace dw
