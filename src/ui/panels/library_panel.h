#pragma once

#include <functional>
#include <optional>
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

    // Callback when thumbnail regeneration is requested
    using RegenerateThumbnailCallback = std::function<void(int64_t modelId)>;
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
    int64_t selectedModelId() const { return m_selectedModelId; }
    void setSelectedModelId(int64_t id) { m_selectedModelId = id; }

    // Set context menu manager (must be called before first render)
    void setContextMenuManager(ContextMenuManager* mgr);

  private:
    enum class ViewTab { All, Models, GCode };

    void renderToolbar();
    void renderTabs();
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
    int64_t m_selectedModelId = -1;
    int64_t m_selectedGCodeId = -1;

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
    int64_t m_deleteItemId = 0;
    bool m_deleteIsGCode = false;
    std::string m_deleteItemName;

    // View options
    bool m_showThumbnails = true;
    float m_thumbnailSize = 96.0f;
    static constexpr float THUMB_MIN = 48.0f;
    // THUMB_MAX is computed per-frame as the available content width

    // Context menu management
    ContextMenuManager* m_contextMenuManager = nullptr;
    bool m_contextMenuRegistered = false;
    std::optional<ModelRecord> m_currentContextMenuModel;
    std::optional<GCodeRecord> m_currentContextMenuGCode;
};

} // namespace dw
