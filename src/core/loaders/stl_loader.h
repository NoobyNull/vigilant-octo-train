#pragma once

#include "loader.h"

namespace dw {

// STL file loader (ASCII and Binary)
class STLLoader : public MeshLoader {
public:
    STLLoader() = default;

    LoadResult load(const Path& path) override;
    LoadResult loadFromBuffer(const ByteBuffer& data) override;
    bool supports(const std::string& extension) const override;
    std::vector<std::string> extensions() const override;

private:
    LoadResult loadBinary(const ByteBuffer& data);
    LoadResult loadAscii(const std::string& content);
    bool isBinary(const ByteBuffer& data);
};

}  // namespace dw
