#pragma once

#include "../../core/library/library_manager.h"
#include "panel.h"

#include <functional>
#include <string>
#include <vector>

namespace dw {

// Library panel for browsing and managing imported models
class LibraryPanel : public Panel {
public:
    explicit LibraryPanel(LibraryManager* library);
    ~LibraryPanel() override = default;

    void render() override;

    // Callback when a model is selected
    using ModelSelectedCallback = std::function<void(int64_t modelId)>;
    void setOnModelSelected(ModelSelectedCallback callback) {
        m_onModelSelected = std::move(callback);
    }

    // Refresh the model list
    void refresh();

    // Get currently selected model ID (-1 if none)
    int64_t selectedModelId() const { return m_selectedModelId; }

private:
    void renderToolbar();
    void renderModelList();
    void renderModelItem(const ModelRecord& model, int index);
    void renderContextMenu(const ModelRecord& model);

    LibraryManager* m_library;
    std::vector<ModelRecord> m_models;
    std::string m_searchQuery;
    int64_t m_selectedModelId = -1;
    int m_contextMenuModelIndex = -1;

    ModelSelectedCallback m_onModelSelected;

    // View options
    bool m_showThumbnails = true;
    float m_thumbnailSize = 64.0f;
};

}  // namespace dw
