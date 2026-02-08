#pragma once

#include "loader.h"

namespace dw {

// 3MF (3D Manufacturing Format) loader
// Note: 3MF files are ZIP archives containing XML model data.
// This implementation handles uncompressed ZIP entries.
// Compressed entries require extraction first.
class ThreeMFLoader : public MeshLoader {
  public:
    ThreeMFLoader() = default;
    ~ThreeMFLoader() override = default;

    [[nodiscard]] LoadResult load(const Path& path) override;
    [[nodiscard]] LoadResult loadFromBuffer(const ByteBuffer& data) override;
    bool supports(const std::string& extension) const override;
    std::vector<std::string> extensions() const override;

  private:
    // Parse the 3dmodel.model XML file
    LoadResult parseModelXML(const std::string& xmlContent);

    // Extract vertices and triangles from XML
    bool parseVertices(const std::string& verticesBlock, std::vector<Vec3>& outVertices);
    bool parseTriangles(const std::string& trianglesBlock, const std::vector<Vec3>& vertices,
                        Mesh& outMesh);

    // Simple ZIP extraction for the model file
    std::string extractModelXML(const Path& zipPath);
    std::string extractModelXMLFromBuffer(const ByteBuffer& data);
};

} // namespace dw
