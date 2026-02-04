#include "mesh.h"

#include <cmath>

namespace dw {

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<u32> indices)
    : m_vertices(std::move(vertices)), m_indices(std::move(indices)) {
    recalculateBounds();
}

void Mesh::clear() {
    m_vertices.clear();
    m_indices.clear();
    m_bounds.reset();
    m_name.clear();
}

void Mesh::recalculateBounds() {
    m_bounds.reset();
    for (const auto& vertex : m_vertices) {
        m_bounds.expand(vertex.position);
    }
}

void Mesh::recalculateNormals() {
    // Reset all normals to zero
    for (auto& vertex : m_vertices) {
        vertex.normal = Vec3{0.0f, 0.0f, 0.0f};
    }

    // Accumulate face normals for each vertex
    for (usize i = 0; i + 2 < m_indices.size(); i += 3) {
        u32 i0 = m_indices[i];
        u32 i1 = m_indices[i + 1];
        u32 i2 = m_indices[i + 2];

        const Vec3& v0 = m_vertices[i0].position;
        const Vec3& v1 = m_vertices[i1].position;
        const Vec3& v2 = m_vertices[i2].position;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.cross(edge2);

        m_vertices[i0].normal = m_vertices[i0].normal + normal;
        m_vertices[i1].normal = m_vertices[i1].normal + normal;
        m_vertices[i2].normal = m_vertices[i2].normal + normal;
    }

    // Normalize all normals
    for (auto& vertex : m_vertices) {
        vertex.normal = vertex.normal.normalized();
    }
}

void Mesh::transform(const Mat4& matrix) {
    // Normal matrix (inverse transpose of upper 3x3)
    // For simple transforms, we can use the matrix directly
    for (auto& vertex : m_vertices) {
        Vec4 pos = matrix * Vec4(vertex.position, 1.0f);
        vertex.position = Vec3{pos.x, pos.y, pos.z};

        // Transform normal (assuming no non-uniform scaling)
        Vec4 norm = matrix * Vec4(vertex.normal, 0.0f);
        vertex.normal = Vec3{norm.x, norm.y, norm.z}.normalized();
    }

    recalculateBounds();
}

void Mesh::centerOnOrigin() {
    Vec3 center = m_bounds.center();

    for (auto& vertex : m_vertices) {
        vertex.position = vertex.position - center;
    }

    recalculateBounds();
}

void Mesh::normalizeSize(f32 targetSize) {
    f32 maxExtent = m_bounds.maxExtent();
    if (maxExtent > 0.0f) {
        f32 scale = targetSize / maxExtent;
        Mat4 scaleMatrix = Mat4::scale(Vec3{scale, scale, scale});
        transform(scaleMatrix);
    }
}

void Mesh::merge(const Mesh& other) {
    u32 vertexOffset = static_cast<u32>(m_vertices.size());

    // Add vertices
    m_vertices.insert(m_vertices.end(), other.m_vertices.begin(),
                      other.m_vertices.end());

    // Add indices with offset
    for (u32 index : other.m_indices) {
        m_indices.push_back(index + vertexOffset);
    }

    // Update bounds
    m_bounds.expand(other.m_bounds);
}

Mesh Mesh::clone() const {
    Mesh copy;
    copy.m_vertices = m_vertices;
    copy.m_indices = m_indices;
    copy.m_bounds = m_bounds;
    copy.m_name = m_name;
    return copy;
}

bool Mesh::isValid() const {
    return !m_vertices.empty() && !m_indices.empty() && m_indices.size() % 3 == 0;
}

bool Mesh::hasNormals() const {
    if (m_vertices.empty()) {
        return false;
    }

    // Check if any normal is non-zero
    for (const auto& vertex : m_vertices) {
        if (vertex.normal.x != 0.0f || vertex.normal.y != 0.0f ||
            vertex.normal.z != 0.0f) {
            return true;
        }
    }
    return false;
}

bool Mesh::hasTexCoords() const {
    if (m_vertices.empty()) {
        return false;
    }

    // Check if any texcoord is non-zero
    for (const auto& vertex : m_vertices) {
        if (vertex.texCoord.x != 0.0f || vertex.texCoord.y != 0.0f) {
            return true;
        }
    }
    return false;
}

void Mesh::reserve(u32 vertexCount, u32 indexCount) {
    m_vertices.reserve(vertexCount);
    m_indices.reserve(indexCount);
}

void Mesh::addVertex(const Vertex& vertex) {
    m_vertices.push_back(vertex);
    m_bounds.expand(vertex.position);
}

void Mesh::addTriangle(u32 v0, u32 v1, u32 v2) {
    m_indices.push_back(v0);
    m_indices.push_back(v1);
    m_indices.push_back(v2);
}

}  // namespace dw
