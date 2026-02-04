#pragma once

#include "../types.h"
#include "bounds.h"
#include "vertex.h"

#include <memory>
#include <string>
#include <vector>

namespace dw {

// 3D mesh class
class Mesh {
public:
    Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<u32> indices);

    // Accessors
    const std::vector<Vertex>& vertices() const { return m_vertices; }
    const std::vector<u32>& indices() const { return m_indices; }
    const AABB& bounds() const { return m_bounds; }

    std::vector<Vertex>& vertices() { return m_vertices; }
    std::vector<u32>& indices() { return m_indices; }

    // Statistics
    u32 vertexCount() const { return static_cast<u32>(m_vertices.size()); }
    u32 triangleCount() const { return static_cast<u32>(m_indices.size() / 3); }
    u32 indexCount() const { return static_cast<u32>(m_indices.size()); }

    // Metadata
    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Operations
    void clear();
    void recalculateBounds();
    void recalculateNormals();
    void transform(const Mat4& matrix);
    void centerOnOrigin();
    void normalizeSize(f32 targetSize = 1.0f);

    // Merge another mesh into this one
    void merge(const Mesh& other);

    // Create a copy
    Mesh clone() const;

    // Check if mesh has valid data
    bool isValid() const;
    bool hasNormals() const;
    bool hasTexCoords() const;

    // Reserve memory
    void reserve(u32 vertexCount, u32 indexCount);

    // Add geometry
    void addVertex(const Vertex& vertex);
    void addTriangle(u32 v0, u32 v1, u32 v2);

private:
    std::vector<Vertex> m_vertices;
    std::vector<u32> m_indices;
    AABB m_bounds;
    std::string m_name;
};

// Shared mesh pointer type
using MeshPtr = std::shared_ptr<Mesh>;

}  // namespace dw
