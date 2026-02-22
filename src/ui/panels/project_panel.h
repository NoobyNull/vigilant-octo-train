#pragma once

#include <functional>
#include <memory>

#include "../../core/project/project.h"
#include "../../core/types.h"
#include "panel.h"

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

    // Callback when Open Project button is clicked in the panel
    using OpenProjectCallback = std::function<void()>;
    void setOpenProjectCallback(OpenProjectCallback callback) {
        m_openProjectCallback = std::move(callback);
    }

    // Callback when Save Project button is clicked in the panel
    using SaveProjectCallback = std::function<void()>;
    void setSaveProjectCallback(SaveProjectCallback callback) {
        m_saveProjectCallback = std::move(callback);
    }

    // Callback when Export .dwproj button is clicked
    using ExportProjectCallback = std::function<void()>;
    void setExportProjectCallback(ExportProjectCallback cb) {
        m_exportProjectCallback = std::move(cb);
    }

    // Callback when a recent project is clicked
    using RecentProjectCallback = std::function<void(const Path&)>;
    void setOnOpenRecentProject(RecentProjectCallback cb) { m_onOpenRecentProject = std::move(cb); }

  private:
    void renderProjectInfo();
    void renderModelList();
    void renderNoProject();

    ProjectManager* m_projectManager;
    int64_t m_selectedModelId = -1;
    ModelSelectedCallback m_onModelSelected;
    OpenProjectCallback m_openProjectCallback;
    SaveProjectCallback m_saveProjectCallback;
    ExportProjectCallback m_exportProjectCallback;
    RecentProjectCallback m_onOpenRecentProject;
};

} // namespace dw
