#include "stl_loader.h"

#include <cmath>
#include <cstring>
#include <sstream>
#include <unordered_map>

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

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
        return false; // Too small for binary STL
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

    // Handle empty STL
    if (triangleCount == 0) {
        return LoadResult{nullptr, "STL file contains no geometry (0 triangles)"};
    }

    // Guard against integer overflow: triangleCount * 50 must not wrap
    if (triangleCount > (SIZE_MAX - 84) / 50) {
        return LoadResult{nullptr, "Invalid binary STL: triangle count causes overflow"};
    }

    // Validate size
    usize expectedSize = 84 + static_cast<usize>(triangleCount) * 50;
    if (data.size() < expectedSize) {
        std::ostringstream oss;
        oss << "STL file truncated: expected " << triangleCount << " triangles but file too short";
        return LoadResult{nullptr, oss.str()};
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

        // Validate normal for NaN/Inf
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
            return LoadResult{nullptr, "STL contains invalid normal data (NaN or Inf values)"};
        }

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

            // Validate vertex for NaN/Inf
            if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y) ||
                !std::isfinite(v.position.z)) {
                return LoadResult{nullptr, "STL contains invalid vertex data (NaN or Inf values)"};
            }

            // Bounds check on vertex data
            constexpr f32 MAX_COORD = 1e6f;
            if (std::abs(v.position.x) > MAX_COORD || std::abs(v.position.y) > MAX_COORD ||
                std::abs(v.position.z) > MAX_COORD) {
                return LoadResult{nullptr,
                                  "STL contains extreme coordinates (>1e6), likely corrupt"};
            }

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
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        lineNumber++;
        line = str::trim(line);
        if (line.empty()) {
            continue;
        }

        std::string lower = str::toLower(line);

        if (str::startsWith(lower, "facet normal")) {
            // Parse normal
            try {
                std::istringstream ns(line.substr(12));
                ns >> currentNormal.x >> currentNormal.y >> currentNormal.z;

                // Validate normal
                if (!std::isfinite(currentNormal.x) || !std::isfinite(currentNormal.y) ||
                    !std::isfinite(currentNormal.z)) {
                    std::ostringstream oss;
                    oss << "STL contains invalid normal data at line " << lineNumber
                        << " (NaN or Inf values)";
                    return LoadResult{nullptr, oss.str()};
                }
            } catch (const std::exception& e) {
                log::warningf("STL", "Malformed normal at line %d, using default", lineNumber);
                currentNormal = Vec3{0.0f, 0.0f, 1.0f};
            }
            faceIndices.clear();
        } else if (str::startsWith(lower, "vertex")) {
            // Parse vertex
            try {
                std::istringstream vs(line.substr(6));
                Vertex v;
                vs >> v.position.x >> v.position.y >> v.position.z;
                v.normal = currentNormal;

                // Validate vertex
                if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y) ||
                    !std::isfinite(v.position.z)) {
                    std::ostringstream oss;
                    oss << "STL contains invalid vertex data at line " << lineNumber
                        << " (NaN or Inf values)";
                    return LoadResult{nullptr, oss.str()};
                }

                // Bounds check
                constexpr f32 MAX_COORD = 1e6f;
                if (std::abs(v.position.x) > MAX_COORD || std::abs(v.position.y) > MAX_COORD ||
                    std::abs(v.position.z) > MAX_COORD) {
                    std::ostringstream oss;
                    oss << "STL contains extreme coordinates at line " << lineNumber
                        << " (>1e6), likely corrupt";
                    return LoadResult{nullptr, oss.str()};
                }

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
            } catch (const std::exception& e) {
                log::warningf("STL", "Malformed vertex at line %d, skipping", lineNumber);
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

} // namespace dw
