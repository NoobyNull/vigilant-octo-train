#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_tool.h"
#include "panel.h"

namespace dw {

class ToolDatabase;
class FileDialog;

class ToolBrowserPanel : public Panel {
  public:
    ToolBrowserPanel();
    ~ToolBrowserPanel() override = default;

    void render() override;

    void setToolDatabase(ToolDatabase* db) { m_toolDatabase = db; }
    void setFileDialog(FileDialog* dlg) { m_fileDialog = dlg; }
    void refresh();

  private:
    void renderToolbar();
    void renderTree();
    void renderToolDetail();
    void renderAddToolPopup();
    void renderAddGroupPopup();

    void loadData();
    void selectTool(const std::string& geometryId);
    void deleteSelected();

    ToolDatabase* m_toolDatabase = nullptr;
    FileDialog* m_fileDialog = nullptr;

    // Cached data
    std::vector<VtdbTreeEntry> m_treeEntries;
    std::vector<VtdbMaterial> m_materials;
    std::vector<VtdbMachine> m_machines;
    std::vector<VtdbToolGeometry> m_geometries;

    // Selection state
    std::string m_selectedTreeEntryId;
    std::string m_selectedGeometryId;
    std::string m_selectedMaterialId;
    std::string m_selectedMachineId;

    // Editing state
    VtdbToolGeometry m_editGeometry;
    VtdbCuttingData m_editCuttingData;
    bool m_hasCuttingData = false;

    // Add Tool popup state
    bool m_showAddTool = false;
    int m_addToolType = 1; // EndMill default
    std::string m_addToolName;
    float m_addToolDiameter = 0.25f;
    int m_addToolFlutes = 2;
    std::string m_addToolParentGroupId;

    // Add Group popup state
    bool m_showAddGroup = false;
    std::string m_addGroupName;
    std::string m_addGroupParentId;

    bool m_needsRefresh = true;
};

} // namespace dw
