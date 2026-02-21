#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>
#include <imgui.h>

#include "../../core/materials/material_manager.h"
#include "panel.h"

namespace dw {

// Materials panel for browsing, selecting, editing, importing, and exporting wood species materials
class MaterialsPanel : public Panel {
  public:
    explicit MaterialsPanel(MaterialManager* materialManager);
    ~MaterialsPanel() override;

    void render() override;

    // Callback when a material is single-clicked (select/preview)
    using MaterialSelectedCallback = std::function<void(i64 materialId)>;
    void setOnMaterialSelected(MaterialSelectedCallback callback) {
        m_onMaterialSelected = std::move(callback);
    }

    // Callback when a material is double-clicked (assign to current object)
    using MaterialAssignedCallback = std::function<void(i64 materialId)>;
    void setOnMaterialAssigned(MaterialAssignedCallback callback) {
        m_onMaterialAssigned = std::move(callback);
    }

    // Callback when user requests material generation from prompt
    using GenerateCallback = std::function<void(const std::string& prompt)>;
    void setOnGenerate(GenerateCallback callback) { m_onGenerate = std::move(callback); }

    // Set generating state (called from main thread after async generation completes/fails)
    void setGenerating(bool generating) { m_isGenerating = generating; }

    // Reload materials from database
    void refresh();

    // Get currently selected material ID (-1 if none)
    i64 selectedMaterialId() const { return m_selectedMaterialId; }

    // Set whether a model is loaded (enables/disables Assign button)
    void setModelLoaded(bool loaded) { m_modelLoaded = loaded; }

  private:
    enum class CategoryTab { All, Hardwood, Softwood, Domestic, Composite };

    void renderToolbar();
    void renderCategoryTabs();
    void renderMaterialGrid(const std::vector<MaterialRecord>& materials);
    void renderEditForm();
    void renderDeleteConfirm();

    // Get a cached GL texture for a material thumbnail (loads lazily)
    GLuint getThumbnailTexture(const MaterialRecord& material);

    // Load a TGA file into an OpenGL texture, returns 0 on failure
    GLuint loadTGATexture(const Path& path);

    // Release all cached GL textures
    void clearTextureCache();

    // Get the category initial letter for placeholder display (H/S/D/C)
    static const char* categoryInitial(MaterialCategory cat);

    // Get category placeholder background color
    static ImU32 categoryPlaceholderColor(MaterialCategory cat);

    MaterialManager* m_materialManager;
    std::vector<MaterialRecord> m_allMaterials;

    std::string m_searchQuery;
    i64 m_selectedMaterialId = -1;
    CategoryTab m_activeCategory = CategoryTab::All;

    // Edit form state
    bool m_showEditForm = false;
    bool m_isNewMaterial = false;
    MaterialRecord m_editBuffer;

    // Delete confirmation state
    bool m_showDeleteConfirm = false;
    i64 m_deleteId = -1;
    std::string m_deleteName;

    // Thumbnail texture cache: material ID -> GL texture (0 = no texture)
    std::unordered_map<i64, GLuint> m_thumbnailCache;

    MaterialSelectedCallback m_onMaterialSelected;
    MaterialAssignedCallback m_onMaterialAssigned;
    GenerateCallback m_onGenerate;

    // Generate state
    char m_generatePrompt[256]{};
    bool m_isGenerating = false;

    bool m_modelLoaded = false;
    float m_thumbnailSize = 96.0f;
};

} // namespace dw
