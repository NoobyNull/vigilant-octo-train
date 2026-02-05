#include "properties_panel.h"

#include "../icons.h"

#include <imgui.h>

namespace dw {

PropertiesPanel::PropertiesPanel() : Panel("Properties") {}

void PropertiesPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        if (m_mesh && m_mesh->isValid()) {
            renderMeshInfo();
            ImGui::Spacing();
            renderBoundsInfo();
            ImGui::Spacing();
            renderTransformInfo();
            ImGui::Spacing();
            renderMaterialInfo();
        } else {
            ImGui::TextDisabled("No model selected");
            ImGui::TextDisabled("Select a model from the library");
        }
    }
    ImGui::End();
}

void PropertiesPanel::setMesh(std::shared_ptr<Mesh> mesh,
                               const std::string& name) {
    m_mesh = std::move(mesh);
    m_meshName = name;
}

void PropertiesPanel::clearMesh() {
    m_mesh = nullptr;
    m_meshName.clear();
}

void PropertiesPanel::renderMeshInfo() {
    if (ImGui::CollapsingHeader("Mesh Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Name
        if (!m_meshName.empty()) {
            ImGui::Text("Name: %s", m_meshName.c_str());
        }

        // Vertex count
        size_t vertexCount = m_mesh->vertices().size();
        if (vertexCount >= 1000000) {
            ImGui::Text("Vertices: %.2fM", vertexCount / 1000000.0);
        } else if (vertexCount >= 1000) {
            ImGui::Text("Vertices: %.1fK", vertexCount / 1000.0);
        } else {
            ImGui::Text("Vertices: %zu", vertexCount);
        }

        // Triangle count
        size_t triangleCount = m_mesh->triangleCount();
        if (triangleCount >= 1000000) {
            ImGui::Text("Triangles: %.2fM", triangleCount / 1000000.0);
        } else if (triangleCount >= 1000) {
            ImGui::Text("Triangles: %.1fK", triangleCount / 1000.0);
        } else {
            ImGui::Text("Triangles: %zu", triangleCount);
        }

        // Index count
        size_t indexCount = m_mesh->indices().size();
        ImGui::Text("Indices: %zu", indexCount);

        // Memory usage estimate
        size_t memoryBytes =
            vertexCount * sizeof(Vertex) + indexCount * sizeof(uint32_t);
        if (memoryBytes >= 1024 * 1024) {
            ImGui::Text("Memory: %.2f MB", memoryBytes / (1024.0 * 1024.0));
        } else if (memoryBytes >= 1024) {
            ImGui::Text("Memory: %.1f KB", memoryBytes / 1024.0);
        } else {
            ImGui::Text("Memory: %zu bytes", memoryBytes);
        }

        ImGui::Unindent();
    }
}

void PropertiesPanel::renderBoundsInfo() {
    if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        const AABB& bounds = m_mesh->bounds();

        ImGui::Text("Min: (%.3f, %.3f, %.3f)", bounds.min.x, bounds.min.y,
                    bounds.min.z);
        ImGui::Text("Max: (%.3f, %.3f, %.3f)", bounds.max.x, bounds.max.y,
                    bounds.max.z);

        Vec3 size = bounds.size();
        ImGui::Text("Size: %.3f x %.3f x %.3f", size.x, size.y, size.z);

        Vec3 center = bounds.center();
        ImGui::Text("Center: (%.3f, %.3f, %.3f)", center.x, center.y,
                    center.z);

        float diagonal = bounds.diagonal();
        ImGui::Text("Diagonal: %.3f", diagonal);

        ImGui::Unindent();
    }
}

void PropertiesPanel::renderTransformInfo() {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Display current mesh bounds as informational readouts
        const AABB& bounds = m_mesh->bounds();
        Vec3 center = bounds.center();
        Vec3 size = bounds.size();

        // Position (center of bounding box) - read-only display
        float pos[3] = {center.x, center.y, center.z};
        ImGui::BeginDisabled();
        ImGui::DragFloat3("Center", pos, 0.1f);
        ImGui::EndDisabled();

        // Size - read-only display
        float sz[3] = {size.x, size.y, size.z};
        ImGui::BeginDisabled();
        ImGui::DragFloat3("Size", sz, 0.1f);
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // One-shot transform operations
        ImGui::Text("Operations");
        ImGui::Spacing();

        // Center on Origin button
        if (ImGui::Button("Center on Origin", ImVec2(-1, 0))) {
            m_mesh->centerOnOrigin();
            if (m_onMeshModified) {
                m_onMeshModified();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Move mesh so its center is at the origin (0, 0, 0)");
        }

        ImGui::Spacing();

        // Normalize Size button with configurable target size
        static float targetSize = 1.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
        ImGui::DragFloat("Target Size", &targetSize, 0.1f, 0.01f, 1000.0f, "%.2f");

        if (ImGui::Button("Normalize Size", ImVec2(-1, 0))) {
            m_mesh->normalizeSize(targetSize);
            if (m_onMeshModified) {
                m_onMeshModified();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Scale mesh so its largest dimension equals the target size");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // One-shot translate, rotate, scale operations
        ImGui::Text("Apply Transform");
        ImGui::Spacing();

        // Translate
        static float translate[3] = {0.0f, 0.0f, 0.0f};
        ImGui::DragFloat3("Translate", translate, 0.1f);
        ImGui::SameLine();
        if (ImGui::Button("Apply##Translate")) {
            Mat4 mat = Mat4::translate(Vec3{translate[0], translate[1], translate[2]});
            m_mesh->transform(mat);
            translate[0] = translate[1] = translate[2] = 0.0f;
            if (m_onMeshModified) {
                m_onMeshModified();
            }
        }

        // Rotate (degrees)
        static float rotateDeg[3] = {0.0f, 0.0f, 0.0f};
        ImGui::DragFloat3("Rotate (deg)", rotateDeg, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Apply##Rotate")) {
            constexpr float DEG2RAD = 3.14159265358979f / 180.0f;
            Mat4 mat = Mat4::identity();
            if (rotateDeg[0] != 0.0f) mat = Mat4::rotateX(rotateDeg[0] * DEG2RAD) * mat;
            if (rotateDeg[1] != 0.0f) mat = Mat4::rotateY(rotateDeg[1] * DEG2RAD) * mat;
            if (rotateDeg[2] != 0.0f) mat = Mat4::rotateZ(rotateDeg[2] * DEG2RAD) * mat;
            m_mesh->transform(mat);
            rotateDeg[0] = rotateDeg[1] = rotateDeg[2] = 0.0f;
            if (m_onMeshModified) {
                m_onMeshModified();
            }
        }

        // Scale
        static float scaleVal[3] = {1.0f, 1.0f, 1.0f};
        ImGui::DragFloat3("Scale", scaleVal, 0.01f, 0.01f, 100.0f);
        ImGui::SameLine();
        if (ImGui::Button("Apply##Scale")) {
            Mat4 mat = Mat4::scale(Vec3{scaleVal[0], scaleVal[1], scaleVal[2]});
            m_mesh->transform(mat);
            scaleVal[0] = scaleVal[1] = scaleVal[2] = 1.0f;
            if (m_onMeshModified) {
                m_onMeshModified();
            }
        }

        ImGui::Unindent();
    }
}

void PropertiesPanel::renderMaterialInfo() {
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        float color[3] = {m_objectColor.r, m_objectColor.g, m_objectColor.b};
        if (ImGui::ColorEdit3("Object Color", color,
                              ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_PickerHueWheel)) {
            m_objectColor.r = color[0];
            m_objectColor.g = color[1];
            m_objectColor.b = color[2];
            if (m_onColorChanged) {
                m_onColorChanged(m_objectColor);
            }
        }

        // Color presets
        ImGui::Spacing();
        ImGui::Text("Presets");
        ImGui::Spacing();

        struct ColorPreset {
            const char* name;
            u32 hex;
        };

        static const ColorPreset presets[] = {
            {"Steel Blue", 0x6699CC},
            {"Silver", 0xC0C0C0},
            {"Gold", 0xDAA520},
            {"Copper", 0xB87333},
            {"Red", 0xCC3333},
            {"Green", 0x33CC33},
            {"White", 0xEEEEEE},
            {"Dark Gray", 0x555555},
        };

        for (int i = 0; i < 8; i++) {
            Color c = Color::fromHex(presets[i].hex);
            ImVec4 imColor(c.r, c.g, c.b, 1.0f);

            if (i > 0 && i % 4 != 0) {
                ImGui::SameLine();
            }

            ImGui::PushID(i);
            if (ImGui::ColorButton(presets[i].name, imColor, 0, ImVec2(24, 24))) {
                m_objectColor = c;
                if (m_onColorChanged) {
                    m_onColorChanged(m_objectColor);
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", presets[i].name);
            }
            ImGui::PopID();
        }

        ImGui::Unindent();
    }
}

}  // namespace dw
