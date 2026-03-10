#include "group_panel.h"

#include <imgui.h>

namespace dw {

GroupPanel::GroupPanel(int id)
    : Panel("Group " + std::to_string(id)) {}

void GroupPanel::render() {
    m_wasClosed = false;
    if (!m_open)
        return;

    applyMinSize(20, 10);
    bool open = m_open;

    // Zero padding so the inner DockSpace fills the entire window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool visible = ImGui::Begin(m_title.c_str(), &open);
    ImGui::PopStyleVar();

    if (visible) {
        // Host a DockSpace inside this window so panels can be docked INTO it.
        // The ID is derived from the window context, making it unique per Group panel.
        ImGuiID dockId = ImGui::GetID("##GroupDock");
        ImGui::DockSpace(dockId, ImVec2(0.0f, 0.0f));
    }
    ImGui::End();

    if (!open) {
        m_open = false;
        m_wasClosed = true;
    }
}

} // namespace dw
