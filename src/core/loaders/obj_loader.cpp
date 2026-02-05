#include "obj_loader.h"

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

#include <sstream>
#include <unordered_map>

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

    // Temporary storage for OBJ data
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;

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

    while (std::getline(stream, line)) {
        line = str::trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream ls(line);
        std::string cmd;
        ls >> cmd;

        if (cmd == "v") {
            // Vertex position
            Vec3 pos;
            ls >> pos.x >> pos.y >> pos.z;
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
            normals.push_back(norm);
        } else if (cmd == "f") {
            // Face - can be triangles, quads, or polygons
            std::vector<u32> faceIndices;
            std::string vertexStr;

            while (ls >> vertexStr) {
                VertexKey key;

                // Parse v/vt/vn format
                auto parts = str::split(vertexStr, '/');

                if (!parts.empty() && !parts[0].empty()) {
                    int idx = 0;
                    if (str::parseInt(parts[0], idx)) {
                        // OBJ indices are 1-based, negative means from end
                        if (idx > 0) {
                            key.pos = idx - 1;
                        } else if (idx < 0) {
                            key.pos = static_cast<int>(positions.size()) + idx;
                        }
                    }
                }

                if (parts.size() > 1 && !parts[1].empty()) {
                    int idx = 0;
                    if (str::parseInt(parts[1], idx)) {
                        if (idx > 0) {
                            key.tex = idx - 1;
                        } else if (idx < 0) {
                            key.tex = static_cast<int>(texCoords.size()) + idx;
                        }
                    }
                }

                if (parts.size() > 2 && !parts[2].empty()) {
                    int idx = 0;
                    if (str::parseInt(parts[2], idx)) {
                        if (idx > 0) {
                            key.norm = idx - 1;
                        } else if (idx < 0) {
                            key.norm = static_cast<int>(normals.size()) + idx;
                        }
                    }
                }

                faceIndices.push_back(getOrCreateVertex(key));
            }

            // Triangulate polygon (fan triangulation)
            for (usize i = 2; i < faceIndices.size(); ++i) {
                mesh->addTriangle(faceIndices[0], faceIndices[i - 1], faceIndices[i]);
            }
        }
        // Ignore: mtllib, usemtl, g, o, s, etc.
    }

    if (mesh->triangleCount() == 0) {
        return LoadResult{nullptr, "No faces found in OBJ file"};
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

}  // namespace dw
