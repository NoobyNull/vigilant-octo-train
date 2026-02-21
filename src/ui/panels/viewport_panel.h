#pragma once

#include <array>
#include <memory>

#include <imgui.h>

#include "../../render/camera.h"
#include "../../render/framebuffer.h"
#include "../../render/renderer.h"
#include "panel.h"

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

    // Set toolpath mesh to display
    void setToolpathMesh(MeshPtr toolpathMesh);
    void clearToolpathMesh();

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
    void renderViewCube();

    // ViewCube geometry cache — invalidated when camera orientation changes
    struct ViewCubeCache {
        f32 lastYaw = -999.0f;
        f32 lastPitch = -999.0f;
        std::array<ImVec2, 8> projectedVerts{};
        std::array<f32, 8> depths{};
        struct FaceSort {
            int index;
            f32 avgZ;
        };
        std::array<FaceSort, 6> sortedFaces{};
        bool valid = false;
    };

    Renderer m_renderer;
    Camera m_camera;
    Framebuffer m_framebuffer;

    MeshPtr m_mesh;
    GPUMesh m_gpuMesh;

    MeshPtr m_toolpathMesh;
    GPUMesh m_gpuToolpath;

    // Light drag state
    bool m_lightDirDragging = false;
    bool m_lightIntensityDragging = false;

    // Viewport size
    int m_viewportWidth = 1;
    int m_viewportHeight = 1;

    // ViewCube cache
    ViewCubeCache m_viewCubeCache;
};

} // namespace dw
