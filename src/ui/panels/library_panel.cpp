#include "library_panel.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#ifdef _WIN32
    #include <shellapi.h>
    #include <windows.h>
#endif

#include <imgui.h>

#include "../../core/utils/log.h"
#include "../icons.h"

namespace dw {

LibraryPanel::LibraryPanel(LibraryManager* library) : Panel("Library"), m_library(library) {
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
        log::warningf("Library", "Failed to open TGA file: %s", path.string().c_str());
        return 0;
    }

    // Read 18-byte TGA header
    uint8_t header[18];
    file.read(reinterpret_cast<char*>(header), 18);
    if (!file) {
        log::warningf("Library", "Failed to read TGA header: %s", path.string().c_str());
        return 0;
    }

    // Validate: uncompressed true-color (type 2), 32bpp
    if (header[2] != 2 || header[16] != 32) {
        log::warningf("Library", "Unsupported TGA format (type=%d, bpp=%d): %s", header[2],
                      header[16], path.string().c_str());
        return 0;
    }

    int width = header[12] | (header[13] << 8);
    int height = header[14] | (header[15] << 8);
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        log::warningf("Library", "Invalid TGA dimensions (%dx%d): %s", width, height,
                      path.string().c_str());
        return 0;
    }

    // Read BGRA pixel data
    size_t dataSize = static_cast<size_t>(width) * height * 4;
    std::vector<uint8_t> bgra(dataSize);
    file.read(reinterpret_cast<char*>(bgra.data()), static_cast<std::streamsize>(dataSize));
    if (!file) {
        log::warningf("Library", "Failed to read TGA pixel data: %s", path.string().c_str());
        return 0;
    }

    // Convert BGRA to RGBA in-place
    for (size_t i = 0; i < dataSize; i += 4) {
        std::swap(bgra[i + 0], bgra[i + 2]); // swap B and R
    }

    // Create OpenGL texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        log::warning("Library", "Failed to create GL texture for thumbnail");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 bgra.data());
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
        renderTabs();
        ImGui::Separator();

        // Render content based on active tab
        switch (m_activeTab) {
        case ViewTab::Models:
            renderModelList();
            break;
        case ViewTab::GCode:
            renderGCodeList();
            break;
        case ViewTab::All:
            renderCombinedList();
            break;
        }

        renderRenameDialog();
        renderDeleteConfirm();
    }
    ImGui::End();
}

void LibraryPanel::refresh() {
    if (m_library) {
        if (m_searchQuery.empty()) {
            m_models = m_library->getAllModels();
            m_gcodeFiles = m_library->getAllGCodeFiles();
        } else {
            m_models = m_library->searchModels(m_searchQuery);
            m_gcodeFiles = m_library->searchGCodeFiles(m_searchQuery);
        }
    }
}

void LibraryPanel::renderToolbar() {
    float avail = ImGui::GetContentRegionAvail().x;
    float style = ImGui::GetStyle().ItemSpacing.x;

    // Calculate actual button widths dynamically
    float refreshBtnW =
        ImGui::CalcTextSize(Icons::Refresh).x + ImGui::GetStyle().FramePadding.x * 2;
    const char* viewIcon = m_showThumbnails ? Icons::Grid : Icons::List;
    float viewBtnW = ImGui::CalcTextSize(viewIcon).x + ImGui::GetStyle().FramePadding.x * 2;
    float buttonsW = refreshBtnW + viewBtnW + style * 2; // 2 SameLine gaps

    // Search input takes remaining space after buttons
    float searchWidth = avail - buttonsW;
    if (searchWidth < 50.0f) {
        searchWidth = 50.0f;
    }

    ImGui::SetNextItemWidth(searchWidth);
    if (ImGui::InputTextWithHint(
            "##Search", "Search library...", m_searchQuery.data(), 256,
            ImGuiInputTextFlags_CallbackResize,
            [](ImGuiInputTextCallbackData* data) -> int {
                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                    auto* str = static_cast<std::string*>(data->UserData);
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
    if (ImGui::Button(viewIcon)) {
        m_showThumbnails = !m_showThumbnails;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(m_showThumbnails ? "List view" : "Grid view");
    }
}

void LibraryPanel::renderTabs() {
    if (ImGui::BeginTabBar("LibraryTabs")) {
        if (ImGui::BeginTabItem("All")) {
            m_activeTab = ViewTab::All;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Models")) {
            m_activeTab = ViewTab::Models;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("G-code")) {
            m_activeTab = ViewTab::GCode;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
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

        if (ImGui::Selectable("##item", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, itemHeight))) {
            m_selectedModelId = model.id;
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onModelOpened)
                    m_onModelOpened(model.id);
            } else {
                if (m_onModelSelected)
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
        ImVec2 thumbMax = ImVec2(thumbMin.x + m_thumbnailSize, thumbMin.y + m_thumbnailSize);

        GLuint tex = getThumbnailTexture(model);
        if (tex != 0) {
            drawList->AddImageRounded((ImTextureID)(intptr_t)tex, thumbMin, thumbMax, ImVec2(0, 0),
                                      ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 4.0f);
            drawList->AddRect(thumbMin, thumbMax, IM_COL32(80, 80, 80, 255), 4.0f);
        } else {
            drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 60, 255), 4.0f);
            drawList->AddRect(thumbMin, thumbMax, IM_COL32(80, 80, 80, 255), 4.0f);

            // Center icon in placeholder
            const char* icon = Icons::Model;
            ImVec2 iconSize = ImGui::CalcTextSize(icon);
            ImVec2 iconPos = ImVec2(thumbMin.x + (m_thumbnailSize - iconSize.x) * 0.5f,
                                    thumbMin.y + (m_thumbnailSize - iconSize.y) * 0.5f);
            drawList->AddText(iconPos, IM_COL32(128, 128, 128, 255), icon);
        }

        // Text to the right of thumbnail, clipped to item bounds
        float textX = thumbMax.x + 8.0f;
        float textMaxX = itemMax.x - pad;

        // Model name
        ImVec4 clipRect(textX, itemMin.y, textMaxX, itemMax.y);
        drawList->AddText(nullptr, 0.0f, ImVec2(textX, thumbMin.y),
                          ImGui::GetColorU32(ImGuiCol_Text), model.name.c_str(), nullptr, 0.0f,
                          &clipRect);

        // Format info line
        std::string info = model.fileFormat;
        if (model.triangleCount > 0) {
            if (model.triangleCount >= 1000000) {
                info += " | " + std::to_string(model.triangleCount / 1000000) + "M tris";
            } else if (model.triangleCount >= 1000) {
                info += " | " + std::to_string(model.triangleCount / 1000) + "K tris";
            } else {
                info += " | " + std::to_string(model.triangleCount) + " tris";
            }
        }
        float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        drawList->AddText(nullptr, 0.0f, ImVec2(textX, thumbMin.y + lineHeight),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), info.c_str(), nullptr, 0.0f,
                          &clipRect);
    } else {
        // List view - compact row
        if (ImGui::Selectable("##item", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, 24.0f))) {
            m_selectedModelId = model.id;
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onModelOpened)
                    m_onModelOpened(model.id);
            } else {
                if (m_onModelSelected)
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
        drawList->AddText(nullptr, 0.0f, ImVec2(itemMin.x + pad, itemMin.y + 3.0f),
                          ImGui::GetColorU32(ImGuiCol_Text), label.c_str(), nullptr, 0.0f,
                          &clipRect);

        // Format on the right
        ImVec2 fmtSize = ImGui::CalcTextSize(model.fileFormat.c_str());
        drawList->AddText(ImVec2(itemMax.x - fmtSize.x - pad, itemMin.y + 3.0f),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), model.fileFormat.c_str());
    }

    ImGui::PopID();
}

void LibraryPanel::renderContextMenu(const ModelRecord& model) {
    if (ImGui::MenuItem("Open")) {
        if (m_onModelOpened) {
            m_onModelOpened(model.id);
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
        m_showDeleteConfirm = true;
        m_deleteItemId = model.id;
        m_deleteIsGCode = false;
        m_deleteItemName = model.name;
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer")) {
        auto parentDir = model.filePath.parent_path();
        if (!parentDir.empty()) {
#ifdef _WIN32
            ShellExecuteW(nullptr, L"open", parentDir.wstring().c_str(), nullptr, nullptr,
                          SW_SHOWNORMAL);
#else
            std::string cmd = "xdg-open \"" + parentDir.string() + "\"";
            int ret = std::system(cmd.c_str());
            if (ret != 0) {
                log::warningf("Library", "Failed to open file manager (exit code %d)", ret);
            }
#endif
        }
    }

    if (ImGui::MenuItem("Copy Path")) {
        ImGui::SetClipboardText(model.filePath.string().c_str());
    }
}

void LibraryPanel::renderGCodeList() {
    ImGui::BeginChild("GCodeList", ImVec2(0, 0), false);

    if (m_gcodeFiles.empty()) {
        ImGui::TextDisabled("No G-code files in library");
        ImGui::TextDisabled("Import G-code using File > Import");
    } else {
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderCombinedList() {
    ImGui::BeginChild("CombinedList", ImVec2(0, 0), false);

    if (m_models.empty() && m_gcodeFiles.empty()) {
        ImGui::TextDisabled("Library is empty");
        ImGui::TextDisabled("Import files using File > Import");
    } else {
        // Render models first
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i));
        }

        // Then G-code files
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i + m_models.size()));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderGCodeItem(const GCodeRecord& gcode, int index) {
    ImGui::PushID(static_cast<int>(gcode.id + 1000000)); // Offset to avoid ID collision with models

    bool isSelected = (gcode.id == m_selectedGCodeId);

    if (m_showThumbnails) {
        // Thumbnail view
        float itemHeight = m_thumbnailSize + 8.0f;

        if (ImGui::Selectable("##gcodeitem", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, itemHeight))) {
            m_selectedGCodeId = gcode.id;
            m_selectedModelId = -1; // Deselect models
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onGCodeOpened)
                    m_onGCodeOpened(gcode.id);
            } else {
                if (m_onGCodeSelected)
                    m_onGCodeSelected(gcode.id);
            }
        }

        if (ImGui::BeginPopupContextItem("GCodeContext")) {
            renderGCodeContextMenu(gcode);
            ImGui::EndPopup();
        }

        // Draw placeholder and text
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float pad = 4.0f;
        ImVec2 thumbMin = ImVec2(itemMin.x + pad, itemMin.y + pad);
        ImVec2 thumbMax = ImVec2(thumbMin.x + m_thumbnailSize, thumbMin.y + m_thumbnailSize);

        // Draw placeholder with "GC" text
        drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(50, 70, 90, 255), 4.0f);
        drawList->AddRect(thumbMin, thumbMax, IM_COL32(70, 100, 130, 255), 4.0f);

        // Center "GC" text in placeholder
        const char* icon = "GC";
        ImVec2 iconSize = ImGui::CalcTextSize(icon);
        ImVec2 iconPos = ImVec2(thumbMin.x + (m_thumbnailSize - iconSize.x) * 0.5f,
                                thumbMin.y + (m_thumbnailSize - iconSize.y) * 0.5f);
        drawList->AddText(iconPos, IM_COL32(150, 180, 220, 255), icon);

        // Text to the right of thumbnail
        float textX = thumbMax.x + 8.0f;
        float textMaxX = itemMax.x - pad;

        // G-code name
        ImVec4 clipRect(textX, itemMin.y, textMaxX, itemMax.y);
        drawList->AddText(nullptr, 0.0f, ImVec2(textX, thumbMin.y),
                          ImGui::GetColorU32(ImGuiCol_Text), gcode.name.c_str(), nullptr, 0.0f,
                          &clipRect);

        // Format info line with time and distance
        std::string info = "G-code";
        if (gcode.estimatedTime > 0) {
            int minutes = static_cast<int>(gcode.estimatedTime);
            info += " | " + std::to_string(minutes) + "min";
        }
        if (gcode.totalDistance > 0) {
            info += " | " + std::to_string(static_cast<int>(gcode.totalDistance)) + "mm";
        }
        float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        drawList->AddText(nullptr, 0.0f, ImVec2(textX, thumbMin.y + lineHeight),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), info.c_str(), nullptr, 0.0f,
                          &clipRect);
    } else {
        // List view - compact row
        if (ImGui::Selectable("##gcodeitem", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2(0, 24.0f))) {
            m_selectedGCodeId = gcode.id;
            m_selectedModelId = -1;
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onGCodeOpened)
                    m_onGCodeOpened(gcode.id);
            } else {
                if (m_onGCodeSelected)
                    m_onGCodeSelected(gcode.id);
            }
        }

        if (ImGui::BeginPopupContextItem("GCodeContext")) {
            renderGCodeContextMenu(gcode);
            ImGui::EndPopup();
        }

        // Draw text
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float pad = 4.0f;

        // Name on the left with icon
        std::string label = "GC " + gcode.name;
        float textMaxX = itemMax.x - 70.0f;
        ImVec4 clipRect(itemMin.x + pad, itemMin.y, textMaxX, itemMax.y);
        drawList->AddText(nullptr, 0.0f, ImVec2(itemMin.x + pad, itemMin.y + 3.0f),
                          ImGui::GetColorU32(ImGuiCol_Text), label.c_str(), nullptr, 0.0f,
                          &clipRect);

        // Format on the right
        const char* format = "G-code";
        ImVec2 fmtSize = ImGui::CalcTextSize(format);
        drawList->AddText(ImVec2(itemMax.x - fmtSize.x - pad, itemMin.y + 3.0f),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), format);
    }

    ImGui::PopID();
}

void LibraryPanel::renderGCodeContextMenu(const GCodeRecord& gcode) {
    if (ImGui::MenuItem("Open")) {
        if (m_onGCodeOpened) {
            m_onGCodeOpened(gcode.id);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Delete")) {
        m_showDeleteConfirm = true;
        m_deleteItemId = gcode.id;
        m_deleteIsGCode = true;
        m_deleteItemName = gcode.name;
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer")) {
        auto parentDir = gcode.filePath.parent_path();
        if (!parentDir.empty()) {
#ifdef _WIN32
            ShellExecuteW(nullptr, L"explore",
                          reinterpret_cast<LPCWSTR>(parentDir.u16string().c_str()), nullptr,
                          nullptr, SW_SHOWNORMAL);
#else
            std::string cmd = "xdg-open \"" + parentDir.string() + "\"";
            int ret = std::system(cmd.c_str());
            if (ret != 0) {
                log::warningf("Library", "Failed to open file manager (exit code %d)", ret);
            }
#endif
        }
    }
}

void LibraryPanel::renderDeleteConfirm() {
    if (m_showDeleteConfirm) {
        ImGui::OpenPopup("Delete Item?");
        m_showDeleteConfirm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Delete Item?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete \"%s\"?", m_deleteItemName.c_str());
        ImGui::TextDisabled("This action cannot be undone.");
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (m_library) {
                if (m_deleteIsGCode) {
                    m_library->deleteGCodeFile(m_deleteItemId);
                } else {
                    m_library->removeModel(m_deleteItemId);
                }
                refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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

    if (ImGui::BeginPopupModal("Rename Model", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        ImGui::SetNextItemWidth(-1);

        bool enterPressed =
            ImGui::InputText("##RenameInput", m_renameBuffer, sizeof(m_renameBuffer),
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
                    if (m_library->updateModel(*record)) {
                        refresh();
                    } else {
                        log::error("Library", "Failed to rename model");
                    }
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

} // namespace dw
