#pragma once

#include "../../core/project/project.h"
#include "panel.h"

#include <functional>
#include <memory>

namespace dw {

// Project panel for managing current project and its models
class ProjectPanel : public Panel {
public:
    explicit ProjectPanel(ProjectManager* projectManager);
    ~ProjectPanel() override = default;

    void render() override;

    // Callback when a project model is selected
    using ModelSelectedCallback = std::function<void(int64_t modelId)>;
    void setOnModelSelected(ModelSelectedCallback callback) {
        m_onModelSelected = std::move(callback);
    }

    // Refresh project data
    void refresh();

private:
    void renderProjectInfo();
    void renderModelList();
    void renderNoProject();

    ProjectManager* m_projectManager;
    int64_t m_selectedModelId = -1;
    ModelSelectedCallback m_onModelSelected;
};

}  // namespace dw
