#include "context_menu_manager.h"

#include <imgui.h>

namespace dw {

ContextMenuManager::ContextMenuManager() = default;

ContextMenuManager::~ContextMenuManager() = default;

void ContextMenuManager::registerEntries(const std::string& popupId,
                                         const std::vector<ContextMenuEntry>& entries) {
    m_entries[popupId] = entries;
    m_popupState[popupId] = false;
}

bool ContextMenuManager::render(const std::string& popupId) {
    auto it = m_entries.find(popupId);
    if (it == m_entries.end()) {
        // No entries registered for this popup
        return false;
    }

    renderEntries(it->second);
    return true;
}

bool ContextMenuManager::isPopupOpen(const std::string& popupId) const {
    auto it = m_popupState.find(popupId);
    return it != m_popupState.end() && it->second;
}

void ContextMenuManager::closePopup(const std::string& popupId) {
    if (m_popupState.find(popupId) != m_popupState.end()) {
        m_popupState[popupId] = false;
        ImGui::CloseCurrentPopup();
    }
}

void ContextMenuManager::clearEntries(const std::string& popupId) {
    m_entries.erase(popupId);
    m_popupState.erase(popupId);
}

void ContextMenuManager::renderEntries(const std::vector<ContextMenuEntry>& entries) {
    for (const auto& entry : entries) {
        // Render separator
        if (entry.isSeparator) {
            ImGui::Separator();
            continue;
        }

        // Check if entry should be disabled
        bool isEnabled = entry.enabled();

        // Render menu item with optional icon
        std::string displayLabel = entry.label;
        if (!entry.icon.empty()) {
            displayLabel = entry.icon + " " + entry.label;
        }

        // Handle submenu
        if (!entry.submenu.empty()) {
            if (ImGui::BeginMenu(displayLabel.c_str(), isEnabled)) {
                renderEntries(entry.submenu);
                ImGui::EndMenu();
            }
        } else {
            // Regular menu item
            if (ImGui::MenuItem(displayLabel.c_str(), nullptr, false, isEnabled)) {
                if (entry.action) {
                    entry.action();
                }
            }
        }
    }
}

} // namespace dw
