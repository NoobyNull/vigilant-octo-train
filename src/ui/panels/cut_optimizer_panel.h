#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../core/optimizer/cut_list_file.h"
#include "../../core/optimizer/cut_optimizer.h"
#include "../../core/optimizer/sheet.h"
#include "../widgets/canvas_2d.h"
#include "panel.h"

namespace dw {

class ModelRepository;
class ProjectManager;
class CostRepository;

// Stock sheet preset (material + dimensions + cost)
struct StockPreset {
    const char* name;
    float width;
    float height;
    float cost;
};

// 2D Cut optimizer panel — CutListMachine-style 3-column layout
class CutOptimizerPanel : public Panel {
  public:
    CutOptimizerPanel();
    ~CutOptimizerPanel() override = default;

    void render() override;

    // Clear all data
    void clear();

    // Load from a CutListFile::LoadResult
    void loadCutPlan(const CutListFile::LoadResult& lr);

    // Dependency injection
    void setCutListFile(CutListFile* clf) { m_cutListFile = clf; }
    void setProjectManager(ProjectManager* pm) { m_projectManager = pm; }
    void setModelRepository(ModelRepository* repo) { m_modelRepo = repo; }

    // Cost integration callback
    using AddToCostCallback = std::function<void(const std::string& name,
                                                  int sheetsUsed,
                                                  float costPerSheet,
                                                  float totalCost)>;
    void setOnAddToCost(AddToCostCallback cb) { m_onAddToCost = std::move(cb); }

  private:
    void saveCutPlan(const char* name);
    std::string nextPartLabel() const;

    // Toolbar
    void renderToolbar();

    // Left column
    void renderCutListTable();
    void renderStockSheets();
    void renderSettings();

    // Center column
    void renderVisualization();

    // Right column
    void renderLayoutsSidebar();
    void renderResultsPanel();

    // Bottom
    void renderStatusBar();

    // Popups
    void renderImportPopup();

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
    int m_editPartIdx = -1; // Which part row is being inline-edited (-1 = none)
    char m_newPartName[64] = "";
    float m_newPartWidth = 100.0f;
    float m_newPartHeight = 100.0f;
    int m_newPartQuantity = 1;

    // Visualization state
    int m_selectedSheet = 0;
    Canvas2D m_canvas;

    // Stock sheet name buffer
    char m_sheetName[64] = "Plywood";

    // Persistence
    CutListFile* m_cutListFile = nullptr;
    ProjectManager* m_projectManager = nullptr;
    Path m_loadedPlanPath;

    // Model import
    ModelRepository* m_modelRepo = nullptr;
    bool m_importPopupOpen = false;
    std::vector<char> m_importSelection; // per-model checkbox state (char to avoid vector<bool> proxy)

    // Cost integration
    AddToCostCallback m_onAddToCost;

    // Stock presets
    static constexpr StockPreset kPresets[] = {
        {"Plywood", 2440.0f, 1220.0f, 45.0f},
        {"MDF", 2440.0f, 1220.0f, 35.0f},
        {"Baltic Birch", 1524.0f, 1524.0f, 85.0f},
    };
};

} // namespace dw
