#include "machine_profile_dialog.h"

#include <cstdio>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../icons.h"

namespace dw {

void MachineProfileDialog::open() {
    m_open = true;
    auto& config = Config::instance();
    m_editProfileIndex = config.getActiveMachineProfileIndex();
    m_editProfile = config.getMachineProfiles()[static_cast<size_t>(m_editProfileIndex)];
}

void MachineProfileDialog::render() {
    if (!m_open)
        return;

    const auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x * 0.28f, viewport->WorkSize.y * 0.5f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Machine Profiles", &m_open)) {
        auto& config = Config::instance();
        const auto& profiles = config.getMachineProfiles();
        int activeIdx = config.getActiveMachineProfileIndex();

        // Active profile selector
        ImGui::Text("%s Active Profile", Icons::Settings);
        ImGui::SetNextItemWidth(-1);
        const char* activePreview = profiles[static_cast<size_t>(activeIdx)].name.c_str();
        if (ImGui::BeginCombo("##ActiveProfile", activePreview)) {
            for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
                bool selected = (i == activeIdx);
                if (ImGui::Selectable(profiles[static_cast<size_t>(i)].name.c_str(), selected)) {
                    config.setActiveMachineProfileIndex(i);
                    m_editProfileIndex = i;
                    m_editProfile = profiles[static_cast<size_t>(i)];
                    config.save();
                    if (m_onChanged)
                        m_onChanged();
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // Editing profile selector
        ImGui::Text("Edit Profile:");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##EditProfile",
                              profiles[static_cast<size_t>(m_editProfileIndex)].name.c_str())) {
            for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
                if (ImGui::Selectable(profiles[static_cast<size_t>(i)].name.c_str(),
                                      i == m_editProfileIndex)) {
                    m_editProfileIndex = i;
                    m_editProfile = profiles[static_cast<size_t>(i)];
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // Name
        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s", m_editProfile.name.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            m_editProfile.name = nameBuf;

        ImGui::Spacing();

        // Feed rates
        if (ImGui::CollapsingHeader("Feed Rates (mm/min)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::DragFloat("Max X##feed", &m_editProfile.maxFeedRateX, 100.0f, 0.0f, 100000.0f,
                             "%.0f");
            ImGui::DragFloat("Max Y##feed", &m_editProfile.maxFeedRateY, 100.0f, 0.0f, 100000.0f,
                             "%.0f");
            ImGui::DragFloat("Max Z##feed", &m_editProfile.maxFeedRateZ, 100.0f, 0.0f, 100000.0f,
                             "%.0f");
            ImGui::DragFloat("Rapid", &m_editProfile.rapidRate, 100.0f, 0.0f, 100000.0f, "%.0f");
            ImGui::DragFloat("Default Feed", &m_editProfile.defaultFeedRate, 100.0f, 0.0f,
                             100000.0f, "%.0f");
            ImGui::Unindent();
        }

        // Acceleration
        if (ImGui::CollapsingHeader("Acceleration (mm/s²)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::DragFloat("Accel X", &m_editProfile.accelX, 10.0f, 0.0f, 10000.0f, "%.0f");
            ImGui::DragFloat("Accel Y", &m_editProfile.accelY, 10.0f, 0.0f, 10000.0f, "%.0f");
            ImGui::DragFloat("Accel Z", &m_editProfile.accelZ, 10.0f, 0.0f, 10000.0f, "%.0f");
            ImGui::Unindent();
        }

        // Travel limits
        if (ImGui::CollapsingHeader("Max Travel (mm)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::DragFloat("Travel X", &m_editProfile.maxTravelX, 10.0f, 0.0f, 10000.0f,
                             "%.0f");
            ImGui::DragFloat("Travel Y", &m_editProfile.maxTravelY, 10.0f, 0.0f, 10000.0f,
                             "%.0f");
            ImGui::DragFloat("Travel Z", &m_editProfile.maxTravelZ, 10.0f, 0.0f, 10000.0f,
                             "%.0f");
            ImGui::Unindent();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Action buttons
        if (ImGui::Button("Save")) {
            config.updateMachineProfile(m_editProfileIndex, m_editProfile);
            config.save();
            if (m_editProfileIndex == config.getActiveMachineProfileIndex() && m_onChanged)
                m_onChanged();
        }
        ImGui::SameLine();
        if (ImGui::Button("New Copy")) {
            gcode::MachineProfile copy = m_editProfile;
            copy.name += " (Copy)";
            copy.builtIn = false;
            config.addMachineProfile(copy);
            m_editProfileIndex = static_cast<int>(config.getMachineProfiles().size()) - 1;
            m_editProfile = copy;
            config.save();
        }
        ImGui::SameLine();

        bool isBuiltIn = profiles[static_cast<size_t>(m_editProfileIndex)].builtIn;
        if (isBuiltIn)
            ImGui::BeginDisabled();
        if (ImGui::Button("Delete")) {
            config.removeMachineProfile(m_editProfileIndex);
            m_editProfileIndex = config.getActiveMachineProfileIndex();
            m_editProfile = config.getActiveMachineProfile();
            config.save();
            if (m_onChanged)
                m_onChanged();
        }
        if (isBuiltIn)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Set Active")) {
            config.setActiveMachineProfileIndex(m_editProfileIndex);
            config.save();
            if (m_onChanged)
                m_onChanged();
        }
    }
    ImGui::End();
}

} // namespace dw
