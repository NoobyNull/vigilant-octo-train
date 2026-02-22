#pragma once

#include <memory>
#include <unordered_map>

#include <glad/gl.h>

#include "../core/mesh/mesh.h"
#include "../core/types.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"

namespace dw {

// GPU mesh data
struct GPUMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    u32 indexCount = 0;

    void destroy();
};

// Render settings
struct RenderSettings {
    Vec3 lightDir{-0.5f, -1.0f, -0.3f};
    Vec3 lightColor{1.0f, 1.0f, 1.0f};
    Vec3 ambient{0.2f, 0.2f, 0.2f};
    Color objectColor = Color::fromHex(0x6699CC);
    f32 shininess = 32.0f;
    bool wireframe = false;
    bool showGrid = true;
    bool showAxis = true;
};

// Main renderer class
class Renderer {
  public:
    Renderer() = default;
    ~Renderer();

    // Initialize/shutdown
    bool initialize();
    void shutdown();

    // Begin/end frame
    void beginFrame(const Color& clearColor = Color{0.15f, 0.15f, 0.15f, 1.0f});
    void endFrame();

    // Set camera for rendering
    void setCamera(const Camera& camera);

    // Render a mesh (solid color fallback)
    void renderMesh(const Mesh& mesh, const Mat4& modelMatrix = Mat4(1.0f));
    void renderMesh(const GPUMesh& gpuMesh, const Mat4& modelMatrix = Mat4(1.0f));

    // Render a mesh with an optional material texture.
    // Pass nullptr for materialTexture to use solid color (same as above overloads).
    void renderMesh(const Mesh& mesh,
                    const Texture* materialTexture,
                    const Mat4& modelMatrix = Mat4(1.0f));
    void renderMesh(const GPUMesh& gpuMesh,
                    const Texture* materialTexture,
                    const Mat4& modelMatrix = Mat4(1.0f));

    // Render a toolpath mesh with color distinction for rapid vs cutting moves
    void renderToolpath(const Mesh& toolpathMesh, const Mat4& modelMatrix = Mat4(1.0f));

    // Render grid
    void renderGrid(f32 size = 10.0f, f32 spacing = 1.0f);

    // Render axis indicator
    void renderAxis(f32 length = 1.0f);

    // Upload mesh to GPU
    GPUMesh uploadMesh(const Mesh& mesh);

    // Mesh cache management
    void clearMeshCache();

    // Settings
    RenderSettings& settings() { return m_settings; }
    const RenderSettings& settings() const { return m_settings; }

    // Check if initialized
    bool isInitialized() const { return m_initialized; }

  private:
    bool createShaders();
    void createGridMesh(f32 size, f32 spacing);
    void createAxisMesh(f32 length);

    bool m_initialized = false;
    Camera m_camera;

    Shader m_meshShader;
    Shader m_flatShader;
    Shader m_gridShader;

    GPUMesh m_gridMesh;
    GPUMesh m_axisMesh;

    RenderSettings m_settings;

    // Mesh cache: hash -> uploaded GPU mesh
    std::unordered_map<u64, GPUMesh> m_meshCache;
};

} // namespace dw
