#pragma once

#include <string>

#include "../mesh/mesh.h"
#include "../types.h"

namespace dw {

// Export formats
enum class ExportFormat { STL_Binary, STL_ASCII, OBJ };

// Export result
struct ExportResult {
    bool success = false;
    std::string error;
};

// Model exporter class
class ModelExporter {
  public:
    ModelExporter() = default;

    // Export mesh to file
    ExportResult exportMesh(const Mesh& mesh, const Path& path, ExportFormat format);

    // Auto-detect format from extension
    ExportResult exportMesh(const Mesh& mesh, const Path& path);

  private:
    ExportResult exportSTLBinary(const Mesh& mesh, const Path& path);
    ExportResult exportSTLAscii(const Mesh& mesh, const Path& path);
    ExportResult exportOBJ(const Mesh& mesh, const Path& path);
};

} // namespace dw
