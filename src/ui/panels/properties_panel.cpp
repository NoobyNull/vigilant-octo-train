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
    if (ImGui::CollapsingHeader("Transform")) {
        ImGui::Indent();

        ImGui::TextDisabled("Transform editing not yet implemented");

        // Placeholder for future transform controls
        static float position[3] = {0, 0, 0};
        static float rotation[3] = {0, 0, 0};
        static float scale[3] = {1, 1, 1};

        ImGui::BeginDisabled();
        ImGui::DragFloat3("Position", position, 0.1f);
        ImGui::DragFloat3("Rotation", rotation, 1.0f);
        ImGui::DragFloat3("Scale", scale, 0.01f, 0.01f, 100.0f);
        ImGui::EndDisabled();

        ImGui::Unindent();
    }
}

void PropertiesPanel::renderMaterialInfo() {
    if (ImGui::CollapsingHeader("Material")) {
        ImGui::Indent();

        ImGui::TextDisabled("Material editing not yet implemented");

        ImGui::Unindent();
    }
}

}  // namespace dw
