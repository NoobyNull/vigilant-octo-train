#include "materials_panel.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/loaders/texture_loader.h"
#include "../../core/materials/material_archive.h"
#include "../../core/utils/log.h"
#include "../context_menu_manager.h"
#include "../icons.h"

namespace dw {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

MaterialsPanel::MaterialsPanel(MaterialManager* materialManager)
    : Panel("Materials"), m_materialManager(materialManager) {
    refresh();
}

MaterialsPanel::~MaterialsPanel() {
    clearTextureCache();
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void MaterialsPanel::refresh() {
    if (!m_materialManager) {
        return;
    }
    m_allMaterials = m_materialManager->getAllMaterials();
}

void MaterialsPanel::render() {
    if (!m_open) {
        return;
    }

    // Lazy initialize context menu manager on first render
    if (m_contextMenuManager == nullptr) {
        // This will be set by UIManager after panel initialization
        // For now, we create a placeholder that will be replaced
        // registerContextMenuEntries will be called once manager is available
    }

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();
        renderCategoryTabs();
        renderEditForm();
        renderDeleteConfirm();
    }
    ImGui::End();
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------

void MaterialsPanel::renderToolbar() {
    // Import button
    if (ImGui::Button(Icons::Import)) {
        // File dialog for .dwmat import.
        // In production this would open a native file dialog; here we log intent.
        log::info("MaterialsPanel", "Import material requested (file dialog not yet wired)");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Import material (.dwmat)");
    }

    ImGui::SameLine();

    // Export button (only active when a material is selected)
    bool hasSelection = (m_selectedMaterialId != -1);
    if (!hasSelection) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(Icons::Export)) {
        if (hasSelection && m_materialManager) {
            log::info("MaterialsPanel", "Export material requested (file dialog not yet wired)");
        }
    }
    if (!hasSelection) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(hasSelection ? "Export selected material (.dwmat)"
                                       : "Select a material to export");
    }

    ImGui::SameLine();

    // Add new material button
    if (ImGui::Button(Icons::Add)) {
        m_editBuffer = MaterialRecord{};
        m_editBuffer.name = "New Material";
        m_isNewMaterial = true;
        m_showEditForm = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add new material");
    }

    ImGui::SameLine();

    // Delete selected material button
    if (!hasSelection) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(Icons::Delete)) {
        if (hasSelection) {
            // Find name for confirmation dialog
            for (const auto& mat : m_allMaterials) {
                if (mat.id == m_selectedMaterialId) {
                    m_deleteName = mat.name;
                    break;
                }
            }
            m_deleteId = m_selectedMaterialId;
            m_showDeleteConfirm = true;
        }
    }
    if (!hasSelection) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(hasSelection ? "Delete selected material"
                                       : "Select a material to delete");
    }

    ImGui::SameLine();

    // Assign to model button (only active when material selected AND model loaded)
    bool canAssign = hasSelection && m_modelLoaded;
    if (!canAssign) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(Icons::Assign)) {
        if (canAssign && m_onMaterialAssigned) {
            m_onMaterialAssigned(m_selectedMaterialId);
        }
    }
    if (!canAssign) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (!hasSelection) {
            ImGui::SetTooltip("Select a material first");
        } else if (!m_modelLoaded) {
            ImGui::SetTooltip("Load a model into the viewport first");
        } else {
            ImGui::SetTooltip("Assign selected material to loaded model");
        }
    }

    ImGui::SameLine();

    // Refresh button
    if (ImGui::Button(Icons::Refresh)) {
        refresh();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh materials list");
    }

    ImGui::SameLine();

    // Thumbnail size slider
    ImGui::SetNextItemWidth(60.0f);
    ImGui::SliderFloat("##ThumbSize", &m_thumbnailSize, THUMB_MIN, 256.0f, "%.0f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Thumbnail size (Ctrl+scroll in grid)");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Generate material section
    if (m_isGenerating) {
        ImGui::BeginDisabled();
    }
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputTextWithHint("##GenPrompt", "Material...", m_generatePrompt,
                             sizeof(m_generatePrompt));
    ImGui::SameLine();
    bool promptEmpty = (m_generatePrompt[0] == '\0');
    if (promptEmpty && !m_isGenerating) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(m_isGenerating ? "..." : Icons::Wand)) {
        if (m_onGenerate && !promptEmpty) {
            m_isGenerating = true;
            m_onGenerate(std::string(m_generatePrompt));
        }
    }
    if (promptEmpty && !m_isGenerating) {
        ImGui::EndDisabled();
    }
    if (m_isGenerating) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (m_isGenerating) {
            ImGui::SetTooltip("Generating material via Gemini AI...");
        } else if (promptEmpty) {
            ImGui::SetTooltip("Enter a material name to generate");
        } else {
            ImGui::SetTooltip("Generate material texture and properties via AI");
        }
    }

    ImGui::SameLine();

    // Search filter — takes remaining width
    float availW = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(availW > 50.0f ? availW : 50.0f);
    if (ImGui::InputTextWithHint(
            "##MatSearch", "Search...", m_searchQuery.data(), 256,
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
        // Filtering happens in renderCategoryTabs via m_searchQuery
    }
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

void MaterialsPanel::registerContextMenuEntries() {
    if (!m_contextMenuManager) {
        return;
    }

    std::vector<ContextMenuEntry> materialEntries = {
        {.label = "Edit",
         .action =
             [this]() {
                 if (m_currentContextMenuMaterial) {
                     m_editBuffer = *m_currentContextMenuMaterial;
                     m_isNewMaterial = false;
                     m_showEditForm = true;
                 }
             }},
        {.label = "Export",
         .action =
             [this]() {
                 if (m_currentContextMenuMaterial) {
                     log::info("MaterialsPanel",
                               "Export material requested (file dialog not yet wired)");
                 }
             }},
        ContextMenuEntry::separator(),
        {.label = "Set as Default Material",
         .action =
             [this]() {
                 if (m_currentContextMenuMaterial) {
                     Config::instance().setDefaultMaterialId(m_currentContextMenuMaterial->id);
                     Config::instance().save();
                 }
             }},
        ContextMenuEntry::separator(),
        {.label = "Delete",
         .action =
             [this]() {
                 if (m_currentContextMenuMaterial) {
                     m_deleteId = m_currentContextMenuMaterial->id;
                     m_deleteName = m_currentContextMenuMaterial->name;
                     m_showDeleteConfirm = true;
                 }
             }},
    };
    m_contextMenuManager->registerEntries("MaterialsPanel_MaterialContext", materialEntries);
}

// ---------------------------------------------------------------------------
// Category tabs
// ---------------------------------------------------------------------------

void MaterialsPanel::renderCategoryTabs() {
    if (ImGui::BeginTabBar("MaterialCategories")) {
        auto makeTab = [&](const char* label, const char* tooltip, CategoryTab tab,
                           std::vector<MaterialRecord> materials) {
            bool selected = ImGui::BeginTabItem(label);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tooltip);
            }
            if (selected) {
                m_activeCategory = tab;
                renderMaterialGrid(materials);
                ImGui::EndTabItem();
            }
        };

        // Pre-filter all materials by search query
        std::vector<MaterialRecord> filtered;
        filtered.reserve(m_allMaterials.size());
        for (const auto& mat : m_allMaterials) {
            if (m_searchQuery.empty()) {
                filtered.push_back(mat);
            } else {
                // Case-insensitive substring match
                std::string nameLower = mat.name;
                std::string queryLower = m_searchQuery;
                for (auto& c : nameLower) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                for (auto& c : queryLower) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (nameLower.find(queryLower) != std::string::npos) {
                    filtered.push_back(mat);
                }
            }
        }

        // Build per-category lists
        auto byCategory = [&](MaterialCategory cat) {
            std::vector<MaterialRecord> result;
            for (const auto& mat : filtered) {
                if (mat.category == cat) {
                    result.push_back(mat);
                }
            }
            return result;
        };

        makeTab("All", "All materials", CategoryTab::All, filtered);
        makeTab("HW", "Hardwood", CategoryTab::Hardwood, byCategory(MaterialCategory::Hardwood));
        makeTab("SW", "Softwood", CategoryTab::Softwood, byCategory(MaterialCategory::Softwood));
        makeTab("Dom", "Domestic", CategoryTab::Domestic, byCategory(MaterialCategory::Domestic));
        makeTab("Cmp", "Composite", CategoryTab::Composite,
                byCategory(MaterialCategory::Composite));

        ImGui::EndTabBar();
    }
}

// ---------------------------------------------------------------------------
// Material grid
// ---------------------------------------------------------------------------

void MaterialsPanel::renderMaterialGrid(const std::vector<MaterialRecord>& materials) {
    ImGui::BeginChild("MatGrid", ImVec2(0, 0), false);

    // Ctrl+scroll to resize thumbnails
    if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f) {
        m_thumbnailSize += ImGui::GetIO().MouseWheel * 16.0f;
        float maxSize = ImGui::GetContentRegionAvail().x;
        m_thumbnailSize = std::clamp(m_thumbnailSize, THUMB_MIN, std::max(THUMB_MIN, maxSize));
    }

    if (materials.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("No materials found.");
        if (!m_searchQuery.empty()) {
            ImGui::TextDisabled("Try clearing the search filter.");
        }
        ImGui::EndChild();
        return;
    }

    const float cellSize = m_thumbnailSize + 16.0f; // padding around thumbnail
    float availW = ImGui::GetContentRegionAvail().x;
    int columns = static_cast<int>(availW / cellSize);
    if (columns < 1) {
        columns = 1;
    }

    int col = 0;
    for (const auto& mat : materials) {
        ImGui::PushID(static_cast<int>(mat.id));

        bool isSelected = (mat.id == m_selectedMaterialId);

        // Group for each material cell
        ImGui::BeginGroup();

        // Invisible selectable underneath the cell (allows click + double-click detection)
        float cellH = m_thumbnailSize + ImGui::GetTextLineHeightWithSpacing() + 8.0f;
        bool clicked = ImGui::Selectable("##mat", isSelected, ImGuiSelectableFlags_AllowDoubleClick,
                                         ImVec2(m_thumbnailSize, cellH));

        if (clicked) {
            m_selectedMaterialId = mat.id;
            if (ImGui::IsMouseDoubleClicked(0)) {
                if (m_onMaterialAssigned) {
                    m_onMaterialAssigned(mat.id);
                }
            } else {
                if (m_onMaterialSelected) {
                    m_onMaterialSelected(mat.id);
                }
            }
        }

        // Context menu
        m_currentContextMenuMaterial = mat;
        if (ImGui::BeginPopupContextItem("MaterialsPanel_MaterialContext")) {
            if (m_contextMenuManager) {
                m_contextMenuManager->render("MaterialsPanel_MaterialContext");
            }
            ImGui::EndPopup();
        }
        m_currentContextMenuMaterial = std::nullopt;

        // Draw thumbnail or placeholder over the selectable
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 thumbMin = ImVec2(itemMin.x + 4.0f, itemMin.y + 4.0f);
        ImVec2 thumbMax =
            ImVec2(thumbMin.x + m_thumbnailSize - 8.0f, thumbMin.y + m_thumbnailSize - 8.0f);
        float thumbW = thumbMax.x - thumbMin.x;
        float thumbH = thumbMax.y - thumbMin.y;

        GLuint tex = getThumbnailTexture(mat);
        if (tex != 0) {
            drawList->AddImageRounded((ImTextureID)(intptr_t)tex, thumbMin, thumbMax, ImVec2(0, 0),
                                      ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 4.0f);
        } else {
            // Colored placeholder with category initial
            ImU32 bgColor = categoryPlaceholderColor(mat.category);
            drawList->AddRectFilled(thumbMin, thumbMax, bgColor, 4.0f);
            drawList->AddRect(thumbMin, thumbMax, IM_COL32(80, 80, 80, 200), 4.0f);

            const char* initial = categoryInitial(mat.category);
            ImVec2 textSize = ImGui::CalcTextSize(initial);
            ImVec2 textPos = ImVec2(thumbMin.x + (thumbW - textSize.x) * 0.5f,
                                    thumbMin.y + (thumbH - textSize.y) * 0.5f);
            drawList->AddText(textPos, IM_COL32(220, 220, 220, 255), initial);
        }

        // Selection highlight border
        if (isSelected) {
            drawList->AddRect(thumbMin, thumbMax, ImGui::GetColorU32(ImGuiCol_ButtonActive), 4.0f,
                              0, 2.0f);
        }

        // Material name below thumbnail (truncated)
        ImVec2 textPos = ImVec2(itemMin.x + 4.0f, itemMin.y + m_thumbnailSize + 2.0f);
        float maxNameW = m_thumbnailSize - 8.0f;
        const char* nameStart = mat.name.c_str();
        ImVec4 clipRect(textPos.x, textPos.y, textPos.x + maxNameW,
                        textPos.y + ImGui::GetTextLineHeight());
        drawList->AddText(nullptr, 0.0f, textPos, ImGui::GetColorU32(ImGuiCol_Text), nameStart,
                          nullptr, maxNameW, &clipRect);

        ImGui::EndGroup();

        // Grid layout: SameLine unless we've filled a row
        ++col;
        if (col < columns) {
            ImGui::SameLine(0.0f, 4.0f);
        } else {
            col = 0;
        }

        ImGui::PopID();
    }

    // End the child window with a newline if needed
    if (col != 0) {
        ImGui::NewLine();
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------
// Edit form (modal)
// ---------------------------------------------------------------------------

void MaterialsPanel::renderEditForm() {
    if (m_showEditForm) {
        ImGui::OpenPopup("Edit Material");
        m_showEditForm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 0), ImVec2(500, FLT_MAX));

    if (ImGui::BeginPopupModal("Edit Material", nullptr, 0)) {
        ImGui::Text(m_isNewMaterial ? "New Material" : "Edit Material");
        ImGui::Separator();
        ImGui::Spacing();

        // Name
        char nameBuf[256];
        std::strncpy(nameBuf, m_editBuffer.name.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##MatName", nameBuf, sizeof(nameBuf))) {
            m_editBuffer.name = nameBuf;
        }
        ImGui::SameLine(0, 0);
        ImGui::TextDisabled(" Name");

        ImGui::Spacing();

        // Category combo
        const char* categories[] = {"Hardwood", "Softwood", "Domestic", "Composite"};
        int catIndex = static_cast<int>(m_editBuffer.category);
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::Combo("Category", &catIndex, categories, 4)) {
            m_editBuffer.category = static_cast<MaterialCategory>(catIndex);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("CNC Parameters");
        ImGui::Spacing();

        // Janka Hardness
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Janka Hardness (lbf)", &m_editBuffer.jankaHardness, 10.0f, 100.0f,
                          "%.0f");

        // Feed Rate
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Feed Rate (in/min)", &m_editBuffer.feedRate, 1.0f, 10.0f, "%.1f");

        // Spindle Speed
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Spindle Speed (RPM)", &m_editBuffer.spindleSpeed, 100.0f, 1000.0f,
                          "%.0f");

        // Depth of Cut
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Depth of Cut (in)", &m_editBuffer.depthOfCut, 0.01f, 0.1f, "%.3f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Economics");
        ImGui::Spacing();

        // Cost per Board Foot
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Cost per Board Foot ($)", &m_editBuffer.costPerBoardFoot, 0.1f, 1.0f,
                          "%.2f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Grain");
        ImGui::Spacing();

        // Grain Direction
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("Grain Direction (deg)", &m_editBuffer.grainDirectionDeg, 0.0f, 360.0f,
                           "%.1f deg");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            if (m_materialManager && !m_editBuffer.name.empty()) {
                if (m_isNewMaterial) {
                    auto newId = m_materialManager->addMaterial(m_editBuffer);
                    if (newId) {
                        refresh();
                    } else {
                        log::error("MaterialsPanel", "Failed to add new material");
                    }
                } else {
                    if (m_materialManager->updateMaterial(m_editBuffer)) {
                        refresh();
                    } else {
                        log::error("MaterialsPanel", "Failed to update material");
                    }
                }
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

// ---------------------------------------------------------------------------
// Delete confirmation
// ---------------------------------------------------------------------------

void MaterialsPanel::renderDeleteConfirm() {
    if (m_showDeleteConfirm) {
        ImGui::OpenPopup("Delete Material?");
        m_showDeleteConfirm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Delete Material?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete \"%s\"?", m_deleteName.c_str());
        ImGui::TextDisabled("This action cannot be undone.");
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (m_materialManager && m_deleteId != -1) {
                if (m_materialManager->removeMaterial(m_deleteId)) {
                    if (m_selectedMaterialId == m_deleteId) {
                        m_selectedMaterialId = -1;
                    }
                    refresh();
                } else {
                    log::error("MaterialsPanel", "Failed to delete material");
                }
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

// ---------------------------------------------------------------------------
// Texture cache helpers
// ---------------------------------------------------------------------------

void MaterialsPanel::clearTextureCache() {
    for (auto& [id, tex] : m_thumbnailCache) {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
        }
    }
    m_thumbnailCache.clear();
}

GLuint MaterialsPanel::loadTGATexture(const Path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return 0;
    }

    uint8_t header[18];
    file.read(reinterpret_cast<char*>(header), 18);
    if (!file) {
        return 0;
    }

    // Only support uncompressed true-colour 32bpp TGA
    if (header[2] != 2 || header[16] != 32) {
        return 0;
    }

    int width = header[12] | (header[13] << 8);
    int height = header[14] | (header[15] << 8);
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return 0;
    }

    size_t dataSize = static_cast<size_t>(width) * height * 4;
    std::vector<uint8_t> bgra(dataSize);
    file.read(reinterpret_cast<char*>(bgra.data()), static_cast<std::streamsize>(dataSize));
    if (!file) {
        return 0;
    }

    // Convert BGRA -> RGBA in-place
    for (size_t i = 0; i < dataSize; i += 4) {
        std::swap(bgra[i + 0], bgra[i + 2]);
    }

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 bgra.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

GLuint MaterialsPanel::getThumbnailTexture(const MaterialRecord& material) {
    auto it = m_thumbnailCache.find(material.id);
    if (it != m_thumbnailCache.end()) {
        return it->second;
    }

    GLuint tex = 0;

    // Try loading thumbnail from explicit path (TGA)
    if (tex == 0 && !material.thumbnailPath.empty()) {
        tex = loadTGATexture(material.thumbnailPath);
    }

    // Try extracting texture from .dwmat archive
    if (tex == 0 && !material.archivePath.empty()) {
        auto data = MaterialArchive::load(material.archivePath.string());
        if (data && !data->textureData.empty()) {
            tex = loadPNGTexture(data->textureData.data(), data->textureData.size());
        }
    }

    m_thumbnailCache[material.id] = tex;
    return tex;
}

GLuint MaterialsPanel::loadPNGTexture(const uint8_t* data, size_t size) {
    auto texData = TextureLoader::loadPNGFromMemory(data, size);
    if (!texData || texData->pixels.empty()) {
        return 0;
    }

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texData->width, texData->height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, texData->pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

// ---------------------------------------------------------------------------
// Category helpers
// ---------------------------------------------------------------------------

const char* MaterialsPanel::categoryInitial(MaterialCategory cat) {
    switch (cat) {
    case MaterialCategory::Hardwood:
        return "H";
    case MaterialCategory::Softwood:
        return "S";
    case MaterialCategory::Domestic:
        return "D";
    case MaterialCategory::Composite:
        return "C";
    default:
        return "?";
    }
}

ImU32 MaterialsPanel::categoryPlaceholderColor(MaterialCategory cat) {
    switch (cat) {
    case MaterialCategory::Hardwood:
        return IM_COL32(120, 80, 40, 220); // warm brown
    case MaterialCategory::Softwood:
        return IM_COL32(60, 100, 60, 220); // forest green
    case MaterialCategory::Domestic:
        return IM_COL32(90, 70, 110, 220); // muted purple
    case MaterialCategory::Composite:
        return IM_COL32(60, 80, 100, 220); // slate blue
    default:
        return IM_COL32(80, 80, 80, 220);
    }
}

} // namespace dw
