#include "viewport_panel.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/config/input_binding.h"
#include "../../core/mesh/mesh.h"

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
        f32 yaw = 0.0f;
        if (Config::instance().getAutoOrient()) {
            yaw = m_mesh->autoOrient();
        }

        m_gpuMesh = m_renderer.uploadMesh(*m_mesh);
        fitToModel();

        if (Config::instance().getAutoOrient()) {
            m_camera.setYaw(yaw);
            m_camera.setPitch(0.0f);
        }
    }

    // Invalidate ViewCube cache (camera may have changed)
    m_viewCubeCache.valid = false;
}

void ViewportPanel::setPreOrientedMesh(MeshPtr mesh, f32 orientYaw) {
    m_mesh = mesh;

    if (m_gpuMesh.vao != 0) {
        m_gpuMesh.destroy();
    }

    if (m_mesh && m_mesh->isValid()) {
        m_gpuMesh = m_renderer.uploadMesh(*m_mesh);
        fitToModel();

        if (m_mesh->wasAutoOriented()) {
            m_camera.setYaw(orientYaw);
            m_camera.setPitch(0.0f);
        }
    }

    m_viewCubeCache.valid = false;
}

void ViewportPanel::clearMesh() {
    m_mesh = nullptr;
    if (m_gpuMesh.vao != 0) {
        m_gpuMesh.destroy();
    }

    // Invalidate ViewCube cache
    m_viewCubeCache.valid = false;
}

void ViewportPanel::setToolpathMesh(MeshPtr toolpathMesh) {
    m_toolpathMesh = toolpathMesh;

    if (m_gpuToolpath.vao != 0) {
        m_gpuToolpath.destroy();
    }

    if (m_toolpathMesh && m_toolpathMesh->isValid()) {
        m_gpuToolpath = m_renderer.uploadMesh(*m_toolpathMesh);

        // Auto-fit camera to toolpath bounds if no mesh is currently displayed
        if (!m_mesh) {
            const auto& bounds = m_toolpathMesh->bounds();
            m_camera.fitToBounds(bounds.min, bounds.max);
            m_viewCubeCache.valid = false;
        }
    }
}

void ViewportPanel::clearToolpathMesh() {
    m_toolpathMesh = nullptr;
    if (m_gpuToolpath.vao != 0) {
        m_gpuToolpath.destroy();
    }
}

void ViewportPanel::resetView() {
    m_camera.reset();

    // Reset light to defaults
    auto& rs = m_renderer.settings();
    rs.lightDir = Vec3{-0.5f, -1.0f, -0.3f};
    rs.lightColor = Vec3{1.0f, 1.0f, 1.0f};

    // Persist reset light settings
    auto& cfg = Config::instance();
    cfg.setRenderLightDir(rs.lightDir);
    cfg.setRenderLightColor(rs.lightColor);
    cfg.save();

    // Invalidate ViewCube cache (camera changed)
    m_viewCubeCache.valid = false;
}

void ViewportPanel::fitToModel() {
    if (m_mesh && m_mesh->isValid()) {
        const auto& bounds = m_mesh->bounds();
        m_camera.fitToBounds(bounds.min, bounds.max);
    }

    // Invalidate ViewCube cache (camera may have changed)
    m_viewCubeCache.valid = false;
}

void ViewportPanel::handleInput() {
    if (!ImGui::IsWindowHovered()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    auto& cfg = Config::instance();
    NavStyle nav = cfg.getNavStyle();
    f32 orbitSignX = cfg.getInvertOrbitX() ? 1.0f : -1.0f;
    f32 orbitSignY = cfg.getInvertOrbitY() ? 1.0f : -1.0f;

    // Mouse wheel zoom (all styles)
    if (io.MouseWheel != 0.0f) {
        m_camera.zoom(io.MouseWheel * 0.5f);
    }

    // --- Configurable light controls ---
    InputBinding lightDirBind = cfg.getBinding(BindAction::LightDirDrag);
    InputBinding lightIntBind = cfg.getBinding(BindAction::LightIntensityDrag);

    constexpr f32 kPi = 3.14159265f;
    constexpr f32 kDeg2Rad = kPi / 180.0f;
    constexpr f32 kRad2Deg = 180.0f / kPi;
    constexpr f32 kDragSensitivity = 0.5f; // degrees per pixel

    // Light direction drag
    if (lightDirBind.isValid() && lightDirBind.isHeld()) {
        if (!m_lightDirDragging) {
            m_lightDirDragging = true;
        }
        ImVec2 delta = io.MouseDelta;
        if (delta.x != 0.0f || delta.y != 0.0f) {
            auto& dir = m_renderer.settings().lightDir;
            Vec2 spherical = toSpherical(dir);
            f32 azimuth = spherical.x * kRad2Deg;
            f32 elevation = spherical.y * kRad2Deg;

            azimuth += delta.x * kDragSensitivity;
            elevation += delta.y * kDragSensitivity;
            elevation = std::clamp(elevation, -89.0f, 89.0f);

            dir = fromSpherical(azimuth * kDeg2Rad, elevation * kDeg2Rad);
        }
        return; // consume input
    }
    if (m_lightDirDragging) {
        m_lightDirDragging = false;
        cfg.setRenderLightDir(m_renderer.settings().lightDir);
        cfg.save();
    }

    // Light intensity drag
    if (lightIntBind.isValid() && lightIntBind.isHeld()) {
        if (!m_lightIntensityDragging) {
            m_lightIntensityDragging = true;
        }
        ImVec2 delta = io.MouseDelta;
        if (delta.y != 0.0f) {
            auto& col = m_renderer.settings().lightColor;
            f32 maxC = std::max({col.x, col.y, col.z, 0.001f});
            f32 intensity = maxC - delta.y * 0.005f;
            intensity = std::clamp(intensity, 0.1f, 3.0f);
            f32 scale = intensity / maxC;
            col.x *= scale;
            col.y *= scale;
            col.z *= scale;
        }
        return; // consume input
    }
    if (m_lightIntensityDragging) {
        m_lightIntensityDragging = false;
        cfg.setRenderLightColor(m_renderer.settings().lightColor);
        cfg.save();
    }

    switch (nav) {
    case NavStyle::CAD:
        // Middle=Orbit, Shift+Middle=Pan, Right=Pan, Scroll=Zoom
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = io.MouseDelta;
            if (io.KeyShift) {
                m_camera.pan(-delta.x, delta.y);
            } else {
                m_camera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
            }
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 delta = io.MouseDelta;
            m_camera.pan(-delta.x, delta.y);
        }
        break;

    case NavStyle::Maya:
        // Alt+Left=Orbit, Alt+Middle=Pan, Alt+Right=Zoom
        if (io.KeyAlt) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = io.MouseDelta;
                m_camera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
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
        break;

    default: // NavStyle::Default
        // Left=Orbit, Shift+Left=Pan, Middle=Pan, Right=Zoom
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = io.MouseDelta;
            if (io.KeyShift) {
                m_camera.pan(-delta.x, delta.y);
            } else {
                m_camera.orbit(orbitSignX * delta.x, orbitSignY * delta.y);
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
        break;
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

    // Render grid and axis (if enabled in settings)
    m_renderer.renderGrid(20.0f, 1.0f);
    m_renderer.renderAxis(2.0f);

    // Render mesh (with material texture if assigned)
    if (m_gpuMesh.vao != 0) {
        m_renderer.renderMesh(m_gpuMesh, m_materialTexture);
    }

    // Render toolpath (if present)
    if (m_gpuToolpath.vao != 0) {
        m_renderer.renderToolpath(*m_toolpathMesh);
    }

    m_renderer.endFrame();
    m_framebuffer.unbind();

    // Display framebuffer texture
    ImGui::Image(static_cast<ImTextureID>(m_framebuffer.colorTexture()), contentSize, ImVec2(0, 1),
                 ImVec2(1, 0));

    // Context menu on right-click (only when not dragging)
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        auto& cfg = Config::instance();
        m_contextMenu.clear();
        m_contextMenu.addItem("Reset View", [this]() { resetView(); });
        m_contextMenu.addItem("Fit to Model", [this]() { fitToModel(); }, m_mesh != nullptr);
        m_contextMenu.addSeparator();
        m_contextMenu.addToggle("Invert Orbit X", cfg.getInvertOrbitX(), [&cfg]() {
            cfg.setInvertOrbitX(!cfg.getInvertOrbitX());
            cfg.save();
        });
        m_contextMenu.addToggle("Invert Orbit Y", cfg.getInvertOrbitY(), [&cfg]() {
            cfg.setInvertOrbitY(!cfg.getInvertOrbitY());
            cfg.save();
        });
        m_contextMenu.addSeparator();
        m_contextMenu.addToggle("Show Grid", cfg.getShowGrid(), [&cfg]() {
            cfg.setShowGrid(!cfg.getShowGrid());
            cfg.save();
        });
        m_contextMenu.addToggle("Show Axis", cfg.getShowAxis(), [&cfg]() {
            cfg.setShowAxis(!cfg.getShowAxis());
            cfg.save();
        });
        m_contextMenu.open();
    }
    m_contextMenu.render();

    renderViewCube();
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

    // Wrap Wireframe checkbox to new line if too narrow
    if (ImGui::GetContentRegionAvail().x < 100.0f) {
        ImGui::NewLine();
    }

    bool wireframe = m_renderer.settings().wireframe;
    if (ImGui::Checkbox("Wireframe", &wireframe)) {
        m_renderer.settings().wireframe = wireframe;
    }

    ImGui::PopStyleVar(2);
}

void ViewportPanel::renderViewCube() {
    constexpr f32 kCubeSize = 30.0f;
    constexpr f32 kMargin = 15.0f;
    constexpr f32 kDeg2Rad = 3.14159265f / 180.0f;
    constexpr f32 kEpsilon = 0.001f;

    // Cube vertices: unit cube from -1 to 1
    const std::array<Vec3, 8> verts = {{
        {-1, -1, -1},
        {1, -1, -1},
        {1, 1, -1},
        {-1, 1, -1},
        {-1, -1, 1},
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1},
    }};

    // Face definitions: 4 vertex indices, label, target yaw, target pitch
    struct Face {
        int v[4];
        const char* label;
        f32 yaw;
        f32 pitch;
    };

    const std::array<Face, 6> faces = {{
        {{0, 1, 2, 3}, "F", 0.0f, 0.0f},    // Front (-Z)
        {{5, 4, 7, 6}, "Bk", 180.0f, 0.0f}, // Back (+Z)
        {{1, 5, 6, 2}, "R", 90.0f, 0.0f},   // Right (+X)
        {{4, 0, 3, 7}, "L", 270.0f, 0.0f},  // Left (-X)
        {{3, 2, 6, 7}, "T", 0.0f, 89.0f},   // Top (+Y)
        {{4, 5, 1, 0}, "Bt", 0.0f, -89.0f}, // Bottom (-Y)
    }};

    // Face colors (RGBA)
    constexpr ImU32 kFaceColor = IM_COL32(70, 75, 85, 220);
    constexpr ImU32 kFaceHover = IM_COL32(100, 110, 130, 240);
    constexpr ImU32 kEdgeColor = IM_COL32(40, 42, 48, 255);
    constexpr ImU32 kLabelColor = IM_COL32(200, 210, 225, 255);
    constexpr ImU32 kFrontFace = IM_COL32(85, 95, 110, 230);

    // Get viewport image rect (from previous ImGui::Image call)
    ImVec2 rectMin = ImGui::GetItemRectMin();
    ImVec2 rectMax = ImGui::GetItemRectMax();

    // Cube origin: top-right corner
    ImVec2 origin = {rectMax.x - kMargin - kCubeSize, rectMin.y + kMargin + kCubeSize};

    // --- Cache check: skip geometry recomputation if camera unchanged ---
    f32 yaw = m_camera.yaw();
    f32 pitch = m_camera.pitch();

    if (!m_viewCubeCache.valid || std::abs(yaw - m_viewCubeCache.lastYaw) > kEpsilon ||
        std::abs(pitch - m_viewCubeCache.lastPitch) > kEpsilon) {

        // Recompute geometry: build rotation matrix from camera yaw/pitch
        f32 yawRad = -yaw * kDeg2Rad;
        f32 pitchRad = pitch * kDeg2Rad;

        f32 cy = std::cos(yawRad), sy = std::sin(yawRad);
        f32 cp = std::cos(pitchRad), sp = std::sin(pitchRad);

        // Rotation: yaw around Y, then pitch around X
        // R = Rx(pitch) * Ry(yaw)
        auto rotate = [&](const Vec3& v) -> Vec3 {
            // Ry
            f32 rx = cy * v.x + sy * v.z;
            f32 ry = v.y;
            f32 rz = -sy * v.x + cy * v.z;
            // Rx
            f32 fx = rx;
            f32 fy = cp * ry - sp * rz;
            f32 fz = sp * ry + cp * rz;
            return {fx, fy, fz};
        };

        // Rotate all vertices and store as OFFSETS (relative to origin)
        for (int i = 0; i < 8; i++) {
            Vec3 r = rotate(verts[static_cast<usize>(i)]);
            // Store offsets, not absolute coords
            m_viewCubeCache.projectedVerts[static_cast<usize>(i)] = {r.x * kCubeSize,
                                                                     -r.y * kCubeSize};
            m_viewCubeCache.depths[static_cast<usize>(i)] = r.z;
        }

        // Sort faces back-to-front by average Z
        for (int i = 0; i < 6; i++) {
            const auto& f = faces[static_cast<usize>(i)];
            f32 avgZ = (m_viewCubeCache.depths[static_cast<usize>(f.v[0])] +
                        m_viewCubeCache.depths[static_cast<usize>(f.v[1])] +
                        m_viewCubeCache.depths[static_cast<usize>(f.v[2])] +
                        m_viewCubeCache.depths[static_cast<usize>(f.v[3])]) *
                       0.25f;
            m_viewCubeCache.sortedFaces[static_cast<usize>(i)] = {i, avgZ};
        }
        std::sort(m_viewCubeCache.sortedFaces.begin(), m_viewCubeCache.sortedFaces.end(),
                  [](const ViewCubeCache::FaceSort& a, const ViewCubeCache::FaceSort& b) {
                      return a.avgZ < b.avgZ;
                  });

        m_viewCubeCache.lastYaw = yaw;
        m_viewCubeCache.lastPitch = pitch;
        m_viewCubeCache.valid = true;
    }

    // Apply origin offset to cached verts for drawing (convert offsets to absolute coords)
    std::array<ImVec2, 8> proj;
    for (int i = 0; i < 8; i++) {
        proj[static_cast<usize>(i)] = {
            origin.x + m_viewCubeCache.projectedVerts[static_cast<usize>(i)].x,
            origin.y + m_viewCubeCache.projectedVerts[static_cast<usize>(i)].y};
    }

    // Hit test: check mouse against faces front-to-back
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    int hoveredFace = -1;
    int clickedFace = -1;

    // Point-in-quad test using cross products
    auto pointInQuad = [](ImVec2 p, ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d) -> bool {
        auto cross2d = [](ImVec2 o, ImVec2 e, ImVec2 pt) -> f32 {
            return (e.x - o.x) * (pt.y - o.y) - (e.y - o.y) * (pt.x - o.x);
        };
        f32 c0 = cross2d(a, b, p);
        f32 c1 = cross2d(b, c, p);
        f32 c2 = cross2d(c, d, p);
        f32 c3 = cross2d(d, a, p);
        return (c0 >= 0 && c1 >= 0 && c2 >= 0 && c3 >= 0) ||
               (c0 <= 0 && c1 <= 0 && c2 <= 0 && c3 <= 0);
    };

    // Check front-to-back (reverse of cached sorted order)
    for (int si = 5; si >= 0; si--) {
        const auto& fs = m_viewCubeCache.sortedFaces[static_cast<usize>(si)];
        const auto& f = faces[static_cast<usize>(fs.index)];
        if (fs.avgZ < 0)
            continue; // Skip back faces for hit testing

        ImVec2 qa = proj[static_cast<usize>(f.v[0])];
        ImVec2 qb = proj[static_cast<usize>(f.v[1])];
        ImVec2 qc = proj[static_cast<usize>(f.v[2])];
        ImVec2 qd = proj[static_cast<usize>(f.v[3])];

        if (pointInQuad(mousePos, qa, qb, qc, qd)) {
            hoveredFace = fs.index;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                clickedFace = fs.index;
            }
            break;
        }
    }

    // Draw faces back-to-front
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (int si = 0; si < 6; si++) {
        const auto& fs = m_viewCubeCache.sortedFaces[static_cast<usize>(si)];
        const auto& f = faces[static_cast<usize>(fs.index)];

        ImVec2 qa = proj[static_cast<usize>(f.v[0])];
        ImVec2 qb = proj[static_cast<usize>(f.v[1])];
        ImVec2 qc = proj[static_cast<usize>(f.v[2])];
        ImVec2 qd = proj[static_cast<usize>(f.v[3])];

        // Choose color based on facing and hover
        ImU32 color = kFaceColor;
        if (fs.avgZ > 0.3f)
            color = kFrontFace;
        if (fs.index == hoveredFace)
            color = kFaceHover;

        drawList->AddQuadFilled(qa, qb, qc, qd, color);
        drawList->AddQuad(qa, qb, qc, qd, kEdgeColor, 1.0f);

        // Draw label on front-facing faces
        if (fs.avgZ > 0.0f) {
            ImVec2 center = {
                (qa.x + qb.x + qc.x + qd.x) * 0.25f,
                (qa.y + qb.y + qc.y + qd.y) * 0.25f,
            };
            ImVec2 textSize = ImGui::CalcTextSize(f.label);
            drawList->AddText({center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f},
                              kLabelColor, f.label);
        }
    }

    // Handle click: snap camera to target orientation
    if (clickedFace >= 0) {
        const auto& f = faces[static_cast<usize>(clickedFace)];
        m_camera.setYaw(f.yaw);
        m_camera.setPitch(f.pitch);
    }
}

} // namespace dw
