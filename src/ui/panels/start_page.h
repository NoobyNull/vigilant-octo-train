#pragma once

#include <functional>

#include "../../core/types.h"
#include "panel.h"

namespace dw {

// Start page shown on application launch with recent projects and quick actions
class StartPage : public Panel {
  public:
    StartPage();
    ~StartPage() override = default;

    void render() override;

    // Callbacks
    using VoidCallback = std::function<void()>;
    using PathCallback = std::function<void(const Path&)>;

    void setOnNewProject(VoidCallback cb) { m_onNewProject = std::move(cb); }
    void setOnOpenProject(VoidCallback cb) { m_onOpenProject = std::move(cb); }
    void setOnImportModel(VoidCallback cb) { m_onImportModel = std::move(cb); }
    void setOnOpenRecentProject(PathCallback cb) { m_onOpenRecentProject = std::move(cb); }

  private:
    void renderRecentProjects();
    void renderQuickActions();

    VoidCallback m_onNewProject;
    VoidCallback m_onOpenProject;
    VoidCallback m_onImportModel;
    PathCallback m_onOpenRecentProject;
};

} // namespace dw
