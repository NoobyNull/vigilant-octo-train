#include "obj_loader.h"

#include <cmath>
#include <sstream>
#include <unordered_map>

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {

LoadResult OBJLoader::load(const Path& path) {
    auto content = file::readText(path);
    if (!content) {
        return LoadResult{nullptr, "Failed to read file"};
    }
    return parseContent(*content);
}

LoadResult OBJLoader::loadFromBuffer(const ByteBuffer& data) {
    if (data.empty()) {
        return LoadResult{nullptr, "Empty buffer"};
    }
    std::string content(reinterpret_cast<const char*>(data.data()), data.size());
    return parseContent(content);
}

LoadResult OBJLoader::parseContent(const std::string& content) {
    auto mesh = std::make_shared<Mesh>();

    // Estimate counts from file size to pre-allocate and avoid repeated reallocations.
    // Typical OBJ: ~50 bytes per vertex line, ~30 bytes per face line.
    usize estimatedVertices = content.size() / 50;
    usize estimatedTriangles = content.size() / 30;

    // Temporary storage for OBJ data
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;

    positions.reserve(estimatedVertices);
    normals.reserve(estimatedVertices);
    texCoords.reserve(estimatedVertices);
    mesh->reserve(static_cast<u32>(estimatedVertices), static_cast<u32>(estimatedTriangles * 3));

    // Vertex deduplication
    struct VertexKey {
        int pos = -1;
        int tex = -1;
        int norm = -1;

        bool operator==(const VertexKey& other) const {
            return pos == other.pos && tex == other.tex && norm == other.norm;
        }
    };

    struct VertexKeyHash {
        size_t operator()(const VertexKey& k) const {
            return std::hash<int>()(k.pos) ^ (std::hash<int>()(k.tex) << 1) ^
                   (std::hash<int>()(k.norm) << 2);
        }
    };

    std::unordered_map<VertexKey, u32, VertexKeyHash> vertexMap;

    auto getOrCreateVertex = [&](const VertexKey& key) -> u32 {
        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            return it->second;
        }

        Vertex v;

        // Position (required)
        if (key.pos >= 0 && key.pos < static_cast<int>(positions.size())) {
            v.position = positions[static_cast<usize>(key.pos)];
        }

        // Texture coordinate (optional)
        if (key.tex >= 0 && key.tex < static_cast<int>(texCoords.size())) {
            v.texCoord = texCoords[static_cast<usize>(key.tex)];
        }

        // Normal (optional)
        if (key.norm >= 0 && key.norm < static_cast<int>(normals.size())) {
            v.normal = normals[static_cast<usize>(key.norm)];
        }

        u32 index = mesh->vertexCount();
        mesh->addVertex(v);
        vertexMap[key] = index;
        return index;
    };

    std::istringstream stream(content);
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        lineNumber++;
        line = str::trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        try {
            std::istringstream ls(line);
            std::string cmd;
            ls >> cmd;

            if (cmd == "v") {
                // Vertex position
                Vec3 pos;
                ls >> pos.x >> pos.y >> pos.z;

                // Validate position
                if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) {
                    std::ostringstream oss;
                    oss << "OBJ contains invalid vertex position at line " << lineNumber
                        << " (NaN or Inf values)";
                    return LoadResult{nullptr, oss.str()};
                }

                positions.push_back(pos);
            } else if (cmd == "vt") {
                // Texture coordinate
                Vec2 tex;
                ls >> tex.x >> tex.y;
                texCoords.push_back(tex);
            } else if (cmd == "vn") {
                // Normal
                Vec3 norm;
                ls >> norm.x >> norm.y >> norm.z;

                // Validate normal
                if (!std::isfinite(norm.x) || !std::isfinite(norm.y) || !std::isfinite(norm.z)) {
                    log::warningf("OBJ", "Invalid normal at line %d, skipping", lineNumber);
                    continue;
                }

                normals.push_back(norm);
            } else if (cmd == "mtllib") {
                // Material library reference - log warning but continue
                std::string mtlFile;
                ls >> mtlFile;
                log::warningf("OBJ", "MTL file reference '%s' found but not loaded - continuing without materials", mtlFile.c_str());
            } else if (cmd == "f") {
            // Face - can be triangles, quads, or polygons
            std::vector<u32> faceIndices;
            std::string vertexStr;

            while (ls >> vertexStr) {
                VertexKey key;

                // Parse v, v/vt, v/vt/vn, or v//vn format
                // Cannot use str::split because it skips empty tokens (v//vn -> ["v","vn"])
                std::string components[3];
                int componentCount = 0;
                for (char c : vertexStr) {
                    if (c == '/' && componentCount < 2) {
                        componentCount++;
                    } else {
                        components[componentCount] += c;
                    }
                }
                componentCount++; // Convert from index to count

                auto parseIndex = [](const std::string& s, int totalCount) -> int {
                    int idx = 0;
                    if (s.empty() || !str::parseInt(s, idx))
                        return -1;
                    return (idx > 0) ? idx - 1 : totalCount + idx;
                };

                if (componentCount >= 1) {
                    key.pos = parseIndex(components[0], static_cast<int>(positions.size()));
                }
                if (componentCount >= 2) {
                    key.tex = parseIndex(components[1], static_cast<int>(texCoords.size()));
                }
                if (componentCount >= 3) {
                    key.norm = parseIndex(components[2], static_cast<int>(normals.size()));
                }

                faceIndices.push_back(getOrCreateVertex(key));
            }

                // Triangulate polygon (fan triangulation)
                for (usize i = 2; i < faceIndices.size(); ++i) {
                    mesh->addTriangle(faceIndices[0], faceIndices[i - 1], faceIndices[i]);
                }
            }
            // Ignore: usemtl, g, o, s, etc.
        } catch (const std::exception& e) {
            log::warningf("OBJ", "Error parsing line %d: %s - skipping", lineNumber, e.what());
        }
    }

    if (positions.empty()) {
        return LoadResult{nullptr, "OBJ file contains no vertices"};
    }

    if (mesh->triangleCount() == 0) {
        return LoadResult{nullptr, "OBJ file contains no faces"};
    }

    // Calculate normals if not provided
    if (!mesh->hasNormals()) {
        mesh->recalculateNormals();
    }

    mesh->recalculateBounds();

    log::infof("OBJ", "Loaded: %u vertices, %u triangles", mesh->vertexCount(),
               mesh->triangleCount());

    return LoadResult{mesh, ""};
}

bool OBJLoader::supports(const std::string& extension) const {
    return str::toLower(extension) == "obj";
}

std::vector<std::string> OBJLoader::extensions() const {
    return {"obj"};
}

} // namespace dw
