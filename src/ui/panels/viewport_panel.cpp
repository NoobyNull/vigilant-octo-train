#include "viewport_panel.h"

#include "../../core/mesh/mesh.h"

#include <imgui.h>

namespace dw {

ViewportPanel::ViewportPanel() : Panel("Viewport") {
    m_renderer.initialize();
    m_camera.reset();
}

void ViewportPanel::render() {
    if (!m_open) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin(m_title.c_str(), &m_open,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        renderToolbar();
        renderViewport();
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void ViewportPanel::setMesh(MeshPtr mesh) {
    m_mesh = mesh;

    if (m_gpuMesh.vao != 0) {
        m_gpuMesh.destroy();
    }

    if (m_mesh && m_mesh->isValid()) {
        m_gpuMesh = m_renderer.uploadMesh(*m_mesh);
        fitToModel();
    }
}

void ViewportPanel::clearMesh() {
    m_mesh = nullptr;
    if (m_gpuMesh.vao != 0) {
        m_gpuMesh.destroy();
    }
}

void ViewportPanel::resetView() {
    m_camera.reset();
}

void ViewportPanel::fitToModel() {
    if (m_mesh && m_mesh->isValid()) {
        const auto& bounds = m_mesh->bounds();
        m_camera.fitToBounds(bounds.min, bounds.max);
    }
}

void ViewportPanel::handleInput() {
    if (!ImGui::IsWindowHovered()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Mouse wheel zoom
    if (io.MouseWheel != 0.0f) {
        m_camera.zoom(io.MouseWheel * 0.5f);
    }

    // Mouse drag for orbit/pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = io.MouseDelta;

        if (io.KeyShift) {
            // Pan
            m_camera.pan(-delta.x, delta.y);
        } else {
            // Orbit
            m_camera.orbit(-delta.x, -delta.y);
        }
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        m_camera.pan(-delta.x, delta.y);
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 delta = io.MouseDelta;
        m_camera.zoom(delta.y * 0.01f);
    }
}

void ViewportPanel::renderViewport() {
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    int width = static_cast<int>(contentSize.x);
    int height = static_cast<int>(contentSize.y);

    if (width <= 0 || height <= 0) {
        return;
    }

    // Resize framebuffer if needed
    if (width != m_viewportWidth || height != m_viewportHeight) {
        m_viewportWidth = width;
        m_viewportHeight = height;
        m_framebuffer.resize(width, height);
        m_camera.setViewport(width, height);
    }

    // Handle input
    handleInput();

    // Render to framebuffer
    m_framebuffer.bind();

    m_renderer.beginFrame(Color{0.15f, 0.16f, 0.17f, 1.0f});
    m_renderer.setCamera(m_camera);

    // Render grid
    m_renderer.renderGrid();

    // Render mesh
    if (m_gpuMesh.vao != 0) {
        m_renderer.renderMesh(m_gpuMesh);
    }

    // Render axis
    m_renderer.renderAxis();

    m_renderer.endFrame();
    m_framebuffer.unbind();

    // Display framebuffer texture
    ImGui::Image(static_cast<ImTextureID>(m_framebuffer.colorTexture()),
                 contentSize, ImVec2(0, 1), ImVec2(1, 0));
}

void ViewportPanel::renderToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    if (ImGui::Button("Reset")) {
        resetView();
    }
    ImGui::SameLine();

    if (ImGui::Button("Fit")) {
        fitToModel();
    }
    ImGui::SameLine();

    ImGui::Separator();
    ImGui::SameLine();

    bool wireframe = m_renderer.settings().wireframe;
    if (ImGui::Checkbox("Wireframe", &wireframe)) {
        m_renderer.settings().wireframe = wireframe;
    }
    ImGui::SameLine();

    bool grid = m_renderer.settings().showGrid;
    if (ImGui::Checkbox("Grid", &grid)) {
        m_renderer.settings().showGrid = grid;
    }
    ImGui::SameLine();

    bool axis = m_renderer.settings().showAxis;
    if (ImGui::Checkbox("Axis", &axis)) {
        m_renderer.settings().showAxis = axis;
    }

    ImGui::PopStyleVar(2);
}

}  // namespace dw
