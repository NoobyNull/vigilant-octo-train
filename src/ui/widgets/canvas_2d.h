#pragma once

#include <algorithm>

#include <imgui.h>

namespace dw {

// Reusable 2D canvas with pan/zoom for ImGui panels.
// Usage:
//   Canvas2D canvas;
//   if (auto area = canvas.begin()) {
//       float scale = canvas.effectiveScale(baseScale);
//       area.drawList->AddLine(...);
//   }
struct Canvas2D {
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float zoomMin = 0.1f;
    float zoomMax = 10.0f;

    // Returned by begin() -- falsy when canvas is too small to draw.
    struct Area {
        ImVec2 pos;  // Screen-space top-left
        ImVec2 size; // Canvas dimensions
        ImDrawList* drawList = nullptr;

        explicit operator bool() const { return drawList != nullptr; }
    };

    // Begin a canvas region. Returns a valid Area if the available space is
    // large enough (>= 50x50); otherwise returns a falsy Area.
    // Call handleInput() after drawing to process pan/zoom.
    Area begin() {
        Area area;
        area.size = ImGui::GetContentRegionAvail();
        float minCanvas = ImGui::GetFontSize() * 3;
        if (area.size.x < minCanvas || area.size.y < minCanvas)
            return area;

        area.pos = ImGui::GetCursorScreenPos();
        area.drawList = ImGui::GetWindowDrawList();
        m_area = area;
        return area;
    }

    // Convert a point from world coordinates to screen coordinates.
    // baseScale is the caller-computed unzoomed scale factor.
    // originX/originY is the screen-space origin (before pan).
    ImVec2 canvasToScreen(float wx, float wy, float scale, float originX, float originY) const {
        return ImVec2(originX + wx * scale, originY + wy * scale);
    }

    // Effective scale = baseScale * zoom
    float effectiveScale(float baseScale) const { return baseScale * zoom; }

    // Process mouse wheel zoom, drag pan, and double-click reset.
    // Must be called after drawing, while the Area from begin() is in scope.
    void handleInput(const char* id) {
        ImGui::SetCursorScreenPos(m_area.pos);
        ImGui::InvisibleButton(id, m_area.size);
        if (ImGui::IsItemHovered()) {
            auto& io = ImGui::GetIO();
            if (io.MouseWheel != 0) {
                zoom *= (1.0f + io.MouseWheel * 0.1f);
                zoom = std::clamp(zoom, zoomMin, zoomMax);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                panX += io.MouseDelta.x;
                panY += io.MouseDelta.y;
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                reset();
            }
        }
    }

    // Reset pan and zoom to defaults
    void reset() {
        zoom = 1.0f;
        panX = 0.0f;
        panY = 0.0f;
    }

  private:
    Area m_area;
};

} // namespace dw
