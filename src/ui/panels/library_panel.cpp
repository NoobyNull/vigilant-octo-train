#include "library_panel.h"

#include "../icons.h"

#include <imgui.h>

namespace dw {

LibraryPanel::LibraryPanel(LibraryManager* library)
    : Panel("Library"), m_library(library) {
    refresh();
}

void LibraryPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();
        renderModelList();
    }
    ImGui::End();
}

void LibraryPanel::refresh() {
    if (m_library) {
        if (m_searchQuery.empty()) {
            m_models = m_library->getAllModels();
        } else {
            m_models = m_library->searchModels(m_searchQuery);
        }
    }
}

void LibraryPanel::renderToolbar() {
    // Search input
    ImGui::SetNextItemWidth(-100.0f);
    if (ImGui::InputTextWithHint("##Search", "Search models...",
                                  m_searchQuery.data(), 256,
                                  ImGuiInputTextFlags_CallbackResize,
                                  [](ImGuiInputTextCallbackData* data) -> int {
                                      if (data->EventFlag ==
                                          ImGuiInputTextFlags_CallbackResize) {
                                          auto* str =
                                              static_cast<std::string*>(data->UserData);
                                          str->resize(data->BufTextLen);
                                          data->Buf = str->data();
                                      }
                                      return 0;
                                  },
                                  &m_searchQuery)) {
        refresh();
    }

    ImGui::SameLine();

    // Refresh button
    if (ImGui::Button(Icons::Refresh)) {
        refresh();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh library");
    }

    ImGui::SameLine();

    // View toggle
    if (ImGui::Button(m_showThumbnails ? Icons::Grid : Icons::List)) {
        m_showThumbnails = !m_showThumbnails;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(m_showThumbnails ? "List view" : "Grid view");
    }
}

void LibraryPanel::renderModelList() {
    ImGui::BeginChild("ModelList", ImVec2(0, 0), false);

    if (m_models.empty()) {
        ImGui::TextDisabled("No models in library");
        ImGui::TextDisabled("Import models using File > Import");
    } else {
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderModelItem(const ModelRecord& model, int index) {
    ImGui::PushID(static_cast<int>(model.id));

    bool isSelected = (model.id == m_selectedModelId);

    // Calculate item height based on view mode
    float itemHeight = m_showThumbnails ? m_thumbnailSize + 8.0f : 24.0f;

    // Selectable for the whole item
    if (ImGui::Selectable("##item", isSelected,
                          ImGuiSelectableFlags_AllowDoubleClick,
                          ImVec2(0, itemHeight))) {
        m_selectedModelId = model.id;
        if (m_onModelSelected) {
            m_onModelSelected(model.id);
        }
    }

    // Context menu
    if (ImGui::BeginPopupContextItem("ModelContext")) {
        renderContextMenu(model);
        ImGui::EndPopup();
    }

    // Draw content on top of selectable
    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();

    ImGui::SetCursorScreenPos(ImVec2(itemMin.x + 4, itemMin.y + 2));

    if (m_showThumbnails) {
        // Thumbnail placeholder (TODO: actual thumbnails)
        ImGui::BeginGroup();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 thumbMin = ImGui::GetCursorScreenPos();
        ImVec2 thumbMax =
            ImVec2(thumbMin.x + m_thumbnailSize, thumbMin.y + m_thumbnailSize);
        drawList->AddRectFilled(thumbMin, thumbMax,
                                IM_COL32(60, 60, 60, 255), 4.0f);
        drawList->AddRect(thumbMin, thumbMax, IM_COL32(80, 80, 80, 255), 4.0f);

        // 3D icon in center
        ImVec2 iconPos =
            ImVec2(thumbMin.x + m_thumbnailSize / 2 - 8,
                   thumbMin.y + m_thumbnailSize / 2 - 8);
        ImGui::SetCursorScreenPos(iconPos);
        ImGui::TextDisabled("%s", Icons::Model);

        ImGui::SetCursorScreenPos(
            ImVec2(thumbMin.x + m_thumbnailSize + 8, thumbMin.y));

        // Model name and info
        ImGui::Text("%s", model.name.c_str());

        // Format info
        std::string info = model.fileFormat;
        if (model.triangleCount > 0) {
            if (model.triangleCount >= 1000000) {
                info += " | " +
                        std::to_string(model.triangleCount / 1000000) + "M tris";
            } else if (model.triangleCount >= 1000) {
                info += " | " +
                        std::to_string(model.triangleCount / 1000) + "K tris";
            } else {
                info += " | " + std::to_string(model.triangleCount) + " tris";
            }
        }
        ImGui::TextDisabled("%s", info.c_str());

        ImGui::EndGroup();
    } else {
        // List view - just name and format
        ImGui::Text("%s %s", Icons::Model, model.name.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        ImGui::TextDisabled("%s", model.fileFormat.c_str());
    }

    ImGui::PopID();
}

void LibraryPanel::renderContextMenu(const ModelRecord& model) {
    if (ImGui::MenuItem("Open")) {
        if (m_onModelSelected) {
            m_onModelSelected(model.id);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Rename")) {
        // TODO: Implement rename dialog
    }

    if (ImGui::MenuItem("Delete")) {
        if (m_library) {
            m_library->removeModel(model.id);
            refresh();
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer")) {
        // TODO: Open file location
    }

    if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(model.filePath.string().c_str());
    }
}

}  // namespace dw
