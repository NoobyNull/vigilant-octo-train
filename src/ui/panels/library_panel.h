#pragma once

#include "../../core/library/library_manager.h"
#include "panel.h"

#include <glad/gl.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dw {

// Library panel for browsing and managing imported models
class LibraryPanel : public Panel {
public:
    explicit LibraryPanel(LibraryManager* library);
    ~LibraryPanel() override;

    void render() override;

    // Callback when a model is selected
    using ModelSelectedCallback = std::function<void(int64_t modelId)>;
    void setOnModelSelected(ModelSelectedCallback callback) {
        m_onModelSelected = std::move(callback);
    }

    // Refresh the model list
    void refresh();

    // Get/set currently selected model ID (-1 if none)
    int64_t selectedModelId() const { return m_selectedModelId; }
    void setSelectedModelId(int64_t id) { m_selectedModelId = id; }

private:
    void renderToolbar();
    void renderModelList();
    void renderModelItem(const ModelRecord& model, int index);
    void renderContextMenu(const ModelRecord& model);
    void renderRenameDialog();

    // Load a TGA file into an OpenGL texture, returns 0 on failure
    GLuint loadTGATexture(const Path& path);

    // Get or load a cached thumbnail texture for a model
    GLuint getThumbnailTexture(const ModelRecord& model);

    // Release all cached GL textures
    void clearTextureCache();

    LibraryManager* m_library;
    std::vector<ModelRecord> m_models;
    std::string m_searchQuery;
    int64_t m_selectedModelId = -1;
    int m_contextMenuModelIndex = -1;

    ModelSelectedCallback m_onModelSelected;

    // Thumbnail texture cache: model ID -> GL texture
    std::unordered_map<int64_t, GLuint> m_textureCache;

    // Rename dialog state
    bool m_showRenameDialog = false;
    int64_t m_renameModelId = 0;
    char m_renameBuffer[256] = {};

    // View options
    bool m_showThumbnails = true;
    float m_thumbnailSize = 64.0f;
};

}  // namespace dw
