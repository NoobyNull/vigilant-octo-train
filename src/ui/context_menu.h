#pragma once

// Digital Workshop - Context Menu System
// Lightweight, reusable right-click context menus for any panel.
// Each panel builds its own menu entries and calls render().

#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

namespace dw {

struct ContextMenuItem {
    std::string label;
    std::function<void()> action;
    bool enabled = true;
    bool isSeparator = false;
    bool isToggle = false;
    bool toggleValue = false;
};

class ContextMenu {
  public:
    explicit ContextMenu(const char* id) : m_id(id) {}

    void clear() { m_items.clear(); }

    void addItem(const std::string& label, std::function<void()> action, bool enabled = true) {
        m_items.push_back({label, std::move(action), enabled, false, false, false});
    }

    void addToggle(const std::string& label, bool value, std::function<void()> action) {
        m_items.push_back({label, std::move(action), true, false, true, value});
    }

    void addSeparator() { m_items.push_back({"", nullptr, true, true, false, false}); }

    // Call this where right-click should open the menu
    void open() { ImGui::OpenPopup(m_id); }

    // Call this each frame inside the panel's render
    void render() {
        if (ImGui::BeginPopup(m_id)) {
            for (auto& item : m_items) {
                if (item.isSeparator) {
                    ImGui::Separator();
                    continue;
                }
                if (item.isToggle) {
                    bool val = item.toggleValue;
                    if (ImGui::MenuItem(item.label.c_str(), nullptr, &val)) {
                        if (item.action)
                            item.action();
                    }
                } else {
                    if (ImGui::MenuItem(item.label.c_str(), nullptr, false, item.enabled)) {
                        if (item.action)
                            item.action();
                    }
                }
            }
            ImGui::EndPopup();
        }
    }

  private:
    const char* m_id;
    std::vector<ContextMenuItem> m_items;
};

} // namespace dw
