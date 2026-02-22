#include "lighting_dialog.h"

#include <cmath>

#include <imgui.h>

namespace dw {

void LightingDialog::render() {
    if (!m_open || !m_settings) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Lighting Settings", &m_open)) {
        // Light direction (as angles for easier manipulation)
        if (ImGui::CollapsingHeader("Light Direction", ImGuiTreeNodeFlags_DefaultOpen)) {
            constexpr float kRad2Deg = 180.0f / 3.14159f;
            constexpr float kDeg2Rad = 3.14159f / 180.0f;

            // Convert direction to angles for intuitive control
            Vec2 spherical = toSpherical(m_settings->lightDir);
            float azimuth = spherical.x * kRad2Deg;
            float elevation = spherical.y * kRad2Deg;

            bool changed = false;
            changed |= ImGui::SliderFloat("Azimuth", &azimuth, -180.0f, 180.0f, "%.0f deg");
            changed |= ImGui::SliderFloat("Elevation", &elevation, 0.0f, 90.0f, "%.0f deg");

            if (changed) {
                m_settings->lightDir = fromSpherical(azimuth * kDeg2Rad, elevation * kDeg2Rad);
            }

            // Direct vector editing
            ImGui::Separator();
            ImGui::Text("Direction Vector:");
            ImGui::DragFloat3("##dir", &m_settings->lightDir.x, 0.01f, -1.0f, 1.0f, "%.2f");

            if (ImGui::Button("From Top")) {
                m_settings->lightDir = Vec3{0.0f, -1.0f, 0.0f};
            }
            ImGui::SameLine();
            if (ImGui::Button("From Front")) {
                m_settings->lightDir = Vec3{0.0f, -0.5f, -1.0f};
            }
            ImGui::SameLine();
            if (ImGui::Button("45 deg")) {
                m_settings->lightDir = Vec3{-0.5f, -0.7f, -0.5f};
            }
        }

        // Light color
        if (ImGui::CollapsingHeader("Light Color", ImGuiTreeNodeFlags_DefaultOpen)) {
            float lightCol[3] = {m_settings->lightColor.x,
                                 m_settings->lightColor.y,
                                 m_settings->lightColor.z};
            if (ImGui::ColorEdit3("Light", lightCol)) {
                m_settings->lightColor = Vec3{lightCol[0], lightCol[1], lightCol[2]};
            }

            // Intensity slider
            float intensity = std::max(
                {m_settings->lightColor.x, m_settings->lightColor.y, m_settings->lightColor.z});
            if (ImGui::SliderFloat("Intensity", &intensity, 0.1f, 2.0f)) {
                float scale = intensity / std::max(0.001f,
                                                   std::max({m_settings->lightColor.x,
                                                             m_settings->lightColor.y,
                                                             m_settings->lightColor.z}));
                m_settings->lightColor.x *= scale;
                m_settings->lightColor.y *= scale;
                m_settings->lightColor.z *= scale;
            }
        }

        // Ambient light
        if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            float ambientCol[3] = {m_settings->ambient.x,
                                   m_settings->ambient.y,
                                   m_settings->ambient.z};
            if (ImGui::ColorEdit3("Ambient", ambientCol)) {
                m_settings->ambient = Vec3{ambientCol[0], ambientCol[1], ambientCol[2]};
            }

            float ambientLevel =
                (m_settings->ambient.x + m_settings->ambient.y + m_settings->ambient.z) / 3.0f;
            if (ImGui::SliderFloat("Ambient Level", &ambientLevel, 0.0f, 1.0f)) {
                m_settings->ambient = Vec3{ambientLevel, ambientLevel, ambientLevel};
            }
        }

        // Object appearance
        if (ImGui::CollapsingHeader("Object Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            float objCol[3] = {m_settings->objectColor.r,
                               m_settings->objectColor.g,
                               m_settings->objectColor.b};
            if (ImGui::ColorEdit3("Object Color", objCol)) {
                m_settings->objectColor = Color{objCol[0], objCol[1], objCol[2], 1.0f};
            }

            ImGui::SliderFloat("Shininess", &m_settings->shininess, 1.0f, 128.0f, "%.0f");
        }

        // Render options
        if (ImGui::CollapsingHeader("Render Options")) {
            ImGui::Checkbox("Wireframe", &m_settings->wireframe);
            ImGui::Checkbox("Show Grid", &m_settings->showGrid);
            ImGui::Checkbox("Show Axis", &m_settings->showAxis);
        }

        ImGui::Separator();

        // Presets
        ImGui::Text("Presets:");
        if (ImGui::Button("Default")) {
            m_settings->lightDir = Vec3{-0.5f, -1.0f, -0.3f};
            m_settings->lightColor = Vec3{1.0f, 1.0f, 1.0f};
            m_settings->ambient = Vec3{0.2f, 0.2f, 0.2f};
            m_settings->objectColor = Color::fromHex(0x6699CC);
            m_settings->shininess = 32.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Bright")) {
            m_settings->lightDir = Vec3{-0.3f, -0.8f, -0.5f};
            m_settings->lightColor = Vec3{1.2f, 1.2f, 1.2f};
            m_settings->ambient = Vec3{0.4f, 0.4f, 0.4f};
            m_settings->shininess = 64.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Soft")) {
            m_settings->lightDir = Vec3{0.0f, -1.0f, 0.0f};
            m_settings->lightColor = Vec3{0.8f, 0.8f, 0.8f};
            m_settings->ambient = Vec3{0.5f, 0.5f, 0.5f};
            m_settings->shininess = 16.0f;
        }
    }
    ImGui::End();
}

} // namespace dw
