#pragma once

#include <array>
#include <memory>
#include <optional>

#include <imgui.h>

#include "../../core/database/model_repository.h"
#include "../../render/camera.h"
#include "../../render/framebuffer.h"
#include "../../render/renderer.h"
#include "../../render/texture.h"
#include "panel.h"

namespace dw {

// Forward declarations
class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;
class ContextMenuManager;

// 3D viewport panel
class ViewportPanel : public Panel {
  public:
    ViewportPanel();
    ~ViewportPanel() override = default;

    void render() override;

    // Set mesh to display
    void setMesh(MeshPtr mesh);
    // Set mesh that was already auto-oriented on worker thread
    void setPreOrientedMesh(MeshPtr mesh,
                            f32 orientYaw,
                            std::optional<CameraState> savedCamera = std::nullopt);
    void clearMesh();

    // Set toolpath mesh to display
    void setToolpathMesh(MeshPtr toolpathMesh);
    void clearToolpathMesh();

    // Set active material texture for rendering (nullptr = solid color)
    void setMaterialTexture(const Texture* texture) { m_materialTexture = texture; }

    // Camera access
    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    // Camera state persistence
    CameraState getCameraState() const;
    void restoreCameraState(const CameraState& state);

    // Render settings
    RenderSettings& renderSettings() { return m_renderer.settings(); }

    // Actions
    void resetView();
    void fitToModel();

    // Context menu manager integration
    void setContextMenuManager(ContextMenuManager* manager) { m_contextMenuManager = manager; }

  private:
    void registerContextMenuEntries();
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

    // Active material texture (not owned — pointer to Application-managed texture)
    const Texture* m_materialTexture = nullptr;

    // Light drag state
    bool m_lightDirDragging = false;
    bool m_lightIntensityDragging = false;

    // Viewport size
    int m_viewportWidth = 1;
    int m_viewportHeight = 1;

    // ViewCube cache
    ViewCubeCache m_viewCubeCache;

    // Context menu manager (not owned — managed by UIManager)
    ContextMenuManager* m_contextMenuManager = nullptr;
};

} // namespace dw
