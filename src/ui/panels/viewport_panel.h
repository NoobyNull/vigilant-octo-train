#pragma once

#include "../../render/camera.h"
#include "../../render/framebuffer.h"
#include "../../render/renderer.h"
#include "panel.h"

#include <memory>

namespace dw {

// Forward declarations
class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

// 3D viewport panel
class ViewportPanel : public Panel {
public:
    ViewportPanel();
    ~ViewportPanel() override = default;

    void render() override;

    // Set mesh to display
    void setMesh(MeshPtr mesh);
    void clearMesh();

    // Camera access
    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    // Render settings
    RenderSettings& renderSettings() { return m_renderer.settings(); }

    // Actions
    void resetView();
    void fitToModel();

private:
    void handleInput();
    void renderViewport();
    void renderToolbar();

    Renderer m_renderer;
    Camera m_camera;
    Framebuffer m_framebuffer;

    MeshPtr m_mesh;
    GPUMesh m_gpuMesh;

    // Input state
    bool m_isDragging = false;
    bool m_isPanning = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;

    // Viewport size
    int m_viewportWidth = 1;
    int m_viewportHeight = 1;
};

}  // namespace dw
