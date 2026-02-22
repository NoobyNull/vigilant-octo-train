#pragma once

#include <memory>
#include <vector>

#include "../../core/database/cut_plan_repository.h"
#include "../../core/optimizer/cut_optimizer.h"
#include "../../core/optimizer/sheet.h"
#include "../widgets/canvas_2d.h"
#include "panel.h"

namespace dw {

class ProjectManager;

// 2D Cut optimizer panel
class CutOptimizerPanel : public Panel {
  public:
    CutOptimizerPanel();
    ~CutOptimizerPanel() override = default;

    void render() override;

    // Clear all data
    void clear();

    // Load a saved cut plan record into the panel state
    void loadCutPlan(const CutPlanRecord& record);

    // Dependency injection
    void setCutPlanRepository(CutPlanRepository* repo) { m_cutPlanRepo = repo; }
    void setProjectManager(ProjectManager* pm) { m_projectManager = pm; }

  private:
    void saveCutPlan(const char* name);
    void renderToolbar();
    void renderPartsEditor();
    void renderSheetConfig();
    void renderResults();
    void renderVisualization();

    void runOptimization();

    // Input data
    std::vector<optimizer::Part> m_parts;
    optimizer::Sheet m_sheet{2440.0f, 1220.0f}; // Standard 4x8 sheet

    // Settings
    optimizer::Algorithm m_algorithm = optimizer::Algorithm::Guillotine;
    bool m_allowRotation = true;
    float m_kerf = 3.0f;   // mm
    float m_margin = 5.0f; // mm

    // Results
    optimizer::CutPlan m_result;
    bool m_hasResults = false;

    // Editor state
    float m_newPartWidth = 100.0f;
    float m_newPartHeight = 100.0f;
    int m_newPartQuantity = 1;
    char m_newPartName[64] = "";

    // Visualization state
    int m_selectedSheet = 0;
    Canvas2D m_canvas; // zoomMax set in constructor

    // Persistence
    CutPlanRepository* m_cutPlanRepo = nullptr;
    ProjectManager* m_projectManager = nullptr;
    i64 m_loadedPlanId = -1;
};

} // namespace dw
