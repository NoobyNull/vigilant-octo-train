#include "model_exporter.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

ExportResult ModelExporter::exportMesh(const Mesh& mesh, const Path& path, ExportFormat format) {
    switch (format) {
    case ExportFormat::STL_Binary:
        return exportSTLBinary(mesh, path);
    case ExportFormat::STL_ASCII:
        return exportSTLAscii(mesh, path);
    case ExportFormat::OBJ:
        return exportOBJ(mesh, path);
    }

    return ExportResult{false, "Unknown export format"};
}

ExportResult ModelExporter::exportMesh(const Mesh& mesh, const Path& path) {
    std::string ext = str::toLower(file::getExtension(path));

    if (ext == "stl") {
        return exportSTLBinary(mesh, path); // Default to binary
    } else if (ext == "obj") {
        return exportOBJ(mesh, path);
    }

    return ExportResult{false, "Unsupported file extension: " + ext};
}

ExportResult ModelExporter::exportSTLBinary(const Mesh& mesh, const Path& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return ExportResult{false, "Failed to create file"};
    }

    // 80-byte header
    char header[80] = {};
    std::snprintf(header, sizeof(header), "Digital Workshop Export");
    file.write(header, 80);

    // Triangle count
    u32 triangleCount = mesh.triangleCount();
    file.write(reinterpret_cast<const char*>(&triangleCount), 4);

    // Write triangles
    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();

    for (u32 i = 0; i < triangleCount; ++i) {
        u32 i0 = indices[i * 3];
        u32 i1 = indices[i * 3 + 1];
        u32 i2 = indices[i * 3 + 2];

        const Vertex& v0 = vertices[i0];
        const Vertex& v1 = vertices[i1];
        const Vertex& v2 = vertices[i2];

        // Calculate face normal
        Vec3 edge1 = v1.position - v0.position;
        Vec3 edge2 = v2.position - v0.position;
        Vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Write normal
        file.write(reinterpret_cast<const char*>(&normal.x), 4);
        file.write(reinterpret_cast<const char*>(&normal.y), 4);
        file.write(reinterpret_cast<const char*>(&normal.z), 4);

        // Write vertices
        file.write(reinterpret_cast<const char*>(&v0.position.x), 4);
        file.write(reinterpret_cast<const char*>(&v0.position.y), 4);
        file.write(reinterpret_cast<const char*>(&v0.position.z), 4);

        file.write(reinterpret_cast<const char*>(&v1.position.x), 4);
        file.write(reinterpret_cast<const char*>(&v1.position.y), 4);
        file.write(reinterpret_cast<const char*>(&v1.position.z), 4);

        file.write(reinterpret_cast<const char*>(&v2.position.x), 4);
        file.write(reinterpret_cast<const char*>(&v2.position.y), 4);
        file.write(reinterpret_cast<const char*>(&v2.position.z), 4);

        // Attribute byte count (unused)
        u16 attrByteCount = 0;
        file.write(reinterpret_cast<const char*>(&attrByteCount), 2);
    }

    log::infof("Export", "Binary STL: %s (%u triangles)", path.string().c_str(), triangleCount);

    return ExportResult{true, ""};
}

ExportResult ModelExporter::exportSTLAscii(const Mesh& mesh, const Path& path) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);

    ss << "solid mesh\n";

    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();
    u32 triangleCount = mesh.triangleCount();

    for (u32 i = 0; i < triangleCount; ++i) {
        u32 i0 = indices[i * 3];
        u32 i1 = indices[i * 3 + 1];
        u32 i2 = indices[i * 3 + 2];

        const Vertex& v0 = vertices[i0];
        const Vertex& v1 = vertices[i1];
        const Vertex& v2 = vertices[i2];

        // Calculate face normal
        Vec3 edge1 = v1.position - v0.position;
        Vec3 edge2 = v2.position - v0.position;
        Vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        ss << "  facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
        ss << "    outer loop\n";
        ss << "      vertex " << v0.position.x << " " << v0.position.y << " " << v0.position.z
           << "\n";
        ss << "      vertex " << v1.position.x << " " << v1.position.y << " " << v1.position.z
           << "\n";
        ss << "      vertex " << v2.position.x << " " << v2.position.y << " " << v2.position.z
           << "\n";
        ss << "    endloop\n";
        ss << "  endfacet\n";
    }

    ss << "endsolid mesh\n";

    if (!file::writeText(path, ss.str())) {
        return ExportResult{false, "Failed to write file"};
    }

    log::infof("Export", "ASCII STL: %s (%u triangles)", path.string().c_str(), triangleCount);

    return ExportResult{true, ""};
}

ExportResult ModelExporter::exportOBJ(const Mesh& mesh, const Path& path) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);

    ss << "# Digital Workshop Export\n";
    ss << "# Vertices: " << mesh.vertexCount() << "\n";
    ss << "# Triangles: " << mesh.triangleCount() << "\n\n";

    const auto& vertices = mesh.vertices();
    const auto& indices = mesh.indices();

    // Write vertices
    for (const auto& v : vertices) {
        ss << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
    }

    ss << "\n";

    // Write normals
    bool hasNormals = mesh.hasNormals();
    if (hasNormals) {
        for (const auto& v : vertices) {
            ss << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
        }
        ss << "\n";
    }

    // Write texture coordinates
    bool hasTexCoords = mesh.hasTexCoords();
    if (hasTexCoords) {
        for (const auto& v : vertices) {
            ss << "vt " << v.texCoord.x << " " << v.texCoord.y << "\n";
        }
        ss << "\n";
    }

    // Write faces
    u32 triangleCount = mesh.triangleCount();
    for (u32 i = 0; i < triangleCount; ++i) {
        u32 i0 = indices[i * 3] + 1; // OBJ is 1-indexed
        u32 i1 = indices[i * 3 + 1] + 1;
        u32 i2 = indices[i * 3 + 2] + 1;

        if (hasNormals && hasTexCoords) {
            ss << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " "
               << i2 << "/" << i2 << "/" << i2 << "\n";
        } else if (hasNormals) {
            ss << "f " << i0 << "//" << i0 << " " << i1 << "//" << i1 << " " << i2 << "//" << i2
               << "\n";
        } else if (hasTexCoords) {
            ss << "f " << i0 << "/" << i0 << " " << i1 << "/" << i1 << " " << i2 << "/" << i2
               << "\n";
        } else {
            ss << "f " << i0 << " " << i1 << " " << i2 << "\n";
        }
    }

    if (!file::writeText(path, ss.str())) {
        return ExportResult{false, "Failed to write file"};
    }

    log::infof("Export", "OBJ: %s (%u triangles)", path.string().c_str(), triangleCount);

    return ExportResult{true, ""};
}

} // namespace dw
