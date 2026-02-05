#include "stl_loader.h"

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

#include <cstring>
#include <sstream>
#include <unordered_map>

namespace dw {

LoadResult STLLoader::load(const Path& path) {
    auto data = file::readBinary(path);
    if (!data) {
        return LoadResult{nullptr, "Failed to read file"};
    }

    // Detect format
    if (isBinary(*data)) {
        return loadBinary(*data);
    }

    // Try ASCII
    std::string content(reinterpret_cast<const char*>(data->data()), data->size());
    return loadAscii(content);
}

LoadResult STLLoader::loadFromBuffer(const ByteBuffer& data) {
    if (data.empty()) {
        return LoadResult{nullptr, "Empty buffer"};
    }

    if (isBinary(data)) {
        return loadBinary(data);
    }

    std::string content(reinterpret_cast<const char*>(data.data()), data.size());
    return loadAscii(content);
}

bool STLLoader::supports(const std::string& extension) const {
    return str::toLower(extension) == "stl";
}

std::vector<std::string> STLLoader::extensions() const {
    return {"stl"};
}

bool STLLoader::isBinary(const ByteBuffer& data) {
    if (data.size() < 84) {
        return false;  // Too small for binary STL
    }

    // Check if it starts with "solid" (ASCII) but also has binary header
    // Binary STL has: 80 byte header + 4 byte triangle count
    std::string start(reinterpret_cast<const char*>(data.data()),
                      std::min(data.size(), static_cast<usize>(6)));

    if (str::toLower(start).find("solid") == 0) {
        // Could be ASCII - check if file size matches binary format
        u32 triangleCount = 0;
        std::memcpy(&triangleCount, data.data() + 80, sizeof(triangleCount));

        // Binary STL size = 80 + 4 + (triangleCount * 50)
        usize expectedSize = 84 + static_cast<usize>(triangleCount) * 50;

        // If size doesn't match binary format, it's likely ASCII
        if (data.size() != expectedSize) {
            return false;
        }
    }

    return true;
}

LoadResult STLLoader::loadBinary(const ByteBuffer& data) {
    if (data.size() < 84) {
        return LoadResult{nullptr, "File too small for binary STL"};
    }

    // Skip 80-byte header
    const u8* ptr = data.data() + 80;

    // Read triangle count
    u32 triangleCount = 0;
    std::memcpy(&triangleCount, ptr, sizeof(triangleCount));
    ptr += 4;

    // Validate size
    usize expectedSize = 84 + static_cast<usize>(triangleCount) * 50;
    if (data.size() < expectedSize) {
        return LoadResult{nullptr, "Invalid binary STL: file size mismatch"};
    }

    auto mesh = std::make_shared<Mesh>();
    mesh->reserve(triangleCount * 3, triangleCount * 3);

    // Use hash map for vertex deduplication
    std::unordered_map<Vertex, u32> vertexMap;

    for (u32 i = 0; i < triangleCount; ++i) {
        // Read normal (12 bytes)
        Vec3 normal;
        std::memcpy(&normal.x, ptr, sizeof(f32));
        ptr += 4;
        std::memcpy(&normal.y, ptr, sizeof(f32));
        ptr += 4;
        std::memcpy(&normal.z, ptr, sizeof(f32));
        ptr += 4;

        // Read 3 vertices (36 bytes)
        u32 indices[3];
        for (int j = 0; j < 3; ++j) {
            Vertex v;
            std::memcpy(&v.position.x, ptr, sizeof(f32));
            ptr += 4;
            std::memcpy(&v.position.y, ptr, sizeof(f32));
            ptr += 4;
            std::memcpy(&v.position.z, ptr, sizeof(f32));
            ptr += 4;
            v.normal = normal;

            // Deduplicate vertices
            auto it = vertexMap.find(v);
            if (it != vertexMap.end()) {
                indices[j] = it->second;
            } else {
                indices[j] = mesh->vertexCount();
                vertexMap[v] = indices[j];
                mesh->addVertex(v);
            }
        }

        mesh->addTriangle(indices[0], indices[1], indices[2]);

        // Skip attribute byte count (2 bytes)
        ptr += 2;
    }

    mesh->recalculateBounds();

    log::infof("STL", "Loaded binary: %u vertices, %u triangles", mesh->vertexCount(),
               mesh->triangleCount());

    return LoadResult{mesh, ""};
}

LoadResult STLLoader::loadAscii(const std::string& content) {
    auto mesh = std::make_shared<Mesh>();
    std::unordered_map<Vertex, u32> vertexMap;

    std::istringstream stream(content);
    std::string line;
    Vec3 currentNormal{0.0f, 0.0f, 1.0f};
    std::vector<u32> faceIndices;

    while (std::getline(stream, line)) {
        line = str::trim(line);
        if (line.empty()) {
            continue;
        }

        std::string lower = str::toLower(line);

        if (str::startsWith(lower, "facet normal")) {
            // Parse normal
            std::istringstream ns(line.substr(12));
            ns >> currentNormal.x >> currentNormal.y >> currentNormal.z;
            faceIndices.clear();
        } else if (str::startsWith(lower, "vertex")) {
            // Parse vertex
            std::istringstream vs(line.substr(6));
            Vertex v;
            vs >> v.position.x >> v.position.y >> v.position.z;
            v.normal = currentNormal;

            // Deduplicate
            auto it = vertexMap.find(v);
            if (it != vertexMap.end()) {
                faceIndices.push_back(it->second);
            } else {
                u32 index = mesh->vertexCount();
                vertexMap[v] = index;
                mesh->addVertex(v);
                faceIndices.push_back(index);
            }
        } else if (str::startsWith(lower, "endfacet")) {
            // Add triangle
            if (faceIndices.size() >= 3) {
                mesh->addTriangle(faceIndices[0], faceIndices[1], faceIndices[2]);
            }
        }
    }

    if (mesh->triangleCount() == 0) {
        return LoadResult{nullptr, "No triangles found in ASCII STL"};
    }

    mesh->recalculateBounds();

    log::infof("STL", "Loaded ASCII: %u vertices, %u triangles", mesh->vertexCount(),
               mesh->triangleCount());

    return LoadResult{mesh, ""};
}

}  // namespace dw
