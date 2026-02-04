#include "renderer.h"

#include "../core/utils/log.h"
#include "gl_utils.h"
#include "shader_sources.h"

#include <vector>

namespace dw {

void GPUMesh::destroy() {
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    indexCount = 0;
}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (m_initialized) {
        return true;
    }

    if (!createShaders()) {
        return false;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable back-face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Enable blending for grid fade
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create helper meshes
    createGridMesh(20.0f, 1.0f);
    createAxisMesh(2.0f);

    m_initialized = true;
    log::info("Renderer initialized");
    return true;
}

void Renderer::shutdown() {
    m_gridMesh.destroy();
    m_axisMesh.destroy();
    m_initialized = false;
}

void Renderer::beginFrame(const Color& clearColor) {
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    // Nothing special needed
}

void Renderer::setCamera(const Camera& camera) {
    m_camera = camera;
}

void Renderer::renderMesh(const Mesh& mesh, const Mat4& modelMatrix) {
    // Upload mesh if not cached (simple approach - upload every frame)
    GPUMesh gpuMesh = uploadMesh(mesh);
    renderMesh(gpuMesh, modelMatrix);
    gpuMesh.destroy();
}

void Renderer::renderMesh(const GPUMesh& gpuMesh, const Mat4& modelMatrix) {
    if (gpuMesh.vao == 0 || gpuMesh.indexCount == 0) {
        return;
    }

    if (m_settings.wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    m_meshShader.bind();

    // Set uniforms
    m_meshShader.setMat4("uModel", modelMatrix);
    m_meshShader.setMat4("uView", m_camera.viewMatrix());
    m_meshShader.setMat4("uProjection", m_camera.projectionMatrix());

    // Calculate normal matrix (inverse transpose of model matrix for lighting)
    // For simplicity, assuming uniform scaling
    m_meshShader.setMat4("uNormalMatrix", modelMatrix);

    m_meshShader.setVec3("uLightDir", m_settings.lightDir);
    m_meshShader.setVec3("uLightColor", m_settings.lightColor);
    m_meshShader.setVec3("uAmbient", m_settings.ambient);
    m_meshShader.setVec3("uObjectColor",
                         Vec3{m_settings.objectColor.r, m_settings.objectColor.g,
                              m_settings.objectColor.b});
    m_meshShader.setVec3("uViewPos", m_camera.position());
    m_meshShader.setFloat("uShininess", m_settings.shininess);

    glBindVertexArray(gpuMesh.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(gpuMesh.indexCount),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    if (m_settings.wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::renderGrid(f32 size, f32 spacing) {
    if (!m_settings.showGrid || m_gridMesh.vao == 0) {
        return;
    }

    glDisable(GL_CULL_FACE);

    m_gridShader.bind();
    m_gridShader.setMat4("uMVP", m_camera.viewProjectionMatrix());
    m_gridShader.setVec4("uColor", Vec4{0.5f, 0.5f, 0.5f, 0.5f});
    m_gridShader.setFloat("uFadeStart", size * 0.5f);
    m_gridShader.setFloat("uFadeEnd", size);

    glBindVertexArray(m_gridMesh.vao);
    glDrawElements(GL_LINES, static_cast<GLsizei>(m_gridMesh.indexCount),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
}

void Renderer::renderAxis(f32 length) {
    if (!m_settings.showAxis || m_axisMesh.vao == 0) {
        return;
    }

    glDisable(GL_DEPTH_TEST);

    m_flatShader.bind();
    m_flatShader.setMat4("uMVP", m_camera.viewProjectionMatrix());

    glBindVertexArray(m_axisMesh.vao);

    // X axis (red)
    m_flatShader.setVec4("uColor", Vec4{1.0f, 0.0f, 0.0f, 1.0f});
    glDrawArrays(GL_LINES, 0, 2);

    // Y axis (green)
    m_flatShader.setVec4("uColor", Vec4{0.0f, 1.0f, 0.0f, 1.0f});
    glDrawArrays(GL_LINES, 2, 2);

    // Z axis (blue)
    m_flatShader.setVec4("uColor", Vec4{0.0f, 0.0f, 1.0f, 1.0f});
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

GPUMesh Renderer::uploadMesh(const Mesh& mesh) {
    GPUMesh gpuMesh;

    if (mesh.vertices().empty() || mesh.indices().empty()) {
        return gpuMesh;
    }

    glGenVertexArrays(1, &gpuMesh.vao);
    glGenBuffers(1, &gpuMesh.vbo);
    glGenBuffers(1, &gpuMesh.ebo);

    glBindVertexArray(gpuMesh.vao);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices().size() * sizeof(Vertex)),
                 mesh.vertices().data(), GL_STATIC_DRAW);

    // Upload indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices().size() * sizeof(u32)),
                 mesh.indices().data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    gpuMesh.indexCount = mesh.indexCount();

    return gpuMesh;
}

bool Renderer::createShaders() {
    if (!m_meshShader.compile(shaders::MESH_VERTEX, shaders::MESH_FRAGMENT)) {
        log::error("Failed to compile mesh shader");
        return false;
    }

    if (!m_flatShader.compile(shaders::FLAT_VERTEX, shaders::FLAT_FRAGMENT)) {
        log::error("Failed to compile flat shader");
        return false;
    }

    if (!m_gridShader.compile(shaders::GRID_VERTEX, shaders::GRID_FRAGMENT)) {
        log::error("Failed to compile grid shader");
        return false;
    }

    return true;
}

void Renderer::createGridMesh(f32 size, f32 spacing) {
    std::vector<f32> vertices;
    std::vector<u32> indices;

    int halfLines = static_cast<int>(size / spacing);
    u32 idx = 0;

    for (int i = -halfLines; i <= halfLines; ++i) {
        f32 pos = static_cast<f32>(i) * spacing;

        // X parallel lines
        vertices.insert(vertices.end(), {-size, 0.0f, pos});
        vertices.insert(vertices.end(), {size, 0.0f, pos});
        indices.push_back(idx++);
        indices.push_back(idx++);

        // Z parallel lines
        vertices.insert(vertices.end(), {pos, 0.0f, -size});
        vertices.insert(vertices.end(), {pos, 0.0f, size});
        indices.push_back(idx++);
        indices.push_back(idx++);
    }

    glGenVertexArrays(1, &m_gridMesh.vao);
    glGenBuffers(1, &m_gridMesh.vbo);
    glGenBuffers(1, &m_gridMesh.ebo);

    glBindVertexArray(m_gridMesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_gridMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(f32)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridMesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(u32)), indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    m_gridMesh.indexCount = static_cast<u32>(indices.size());
}

void Renderer::createAxisMesh(f32 length) {
    f32 vertices[] = {
        // X axis
        0.0f, 0.0f, 0.0f,
        length, 0.0f, 0.0f,
        // Y axis
        0.0f, 0.0f, 0.0f,
        0.0f, length, 0.0f,
        // Z axis
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, length,
    };

    glGenVertexArrays(1, &m_axisMesh.vao);
    glGenBuffers(1, &m_axisMesh.vbo);

    glBindVertexArray(m_axisMesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_axisMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    m_axisMesh.indexCount = 6;
}

}  // namespace dw
