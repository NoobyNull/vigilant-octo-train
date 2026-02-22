#pragma once

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>

#include "../../core/library/library_manager.h"
#include "panel.h"

namespace dw {

class ContextMenuManager;

// Library panel for browsing and managing imported models
class LibraryPanel : public Panel {
  public:
    explicit LibraryPanel(LibraryManager* library);
    ~LibraryPanel() override;

    void render() override;

    // Callback when a model is clicked (single-click: preview metadata)
    using ModelSelectedCallback = std::function<void(int64_t modelId)>;
    void setOnModelSelected(ModelSelectedCallback callback) {
        m_onModelSelected = std::move(callback);
    }

    // Callback when a model is double-clicked (load into viewport)
    void setOnModelOpened(ModelSelectedCallback callback) { m_onModelOpened = std::move(callback); }

    // Callback when a G-code file is selected
    using GCodeSelectedCallback = std::function<void(int64_t gcodeId)>;
    void setOnGCodeSelected(GCodeSelectedCallback callback) {
        m_onGCodeSelected = std::move(callback);
    }

    // Callback when a G-code file is double-clicked (load into viewport)
    void setOnGCodeOpened(GCodeSelectedCallback callback) { m_onGCodeOpened = std::move(callback); }

    // Callback when thumbnail regeneration is requested (batch: all selected IDs)
    using RegenerateThumbnailCallback = std::function<void(const std::vector<int64_t>& modelIds)>;
    void setOnRegenerateThumbnail(RegenerateThumbnailCallback callback) {
        m_onRegenerateThumbnail = std::move(callback);
    }

    // Callback when "Assign Default Material" is chosen
    void setOnAssignDefaultMaterial(ModelSelectedCallback callback) {
        m_onAssignDefaultMaterial = std::move(callback);
    }

    // Refresh the model and G-code lists
    void refresh();

    // Invalidate cached thumbnail texture for a model (forces reload from disk)
    void invalidateThumbnail(int64_t modelId);

    // Get/set currently selected model ID (-1 if none)
    int64_t selectedModelId() const { return m_lastClickedModelId; }
    void setSelectedModelId(int64_t id) {
        m_selectedModelIds = {id};
        m_lastClickedModelId = id;
    }

    // Multi-selection accessors
    const std::set<int64_t>& selectedModelIds() const { return m_selectedModelIds; }
    const std::set<int64_t>& selectedGCodeIds() const { return m_selectedGCodeIds; }
    bool isModelSelected(int64_t id) const { return m_selectedModelIds.count(id) > 0; }

    // Set context menu manager (must be called before first render)
    void setContextMenuManager(ContextMenuManager* mgr);

  private:
    enum class ViewTab { All, Models, GCode };

    void renderToolbar();
    void renderTabs();
    void renderCategoryFilter();
    void renderCategoryBreadcrumb();
    void renderCategoryAssignDialog();
    void renderModelList();
    void renderGCodeList();
    void renderCombinedList();
    void renderModelItem(const ModelRecord& model, int index, float thumbOverride = 0.0f);
    void renderGCodeItem(const GCodeRecord& gcode, int index, float thumbOverride = 0.0f);
    void renderRenameDialog();
    void renderDeleteConfirm();
    void registerContextMenuEntries();

    // Load a TGA file into an OpenGL texture, returns 0 on failure
    GLuint loadTGATexture(const Path& path);

    // Get or load a cached thumbnail texture for a model
    GLuint getThumbnailTexture(const ModelRecord& model);

    // Release all cached GL textures
    void clearTextureCache();

    LibraryManager* m_library;
    std::vector<ModelRecord> m_models;
    std::vector<GCodeRecord> m_gcodeFiles;
    std::string m_searchQuery;
    std::set<int64_t> m_selectedModelIds;
    std::set<int64_t> m_selectedGCodeIds;
    int64_t m_lastClickedModelId = -1;
    int64_t m_lastClickedGCodeId = -1;

    ViewTab m_activeTab = ViewTab::All;

    ModelSelectedCallback m_onModelSelected;
    ModelSelectedCallback m_onModelOpened;
    RegenerateThumbnailCallback m_onRegenerateThumbnail;
    ModelSelectedCallback m_onAssignDefaultMaterial;
    GCodeSelectedCallback m_onGCodeSelected;
    GCodeSelectedCallback m_onGCodeOpened;

    // Thumbnail texture cache: model ID -> GL texture
    std::unordered_map<int64_t, GLuint> m_textureCache;

    // Rename dialog state
    bool m_showRenameDialog = false;
    int64_t m_renameModelId = 0;
    char m_renameBuffer[256] = {};

    // Delete confirmation state
    bool m_showDeleteConfirm = false;
    std::vector<int64_t> m_deleteItemIds;
    bool m_deleteIsGCode = false;
    std::string m_deleteItemName; // Single item name, or "N items" for batch

    // View options
    bool m_showThumbnails = true;
    float m_thumbnailSize = 96.0f;
    static constexpr float THUMB_MIN = 48.0f;
    // THUMB_MAX is computed per-frame as the available content width

    // Category filter state
    int64_t m_selectedCategoryId = -1;        // -1 = show all
    std::string m_selectedCategoryName;       // For breadcrumb display
    std::vector<CategoryRecord> m_categories; // Cached category list
    bool m_showCategoryAssignDialog = false;
    float m_searchDebounceTimer = 0.0f;
    bool m_searchDirty = false;
    bool m_useFTS = true; // Use FTS5 for search (true) vs LIKE (false)

    // Category assign dialog state
    std::set<int64_t> m_assignedCategoryIds; // Categories checked for current model(s)
    char m_newCategoryName[128] = {};
    int64_t m_newCategoryParent = -1; // -1 = root

    // Context menu management
    ContextMenuManager* m_contextMenuManager = nullptr;
    std::optional<ModelRecord> m_currentContextMenuModel;
    std::optional<GCodeRecord> m_currentContextMenuGCode;
};

} // namespace dw
