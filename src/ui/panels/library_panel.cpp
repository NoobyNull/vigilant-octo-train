#include "library_panel.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/utils/file_utils.h"
#include "../../core/utils/log.h"
#include "../context_menu_manager.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

LibraryPanel::LibraryPanel(LibraryManager* library) : Panel("Library"), m_library(library) {
    m_thumbnailSize = Config::instance().getLibraryThumbSize();
    refresh();
}

LibraryPanel::~LibraryPanel() {
    clearTextureCache();
}

void LibraryPanel::setContextMenuManager(ContextMenuManager* mgr) {
    m_contextMenuManager = mgr;
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

    // No thumbnail path yet — don't cache so we re-check after generation
    if (model.thumbnailPath.empty()) {
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

    // Debounce timer for FTS search
    if (m_searchDirty && m_searchDebounceTimer > 0) {
        m_searchDebounceTimer -= ImGui::GetIO().DeltaTime;
        if (m_searchDebounceTimer <= 0) {
            m_searchDirty = false;
            refresh();
        }
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();
        renderTabs();
        ImGui::Separator();

        // Category breadcrumb when filtering
        renderCategoryBreadcrumb();

        // Side-by-side layout: category sidebar + content
        float availH = ImGui::GetContentRegionAvail().y;

        ImGui::BeginChild("CategorySidebar", ImVec2(160, availH), true);
        renderCategoryFilter();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ContentArea", ImVec2(0, availH), false);
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
        ImGui::EndChild();

        renderRenameDialog();
        renderDeleteConfirm();
        renderCategoryAssignDialog();
    }
    ImGui::End();
}

void LibraryPanel::invalidateThumbnail(int64_t modelId) {
    auto it = m_textureCache.find(modelId);
    if (it != m_textureCache.end()) {
        if (it->second != 0)
            glDeleteTextures(1, &it->second);
        m_textureCache.erase(it);
    }
}

void LibraryPanel::refresh() {
    if (!m_library)
        return;

    // Refresh category cache
    m_categories = m_library->getAllCategories();

    // Determine model list based on search + category filter
    if (!m_searchQuery.empty()) {
        // FTS5 search with BM25 ranking (falls back to LIKE if FTS unavailable)
        if (m_useFTS) {
            m_models = m_library->searchModelsFTS(m_searchQuery);
        } else {
            m_models = m_library->searchModels(m_searchQuery);
        }

        // If also filtering by category, client-side filter the FTS results
        if (m_selectedCategoryId > 0) {
            auto categoryModels = m_library->filterByCategory(m_selectedCategoryId);
            std::set<int64_t> catIds;
            for (auto& m : categoryModels)
                catIds.insert(m.id);
            m_models.erase(std::remove_if(m_models.begin(), m_models.end(),
                                          [&](const ModelRecord& m) {
                                              return catIds.find(m.id) == catIds.end();
                                          }),
                           m_models.end());
        }
    } else if (m_selectedCategoryId > 0) {
        m_models = m_library->filterByCategory(m_selectedCategoryId);
    } else {
        m_models = m_library->getAllModels();
    }

    // G-code files are not affected by category filter
    if (!m_searchQuery.empty()) {
        m_gcodeFiles = m_library->searchGCodeFiles(m_searchQuery);
    } else {
        m_gcodeFiles = m_library->getAllGCodeFiles();
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

    // Zoom slider width (only in grid view)
    float zoomSliderW = m_showThumbnails ? 60.0f : 0.0f;

    float buttonsW = refreshBtnW + viewBtnW + style * 2; // 2 SameLine gaps
    if (m_showThumbnails)
        buttonsW += zoomSliderW + style;

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
        // Debounce: reset timer on each keystroke, refresh fires when timer expires
        m_searchDirty = true;
        m_searchDebounceTimer = 0.2f; // 200ms debounce
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

    // Zoom slider (grid view only)
    if (m_showThumbnails) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(zoomSliderW);
        ImGui::SliderFloat("##Zoom", &m_thumbnailSize, THUMB_MIN, avail, "",
                           ImGuiSliderFlags_NoRoundToFormat);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Config::instance().setLibraryThumbSize(m_thumbnailSize);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Thumbnail size (%.0fpx)", m_thumbnailSize);
        }
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

void LibraryPanel::renderCategoryFilter() {
    // "All Models" button to clear filter
    bool allSelected = (m_selectedCategoryId == -1);
    if (ImGui::Selectable("All Models", allSelected)) {
        m_selectedCategoryId = -1;
        m_selectedCategoryName.clear();
        refresh();
    }

    ImGui::Separator();

    // Build category tree (2-level max)
    // Roots: categories with no parent
    for (auto& cat : m_categories) {
        if (cat.parentId.has_value())
            continue; // skip children in root loop

        bool isSelected = (m_selectedCategoryId == cat.id);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Check if this root has children
        bool hasChildren = false;
        for (auto& child : m_categories) {
            if (child.parentId.has_value() && *child.parentId == cat.id) {
                hasChildren = true;
                break;
            }
        }
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool open = ImGui::TreeNodeEx(cat.name.c_str(), flags);
        if (ImGui::IsItemClicked()) {
            m_selectedCategoryId = cat.id;
            m_selectedCategoryName = cat.name;
            refresh();
        }
        if (open) {
            for (auto& child : m_categories) {
                if (!child.parentId.has_value() || *child.parentId != cat.id)
                    continue;
                bool childSelected = (m_selectedCategoryId == child.id);
                if (ImGui::Selectable(child.name.c_str(), childSelected)) {
                    m_selectedCategoryId = child.id;
                    m_selectedCategoryName = cat.name + " > " + child.name;
                    refresh();
                }
            }
            ImGui::TreePop();
        }
    }
}

void LibraryPanel::renderCategoryBreadcrumb() {
    if (m_selectedCategoryId <= 0)
        return;

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Category: %s",
                       m_selectedCategoryName.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("x##clearCat")) {
        m_selectedCategoryId = -1;
        m_selectedCategoryName.clear();
        refresh();
    }
    ImGui::Separator();
}

void LibraryPanel::renderCategoryAssignDialog() {
    if (m_showCategoryAssignDialog) {
        ImGui::OpenPopup("Assign Category");
        m_showCategoryAssignDialog = false;

        // Initialize: load which categories the selected model(s) belong to
        m_assignedCategoryIds.clear();
        if (m_library && m_selectedModelIds.size() == 1) {
            int64_t modelId = *m_selectedModelIds.begin();
            // Check each category to see if this model is in it
            for (auto& cat : m_categories) {
                auto models = m_library->filterByCategory(cat.id);
                for (auto& m : models) {
                    if (m.id == modelId) {
                        m_assignedCategoryIds.insert(cat.id);
                        break;
                    }
                }
            }
        }
        m_newCategoryName[0] = '\0';
        m_newCategoryParent = -1;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Assign Category", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select categories for model:");
        ImGui::Separator();

        ImGui::BeginChild("CatList", ImVec2(0, 250), true);

        // Show categories as checkable tree
        for (auto& cat : m_categories) {
            if (cat.parentId.has_value())
                continue; // roots only at top level

            bool checked = m_assignedCategoryIds.count(cat.id) > 0;
            if (ImGui::Checkbox(cat.name.c_str(), &checked)) {
                if (checked)
                    m_assignedCategoryIds.insert(cat.id);
                else
                    m_assignedCategoryIds.erase(cat.id);
            }

            // Show children indented
            ImGui::Indent(20.0f);
            for (auto& child : m_categories) {
                if (!child.parentId.has_value() || *child.parentId != cat.id)
                    continue;
                bool childChecked = m_assignedCategoryIds.count(child.id) > 0;
                if (ImGui::Checkbox(child.name.c_str(), &childChecked)) {
                    if (childChecked)
                        m_assignedCategoryIds.insert(child.id);
                    else
                        m_assignedCategoryIds.erase(child.id);
                }
            }
            ImGui::Unindent(20.0f);
        }

        ImGui::EndChild();

        // Quick add new category
        ImGui::Separator();
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##NewCat", "New category name...", m_newCategoryName,
                                 sizeof(m_newCategoryName));
        ImGui::SameLine();
        if (ImGui::Button("Add") && m_newCategoryName[0] != '\0') {
            std::optional<int64_t> parentId;
            if (m_newCategoryParent > 0)
                parentId = m_newCategoryParent;
            auto newId = m_library->createCategory(m_newCategoryName, parentId);
            if (newId) {
                m_categories = m_library->getAllCategories();
                m_assignedCategoryIds.insert(*newId);
            }
            m_newCategoryName[0] = '\0';
        }

        ImGui::Separator();
        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            // Apply category assignments for selected model(s)
            if (m_library) {
                for (int64_t modelId : m_selectedModelIds) {
                    // Remove categories no longer checked
                    for (auto& cat : m_categories) {
                        bool wasAssigned = false;
                        auto models = m_library->filterByCategory(cat.id);
                        for (auto& m : models) {
                            if (m.id == modelId) {
                                wasAssigned = true;
                                break;
                            }
                        }
                        bool isNowAssigned = m_assignedCategoryIds.count(cat.id) > 0;

                        if (wasAssigned && !isNowAssigned) {
                            m_library->removeModelCategory(modelId, cat.id);
                        } else if (!wasAssigned && isNowAssigned) {
                            m_library->assignCategory(modelId, cat.id);
                        }
                    }
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

void LibraryPanel::renderModelList() {
    ImGui::BeginChild("ModelList", ImVec2(0, 0), false);

    // Ctrl+scroll to zoom thumbnails in grid view
    if (m_showThumbnails && ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float maxThumb = ImGui::GetContentRegionAvail().x;
            m_thumbnailSize = std::clamp(m_thumbnailSize + wheel * 16.0f, THUMB_MIN, maxThumb);
            Config::instance().setLibraryThumbSize(m_thumbnailSize);
            ImGui::GetIO().MouseWheel = 0.0f; // consume so child doesn't scroll
        }
    }

    if (m_models.empty()) {
        ImGui::TextDisabled("No models in library");
        ImGui::TextDisabled("Import models using File > Import");
    } else if (m_showThumbnails) {
        // True grid layout with spill
        float availW = ImGui::GetContentRegionAvail().x;
        float pad = 4.0f;
        float cellBase = m_thumbnailSize + pad;
        int columns = std::max(1, static_cast<int>(availW / cellBase));
        // Spill: distribute leftover space evenly into each cell
        float cellW = availW / columns;
        float thumbSize = cellW - pad;

        int col = 0;
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i), thumbSize);
            ++col;
            if (col < columns) {
                ImGui::SameLine(0.0f, 0.0f);
            } else {
                col = 0;
            }
        }
    } else {
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderModelItem(const ModelRecord& model, [[maybe_unused]] int index,
                                   float thumbOverride) {
    ImGui::PushID(static_cast<int>(model.id));

    bool isSelected = m_selectedModelIds.count(model.id) > 0;

    if (m_showThumbnails) {
        // Grid cell: thumbnail with name below, details on hover
        float ts = thumbOverride > 0.0f ? thumbOverride : m_thumbnailSize;
        float pad = 2.0f;
        float nameH = ImGui::GetTextLineHeightWithSpacing();
        float cellH = ts + nameH + pad * 2;

        if (ImGui::Selectable("##item", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick |
                                  ImGuiSelectableFlags_DontClosePopups,
                              ImVec2(ts + pad * 2, cellH))) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onModelOpened)
                    m_onModelOpened(model.id);
            } else if (ImGui::GetIO().KeyCtrl) {
                // Ctrl+click: toggle this item
                if (m_selectedModelIds.count(model.id))
                    m_selectedModelIds.erase(model.id);
                else
                    m_selectedModelIds.insert(model.id);
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            } else if (ImGui::GetIO().KeyShift && m_lastClickedModelId != -1) {
                // Shift+click: range select by visible order
                m_selectedModelIds.clear();
                bool inRange = false;
                for (auto& m : m_models) {
                    if (m.id == model.id || m.id == m_lastClickedModelId)
                        inRange = !inRange ? true : false;
                    if (inRange || m.id == model.id || m.id == m_lastClickedModelId)
                        m_selectedModelIds.insert(m.id);
                }
                m_selectedGCodeIds.clear();
            } else {
                m_selectedModelIds = {model.id};
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            }
            if (m_onModelSelected)
                m_onModelSelected(model.id);
        }

        // Right-click: ensure clicked item is in selection
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (!m_selectedModelIds.count(model.id)) {
                m_selectedModelIds = {model.id};
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            }
        }

        m_currentContextMenuModel = model;
        registerContextMenuEntries();
        if (ImGui::BeginPopupContextItem("LibraryPanel_ModelContext")) {
            m_contextMenuManager->render("LibraryPanel_ModelContext");
            ImGui::EndPopup();
        }
        m_currentContextMenuModel = std::nullopt;

        // Delayed hover tooltip with details
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            std::string tip = model.name + "\n" + model.fileFormat;
            if (model.triangleCount > 0) {
                if (model.triangleCount >= 1000000)
                    tip += " | " + std::to_string(model.triangleCount / 1000000) + "M tris";
                else if (model.triangleCount >= 1000)
                    tip += " | " + std::to_string(model.triangleCount / 1000) + "K tris";
                else
                    tip += " | " + std::to_string(model.triangleCount) + " tris";
            }
            if (model.vertexCount > 0) {
                if (model.vertexCount >= 1000000)
                    tip += "\n" + std::to_string(model.vertexCount / 1000000) + "M verts";
                else if (model.vertexCount >= 1000)
                    tip += "\n" + std::to_string(model.vertexCount / 1000) + "K verts";
                else
                    tip += "\n" + std::to_string(model.vertexCount) + " verts";
            }
            ImGui::SetTooltip("%s", tip.c_str());
        }

        // Draw thumbnail over selectable
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 thumbMin = ImVec2(itemMin.x + pad, itemMin.y + pad);
        ImVec2 thumbMax = ImVec2(thumbMin.x + ts, thumbMin.y + ts);

        GLuint tex = getThumbnailTexture(model);
        if (tex != 0) {
            drawList->AddImageRounded((ImTextureID)(intptr_t)tex, thumbMin, thumbMax, ImVec2(0, 0),
                                      ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 4.0f);
        } else {
            drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 60, 255), 4.0f);
            drawList->AddRect(thumbMin, thumbMax, IM_COL32(80, 80, 80, 255), 4.0f);

            // Center icon in placeholder
            const char* icon = Icons::Model;
            ImVec2 iconSize = ImGui::CalcTextSize(icon);
            ImVec2 iconPos = ImVec2(thumbMin.x + (ts - iconSize.x) * 0.5f,
                                    thumbMin.y + (ts - iconSize.y) * 0.5f);
            drawList->AddText(iconPos, IM_COL32(128, 128, 128, 255), icon);
        }

        // Selection highlight
        if (isSelected) {
            drawList->AddRect(thumbMin, thumbMax, ImGui::GetColorU32(ImGuiCol_ButtonActive), 4.0f,
                              0, 2.0f);
        }

        // Name below thumbnail (clipped to cell width)
        ImVec2 namePos = ImVec2(itemMin.x + pad, thumbMax.y + 1.0f);
        ImVec4 clipRect(namePos.x, namePos.y, namePos.x + ts, namePos.y + nameH);
        drawList->AddText(nullptr, 0.0f, namePos, ImGui::GetColorU32(ImGuiCol_Text),
                          model.name.c_str(), nullptr, ts, &clipRect);
    } else {
        // List view - compact row
        if (ImGui::Selectable("##item", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick |
                                  ImGuiSelectableFlags_DontClosePopups,
                              ImVec2(0, 24.0f))) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onModelOpened)
                    m_onModelOpened(model.id);
            } else if (ImGui::GetIO().KeyCtrl) {
                if (m_selectedModelIds.count(model.id))
                    m_selectedModelIds.erase(model.id);
                else
                    m_selectedModelIds.insert(model.id);
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            } else if (ImGui::GetIO().KeyShift && m_lastClickedModelId != -1) {
                m_selectedModelIds.clear();
                bool inRange = false;
                for (auto& m : m_models) {
                    if (m.id == model.id || m.id == m_lastClickedModelId)
                        inRange = !inRange ? true : false;
                    if (inRange || m.id == model.id || m.id == m_lastClickedModelId)
                        m_selectedModelIds.insert(m.id);
                }
                m_selectedGCodeIds.clear();
            } else {
                m_selectedModelIds = {model.id};
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            }
            if (m_onModelSelected)
                m_onModelSelected(model.id);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (!m_selectedModelIds.count(model.id)) {
                m_selectedModelIds = {model.id};
                m_lastClickedModelId = model.id;
                m_selectedGCodeIds.clear();
            }
        }

        m_currentContextMenuModel = model;
        registerContextMenuEntries();
        if (ImGui::BeginPopupContextItem("LibraryPanel_ModelContext")) {
            m_contextMenuManager->render("LibraryPanel_ModelContext");
            ImGui::EndPopup();
        }
        m_currentContextMenuModel = std::nullopt;

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

void LibraryPanel::registerContextMenuEntries() {
    if (!m_contextMenuManager) {
        return;
    }

    // Model context menu — labels reflect current selection count
    size_t modelCount = m_selectedModelIds.size();
    bool multi = modelCount > 1;
    std::string countSuffix = multi ? " (" + std::to_string(modelCount) + ")" : "";

    std::vector<ContextMenuEntry> modelEntries = {
        {"Open",
         [this]() {
             if (m_onModelOpened && m_currentContextMenuModel) {
                 m_onModelOpened(m_currentContextMenuModel->id);
             }
         }},
        {"Regenerate Thumbnail" + (multi ? "s" + countSuffix : ""),
         [this]() {
             if (!m_onRegenerateThumbnail)
                 return;
             std::vector<int64_t> ids(m_selectedModelIds.begin(), m_selectedModelIds.end());
             m_onRegenerateThumbnail(ids);
         }},
        {
            "Assign Default Material" + countSuffix,
            [this]() {
                if (!m_onAssignDefaultMaterial)
                    return;
                for (int64_t id : m_selectedModelIds) {
                    m_onAssignDefaultMaterial(id);
                }
            },
            "",                                                                // icon
            [this]() { return Config::instance().getDefaultMaterialId() > 0; } // enabled
        },
        {"Assign Category" + countSuffix, [this]() { m_showCategoryAssignDialog = true; }},
        ContextMenuEntry::separator(),
        {"Rename",
         [this]() {
             if (m_currentContextMenuModel) {
                 m_showRenameDialog = true;
                 m_renameModelId = m_currentContextMenuModel->id;
                 std::strncpy(m_renameBuffer, m_currentContextMenuModel->name.c_str(),
                              sizeof(m_renameBuffer) - 1);
                 m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
             }
         },
         "",                                                   // icon
         [this]() { return m_selectedModelIds.size() == 1; }}, // only for single selection
        {"Delete" + countSuffix,
         [this]() {
             m_showDeleteConfirm = true;
             m_deleteIsGCode = false;
             m_deleteItemIds.assign(m_selectedModelIds.begin(), m_selectedModelIds.end());
             if (m_deleteItemIds.size() == 1 && m_currentContextMenuModel) {
                 m_deleteItemName = m_currentContextMenuModel->name;
             } else {
                 m_deleteItemName = std::to_string(m_deleteItemIds.size()) + " items";
             }
         }},
        ContextMenuEntry::separator(),
        {"Show in Explorer",
         [this]() {
             if (m_currentContextMenuModel) {
                 auto parentDir = m_currentContextMenuModel->filePath.parent_path();
                 if (!parentDir.empty()) {
                     file::openInFileManager(parentDir);
                 }
             }
         }},
        {"Copy Path",
         [this]() {
             if (m_selectedModelIds.size() > 1) {
                 // Copy newline-separated paths for multi-select
                 std::string paths;
                 for (int64_t id : m_selectedModelIds) {
                     for (auto& m : m_models) {
                         if (m.id == id) {
                             if (!paths.empty())
                                 paths += "\n";
                             paths += m.filePath.string();
                             break;
                         }
                     }
                 }
                 ImGui::SetClipboardText(paths.c_str());
             } else if (m_currentContextMenuModel) {
                 ImGui::SetClipboardText(m_currentContextMenuModel->filePath.string().c_str());
             }
         }},
    };

    // GCode context menu
    size_t gcodeCount = m_selectedGCodeIds.size();
    bool gcodeMulti = gcodeCount > 1;
    std::string gcodeCountSuffix = gcodeMulti ? " (" + std::to_string(gcodeCount) + ")" : "";

    std::vector<ContextMenuEntry> gcodeEntries = {
        {"Open",
         [this]() {
             if (m_onGCodeOpened && m_currentContextMenuGCode) {
                 m_onGCodeOpened(m_currentContextMenuGCode->id);
             }
         }},
        ContextMenuEntry::separator(),
        {"Delete" + gcodeCountSuffix,
         [this]() {
             m_showDeleteConfirm = true;
             m_deleteIsGCode = true;
             m_deleteItemIds.assign(m_selectedGCodeIds.begin(), m_selectedGCodeIds.end());
             if (m_deleteItemIds.size() == 1 && m_currentContextMenuGCode) {
                 m_deleteItemName = m_currentContextMenuGCode->name;
             } else {
                 m_deleteItemName = std::to_string(m_deleteItemIds.size()) + " items";
             }
         }},
        ContextMenuEntry::separator(),
        {"Show in Explorer",
         [this]() {
             if (m_currentContextMenuGCode) {
                 auto parentDir = m_currentContextMenuGCode->filePath.parent_path();
                 if (!parentDir.empty()) {
                     file::openInFileManager(parentDir);
                 }
             }
         }},
    };

    m_contextMenuManager->registerEntries("LibraryPanel_ModelContext", modelEntries);
    m_contextMenuManager->registerEntries("LibraryPanel_GCodeContext", gcodeEntries);
}

void LibraryPanel::renderGCodeList() {
    ImGui::BeginChild("GCodeList", ImVec2(0, 0), false);

    // Ctrl+scroll to zoom thumbnails in grid view
    if (m_showThumbnails && ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float maxThumb = ImGui::GetContentRegionAvail().x;
            m_thumbnailSize = std::clamp(m_thumbnailSize + wheel * 16.0f, THUMB_MIN, maxThumb);
            Config::instance().setLibraryThumbSize(m_thumbnailSize);
            ImGui::GetIO().MouseWheel = 0.0f; // consume so child doesn't scroll
        }
    }

    if (m_gcodeFiles.empty()) {
        ImGui::TextDisabled("No G-code files in library");
        ImGui::TextDisabled("Import G-code using File > Import");
    } else if (m_showThumbnails) {
        float availW = ImGui::GetContentRegionAvail().x;
        float pad = 4.0f;
        float cellBase = m_thumbnailSize + pad;
        int columns = std::max(1, static_cast<int>(availW / cellBase));
        float cellW = availW / columns;
        float thumbSize = cellW - pad;

        int col = 0;
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i), thumbSize);
            ++col;
            if (col < columns) {
                ImGui::SameLine(0.0f, 0.0f);
            } else {
                col = 0;
            }
        }
    } else {
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderCombinedList() {
    ImGui::BeginChild("CombinedList", ImVec2(0, 0), false);

    // Ctrl+scroll to zoom thumbnails in grid view
    if (m_showThumbnails && ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float maxThumb = ImGui::GetContentRegionAvail().x;
            m_thumbnailSize = std::clamp(m_thumbnailSize + wheel * 16.0f, THUMB_MIN, maxThumb);
            Config::instance().setLibraryThumbSize(m_thumbnailSize);
            ImGui::GetIO().MouseWheel = 0.0f; // consume so child doesn't scroll
        }
    }

    if (m_models.empty() && m_gcodeFiles.empty()) {
        ImGui::TextDisabled("Library is empty");
        ImGui::TextDisabled("Import files using File > Import");
    } else if (m_showThumbnails) {
        float availW = ImGui::GetContentRegionAvail().x;
        float pad = 4.0f;
        float cellBase = m_thumbnailSize + pad;
        int columns = std::max(1, static_cast<int>(availW / cellBase));
        float cellW = availW / columns;
        float thumbSize = cellW - pad;

        int col = 0;
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i), thumbSize);
            ++col;
            if (col < columns) {
                ImGui::SameLine(0.0f, 0.0f);
            } else {
                col = 0;
            }
        }
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i + m_models.size()), thumbSize);
            ++col;
            if (col < columns) {
                ImGui::SameLine(0.0f, 0.0f);
            } else {
                col = 0;
            }
        }
    } else {
        for (size_t i = 0; i < m_models.size(); ++i) {
            renderModelItem(m_models[i], static_cast<int>(i));
        }
        for (size_t i = 0; i < m_gcodeFiles.size(); ++i) {
            renderGCodeItem(m_gcodeFiles[i], static_cast<int>(i + m_models.size()));
        }
    }

    ImGui::EndChild();
}

void LibraryPanel::renderGCodeItem(const GCodeRecord& gcode, [[maybe_unused]] int index,
                                   float thumbOverride) {
    ImGui::PushID(static_cast<int>(gcode.id + 1000000)); // Offset to avoid ID collision with models

    bool isSelected = m_selectedGCodeIds.count(gcode.id) > 0;

    if (m_showThumbnails) {
        // Grid cell: placeholder with name below, details on hover
        float ts = thumbOverride > 0.0f ? thumbOverride : m_thumbnailSize;
        float pad = 2.0f;
        float nameH = ImGui::GetTextLineHeightWithSpacing();
        float cellH = ts + nameH + pad * 2;

        if (ImGui::Selectable("##gcodeitem", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick |
                                  ImGuiSelectableFlags_DontClosePopups,
                              ImVec2(ts + pad * 2, cellH))) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onGCodeOpened)
                    m_onGCodeOpened(gcode.id);
            } else if (ImGui::GetIO().KeyCtrl) {
                if (m_selectedGCodeIds.count(gcode.id))
                    m_selectedGCodeIds.erase(gcode.id);
                else
                    m_selectedGCodeIds.insert(gcode.id);
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            } else if (ImGui::GetIO().KeyShift && m_lastClickedGCodeId != -1) {
                m_selectedGCodeIds.clear();
                bool inRange = false;
                for (auto& g : m_gcodeFiles) {
                    if (g.id == gcode.id || g.id == m_lastClickedGCodeId)
                        inRange = !inRange ? true : false;
                    if (inRange || g.id == gcode.id || g.id == m_lastClickedGCodeId)
                        m_selectedGCodeIds.insert(g.id);
                }
                m_selectedModelIds.clear();
            } else {
                m_selectedGCodeIds = {gcode.id};
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            }
            if (m_onGCodeSelected)
                m_onGCodeSelected(gcode.id);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (!m_selectedGCodeIds.count(gcode.id)) {
                m_selectedGCodeIds = {gcode.id};
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            }
        }

        m_currentContextMenuGCode = gcode;
        registerContextMenuEntries();
        if (ImGui::BeginPopupContextItem("LibraryPanel_GCodeContext")) {
            m_contextMenuManager->render("LibraryPanel_GCodeContext");
            ImGui::EndPopup();
        }
        m_currentContextMenuGCode = std::nullopt;

        // Delayed hover tooltip with details
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            std::string tip = gcode.name + "\nG-code";
            if (gcode.estimatedTime > 0) {
                int minutes = static_cast<int>(gcode.estimatedTime);
                tip += " | " + std::to_string(minutes) + "min";
            }
            if (gcode.totalDistance > 0) {
                tip += " | " + std::to_string(static_cast<int>(gcode.totalDistance)) + "mm";
            }
            ImGui::SetTooltip("%s", tip.c_str());
        }

        // Draw placeholder
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 thumbMin = ImVec2(itemMin.x + pad, itemMin.y + pad);
        ImVec2 thumbMax = ImVec2(thumbMin.x + ts, thumbMin.y + ts);

        drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(50, 70, 90, 255), 4.0f);
        drawList->AddRect(thumbMin, thumbMax, IM_COL32(70, 100, 130, 255), 4.0f);

        // Center "GC" text in placeholder
        const char* icon = "GC";
        ImVec2 iconSize = ImGui::CalcTextSize(icon);
        ImVec2 iconPos =
            ImVec2(thumbMin.x + (ts - iconSize.x) * 0.5f, thumbMin.y + (ts - iconSize.y) * 0.5f);
        drawList->AddText(iconPos, IM_COL32(150, 180, 220, 255), icon);

        // Selection highlight
        if (isSelected) {
            drawList->AddRect(thumbMin, thumbMax, ImGui::GetColorU32(ImGuiCol_ButtonActive), 4.0f,
                              0, 2.0f);
        }

        // Name below placeholder (clipped)
        ImVec2 namePos = ImVec2(itemMin.x + pad, thumbMax.y + 1.0f);
        ImVec4 clipRect(namePos.x, namePos.y, namePos.x + ts, namePos.y + nameH);
        drawList->AddText(nullptr, 0.0f, namePos, ImGui::GetColorU32(ImGuiCol_Text),
                          gcode.name.c_str(), nullptr, ts, &clipRect);
    } else {
        // List view - compact row
        if (ImGui::Selectable("##gcodeitem", isSelected,
                              ImGuiSelectableFlags_AllowDoubleClick |
                                  ImGuiSelectableFlags_DontClosePopups,
                              ImVec2(0, 24.0f))) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onGCodeOpened)
                    m_onGCodeOpened(gcode.id);
            } else if (ImGui::GetIO().KeyCtrl) {
                if (m_selectedGCodeIds.count(gcode.id))
                    m_selectedGCodeIds.erase(gcode.id);
                else
                    m_selectedGCodeIds.insert(gcode.id);
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            } else if (ImGui::GetIO().KeyShift && m_lastClickedGCodeId != -1) {
                m_selectedGCodeIds.clear();
                bool inRange = false;
                for (auto& g : m_gcodeFiles) {
                    if (g.id == gcode.id || g.id == m_lastClickedGCodeId)
                        inRange = !inRange ? true : false;
                    if (inRange || g.id == gcode.id || g.id == m_lastClickedGCodeId)
                        m_selectedGCodeIds.insert(g.id);
                }
                m_selectedModelIds.clear();
            } else {
                m_selectedGCodeIds = {gcode.id};
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            }
            if (m_onGCodeSelected)
                m_onGCodeSelected(gcode.id);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (!m_selectedGCodeIds.count(gcode.id)) {
                m_selectedGCodeIds = {gcode.id};
                m_lastClickedGCodeId = gcode.id;
                m_selectedModelIds.clear();
            }
        }

        m_currentContextMenuGCode = gcode;
        registerContextMenuEntries();
        if (ImGui::BeginPopupContextItem("LibraryPanel_GCodeContext")) {
            m_contextMenuManager->render("LibraryPanel_GCodeContext");
            ImGui::EndPopup();
        }
        m_currentContextMenuGCode = std::nullopt;

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
                for (int64_t id : m_deleteItemIds) {
                    if (m_deleteIsGCode) {
                        m_library->deleteGCodeFile(id);
                    } else {
                        m_library->removeModel(id);
                    }
                }
                size_t count = m_deleteItemIds.size();
                if (m_deleteIsGCode) {
                    ToastManager::instance().show(ToastType::Success, "Deleted",
                                                  count == 1 ? "G-code file deleted successfully"
                                                             : std::to_string(count) +
                                                                   " G-code files deleted");
                } else {
                    ToastManager::instance().show(ToastType::Success, "Deleted",
                                                  count == 1
                                                      ? "Model deleted successfully"
                                                      : std::to_string(count) + " models deleted");
                }
                // Clear selection for deleted items
                if (m_deleteIsGCode) {
                    m_selectedGCodeIds.clear();
                    m_lastClickedGCodeId = -1;
                } else {
                    m_selectedModelIds.clear();
                    m_lastClickedModelId = -1;
                }
                refresh();
            } else {
                ToastManager::instance().show(ToastType::Error, "Delete Failed",
                                              "Could not delete item");
            }
            m_deleteItemIds.clear();
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
            // Trim whitespace
            newName.erase(0, newName.find_first_not_of(" \t\n\r"));
            newName.erase(newName.find_last_not_of(" \t\n\r") + 1);

            if (newName.empty()) {
                ToastManager::instance().show(ToastType::Warning, "Invalid Name",
                                              "Name cannot be empty");
            } else if (m_library) {
                auto record = m_library->getModel(m_renameModelId);
                if (record) {
                    record->name = newName;
                    if (m_library->updateModel(*record)) {
                        refresh();
                        ToastManager::instance().show(ToastType::Success, "Renamed",
                                                      "Model renamed successfully");
                    } else {
                        ToastManager::instance().show(ToastType::Error, "Rename Failed",
                                                      "Could not rename model");
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
