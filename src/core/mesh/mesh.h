#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../types.h"
#include "bounds.h"
#include "vertex.h"

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

    // Auto-orient for relief models: permute axes to canonical
    // (width=X, height=Y, depth=Z), return camera yaw for front face.
    // Stores the permutation matrix so it can be reverted.
    f32 autoOrient();
    void revertAutoOrient();
    bool wasAutoOriented() const { return m_autoOriented; }
    const Mat4& getOrientMatrix() const { return m_orientMatrix; }

    // Apply a previously computed orient matrix (fast path — skips axis detection and normal
    // counting)
    void applyStoredOrient(const Mat4& matrix);

    // Create a copy
    Mesh clone() const;

    // UV coordinate generation
    // Returns true if all vertices have zero texCoords (typical for STL imports)
    bool needsUVGeneration() const;
    // Generate planar UV coordinates from mesh bounds.
    // Projects onto the plane with largest area (XY, XZ, or YZ).
    // grainRotationDeg rotates UV around (0.5,0.5) for wood grain direction.
    void generatePlanarUVs(float grainRotationDeg = 0.0f);

    // Check if mesh has valid data
    bool isValid() const;
    bool hasNormals() const;
    bool hasTexCoords() const;

    // Validate mesh integrity (checks for NaN, out-of-bounds indices, degenerate triangles)
    // Returns true if mesh passes all checks. Logs warnings for issues found.
    bool validate() const;

    // Validate only fatal geometry issues (NaN/Inf positions, out-of-bounds indices).
    // Degenerate triangles are tolerated (common in real-world meshes).
    // Returns true if mesh can be safely rendered.
    bool validateGeometry() const;

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

    Mat4 m_orientMatrix; // Permutation applied by autoOrient()
    bool m_autoOriented = false;
};

// Shared mesh pointer type
using MeshPtr = std::shared_ptr<Mesh>;

} // namespace dw
