#include "library_panel.h"

#include "../icons.h"

#include <imgui.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace dw {

LibraryPanel::LibraryPanel(LibraryManager* library)
    : Panel("Library"), m_library(library) {
    refresh();
}

LibraryPanel::~LibraryPanel() {
    clearTextureCache();
}

void LibraryPanel::clearTextureCache() {
    for (auto& [id, tex] : m_textureCache) {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
        }
    }
    m_textureCache.clear();
}

GLuint LibraryPanel::loadTGATexture(const Path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return 0;
    }

    // Read 18-byte TGA header
    uint8_t header[18];
    file.read(reinterpret_cast<char*>(header), 18);
    if (!file) {
        return 0;
    }

    // Validate: uncompressed true-color (type 2), 32bpp
    if (header[2] != 2 || header[16] != 32) {
        return 0;
    }

    int width = header[12] | (header[13] << 8);
    int height = header[14] | (header[15] << 8);
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return 0;
    }

    // Read BGRA pixel data
    size_t dataSize = static_cast<size_t>(width) * height * 4;
    std::vector<uint8_t> bgra(dataSize);
    file.read(reinterpret_cast<char*>(bgra.data()),
              static_cast<std::streamsize>(dataSize));
    if (!file) {
        return 0;
    }

    // Convert BGRA to RGBA in-place
    for (size_t i = 0; i < dataSize; i += 4) {
        std::swap(bgra[i + 0], bgra[i + 2]);  // swap B and R
    }

    // Create OpenGL texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, bgra.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

GLuint LibraryPanel::getThumbnailTexture(const ModelRecord& model) {
    // Check cache first
    auto it = m_textureCache.find(model.id);
    if (it != m_textureCache.end()) {
        return it->second;
    }

    // No thumbnail path means no thumbnail
    if (model.thumbnailPath.empty()) {
        m_textureCache[model.id] = 0;
        return 0;
    }

    // Try to load the TGA file
    GLuint tex = loadTGATexture(model.thumbnailPath);
    m_textureCache[model.id] = tex;
    return tex;
}

void LibraryPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();
        renderModelList();
        renderRenameDialog();
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

    if (m_showThumbnails) {
        // Thumbnail view: selectable with embedded thumbnail + text
        float itemHeight = m_thumbnailSize + 8.0f;

        if (ImGui::Selectable("##item", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, itemHeight))) {
            m_selectedModelId = model.id;
            if (m_onModelSelected) {
                m_onModelSelected(model.id);
            }
        }

        if (ImGui::BeginPopupContextItem("ModelContext")) {
            renderContextMenu(model);
            ImGui::EndPopup();
        }

        // Draw thumbnail and text over the selectable using draw list
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float pad = 4.0f;
        ImVec2 thumbMin = ImVec2(itemMin.x + pad, itemMin.y + pad);
        ImVec2 thumbMax = ImVec2(thumbMin.x + m_thumbnailSize,
                                 thumbMin.y + m_thumbnailSize);

        GLuint tex = getThumbnailTexture(model);
        if (tex != 0) {
            drawList->AddImageRounded(
                (ImTextureID)(intptr_t)tex, thumbMin, thumbMax,
                ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255),
                4.0f);
            drawList->AddRect(thumbMin, thumbMax,
                              IM_COL32(80, 80, 80, 255), 4.0f);
        } else {
            drawList->AddRectFilled(thumbMin, thumbMax,
                                    IM_COL32(60, 60, 60, 255), 4.0f);
            drawList->AddRect(thumbMin, thumbMax,
                              IM_COL32(80, 80, 80, 255), 4.0f);

            // Center icon in placeholder
            const char* icon = Icons::Model;
            ImVec2 iconSize = ImGui::CalcTextSize(icon);
            ImVec2 iconPos = ImVec2(
                thumbMin.x + (m_thumbnailSize - iconSize.x) * 0.5f,
                thumbMin.y + (m_thumbnailSize - iconSize.y) * 0.5f);
            drawList->AddText(iconPos, IM_COL32(128, 128, 128, 255), icon);
        }

        // Text to the right of thumbnail, clipped to item bounds
        float textX = thumbMax.x + 8.0f;
        float textMaxX = itemMax.x - pad;

        // Model name
        ImVec4 clipRect(textX, itemMin.y, textMaxX, itemMax.y);
        drawList->AddText(nullptr, 0.0f, ImVec2(textX, thumbMin.y),
                          ImGui::GetColorU32(ImGuiCol_Text),
                          model.name.c_str(), nullptr, 0.0f, &clipRect);

        // Format info line
        std::string info = model.fileFormat;
        if (model.triangleCount > 0) {
            if (model.triangleCount >= 1000000) {
                info += " | " +
                        std::to_string(model.triangleCount / 1000000) +
                        "M tris";
            } else if (model.triangleCount >= 1000) {
                info += " | " +
                        std::to_string(model.triangleCount / 1000) +
                        "K tris";
            } else {
                info += " | " + std::to_string(model.triangleCount) +
                        " tris";
            }
        }
        float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        drawList->AddText(nullptr, 0.0f,
                          ImVec2(textX, thumbMin.y + lineHeight),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled),
                          info.c_str(), nullptr, 0.0f, &clipRect);
    } else {
        // List view - compact row
        if (ImGui::Selectable("##item", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, 24.0f))) {
            m_selectedModelId = model.id;
            if (m_onModelSelected) {
                m_onModelSelected(model.id);
            }
        }

        if (ImGui::BeginPopupContextItem("ModelContext")) {
            renderContextMenu(model);
            ImGui::EndPopup();
        }

        // Draw text over the selectable
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float pad = 4.0f;

        // Name on the left
        std::string label = std::string(Icons::Model) + " " + model.name;
        float textMaxX = itemMax.x - 70.0f;
        ImVec4 clipRect(itemMin.x + pad, itemMin.y, textMaxX, itemMax.y);
        drawList->AddText(nullptr, 0.0f,
                          ImVec2(itemMin.x + pad, itemMin.y + 3.0f),
                          ImGui::GetColorU32(ImGuiCol_Text),
                          label.c_str(), nullptr, 0.0f, &clipRect);

        // Format on the right
        ImVec2 fmtSize = ImGui::CalcTextSize(model.fileFormat.c_str());
        drawList->AddText(
            ImVec2(itemMax.x - fmtSize.x - pad, itemMin.y + 3.0f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            model.fileFormat.c_str());
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
        m_showRenameDialog = true;
        m_renameModelId = model.id;
        std::strncpy(m_renameBuffer, model.name.c_str(), sizeof(m_renameBuffer) - 1);
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }

    if (ImGui::MenuItem("Delete")) {
        if (m_library) {
            m_library->removeModel(model.id);
            refresh();
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer")) {
        auto parentDir = model.filePath.parent_path().string();
        if (!parentDir.empty()) {
            std::string cmd = "xdg-open \"" + parentDir + "\"";
            std::system(cmd.c_str());
        }
    }

    if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(model.filePath.string().c_str());
    }
}

void LibraryPanel::renderRenameDialog() {
    if (m_showRenameDialog) {
        ImGui::OpenPopup("Rename Model");
        m_showRenameDialog = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Rename Model", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        ImGui::SetNextItemWidth(-1);

        bool enterPressed = ImGui::InputText(
            "##RenameInput", m_renameBuffer, sizeof(m_renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue);

        // Auto-focus the input field when popup opens
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::Spacing();

        bool okPressed = ImGui::Button("OK", ImVec2(120, 0));
        ImGui::SameLine();
        bool cancelPressed = ImGui::Button("Cancel", ImVec2(120, 0));

        if (okPressed || enterPressed) {
            std::string newName(m_renameBuffer);
            if (!newName.empty() && m_library) {
                auto record = m_library->getModel(m_renameModelId);
                if (record) {
                    record->name = newName;
                    m_library->updateModel(*record);
                    refresh();
                }
            }
            ImGui::CloseCurrentPopup();
        }

        if (cancelPressed) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}  // namespace dw
