#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_tool.h"
#include "../../core/cnc/tool_calculator.h"
#include "../../core/materials/material.h"
#include "panel.h"

namespace dw {

class ToolDatabase;
class MaterialManager;

// CNC workspace panel for tool/material selection and feeds/speeds reference.
// Operator selects a tool geometry and wood species, panel auto-calculates
// recommended cutting parameters using ToolCalculator.
// Results are advisory — operator reads them while setting up or running a job.
class CncToolPanel : public Panel {
  public:
    CncToolPanel();
    ~CncToolPanel() override = default;

    void render() override;

    void setToolDatabase(ToolDatabase* db) { m_toolDatabase = db; }
    void setMaterialManager(MaterialManager* mgr) { m_materialManager = mgr; }

    void refresh() { m_needsRefresh = true; }

    // Getters for downstream consumers (CncJobPanel feed deviation comparison)
    float getRecommendedFeedRate() const {
        return m_hasCalcResult ? static_cast<float>(m_calcResult.feed_rate) : 0.0f;
    }
    bool hasCalcResult() const { return m_hasCalcResult; }
    const CalcResult& getCalcResult() const { return m_calcResult; }

  private:
    void loadData();
    void recalculate();
    void renderToolSelector();
    void renderMaterialSelector();
    void renderMachineInfo();
    void renderResults();

    ToolDatabase* m_toolDatabase = nullptr;
    MaterialManager* m_materialManager = nullptr;

    // Cached data
    std::vector<VtdbToolGeometry> m_geometries;
    std::vector<VtdbTreeEntry> m_treeEntries;
    std::vector<MaterialRecord> m_materials;
    std::vector<VtdbMachine> m_machines;

    // Selection state
    std::string m_selectedGeometryId;
    i64 m_selectedMaterialId = 0;
    std::string m_selectedMachineId;

    // Display names for combos
    std::string m_selectedToolName;
    std::string m_selectedMaterialName;

    // Material data for calculator
    f32 m_selectedJanka = 0.0f;
    std::string m_selectedMaterialNameForCalc;

    // Calculator state
    CalcResult m_calcResult;
    bool m_hasCalcResult = false;

    // Change tracking for auto-recalculation
    std::string m_prevGeometryId;
    i64 m_prevMaterialId = 0;
    std::string m_prevMachineId;

    bool m_needsRefresh = true;
};

} // namespace dw
