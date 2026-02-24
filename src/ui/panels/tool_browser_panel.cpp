#include "ui/panels/tool_browser_panel.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <imgui.h>

#include "core/database/tool_database.h"
#include "core/utils/log.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/icons.h"

namespace dw {

// Tool type display names
static const char* toolTypeName(VtdbToolType type) {
    switch (type) {
    case VtdbToolType::BallNose: return "Ball Nose";
    case VtdbToolType::EndMill: return "End Mill";
    case VtdbToolType::Radiused: return "Radiused";
    case VtdbToolType::VBit: return "V-Bit";
    case VtdbToolType::TaperedBallNose: return "Tapered Ball Nose";
    case VtdbToolType::Drill: return "Drill";
    case VtdbToolType::ThreadMill: return "Thread Mill";
    case VtdbToolType::FormTool: return "Form Tool";
    case VtdbToolType::DiamondDrag: return "Diamond Drag";
    default: return "Unknown";
    }
}

static constexpr int kToolTypeCount = 10;
static const VtdbToolType kToolTypes[] = {
    VtdbToolType::BallNose,
    VtdbToolType::EndMill,
    VtdbToolType::Radiused,
    VtdbToolType::VBit,
    VtdbToolType::TaperedBallNose,
    VtdbToolType::Drill,
    VtdbToolType::ThreadMill,
    VtdbToolType::FormTool,
    VtdbToolType::DiamondDrag,
};

ToolBrowserPanel::ToolBrowserPanel() : Panel("Tool Browser") {}

void ToolBrowserPanel::render() {
    if (!m_open) return;

    if (m_needsRefresh && m_toolDatabase) {
        loadData();
        m_needsRefresh = false;
    }

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_toolDatabase) {
        ImGui::TextDisabled("No tool database loaded");
        ImGui::End();
        return;
    }

    renderToolbar();
    ImGui::Separator();

    // Two-column layout: tree (left) | detail (right)
    float treeWidth = 260.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("ToolTree", ImVec2(treeWidth, avail.y), ImGuiChildFlags_Borders)) {
        renderTree();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("ToolDetail", ImVec2(0, avail.y), ImGuiChildFlags_Borders)) {
        renderToolDetail();
    }
    ImGui::EndChild();

    renderAddToolPopup();
    renderAddGroupPopup();

    ImGui::End();
}

void ToolBrowserPanel::renderToolbar() {
    auto iconBtn = [](const char* icon, const char* label) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s %s", icon, label);
        return ImGui::Button(buf);
    };

    if (iconBtn(Icons::Add, "Tool")) {
        m_showAddTool = true;
        m_addToolName.clear();
        m_addToolType = static_cast<int>(VtdbToolType::EndMill);
        m_addToolDiameter = 0.25f;
        m_addToolFlutes = 2;
        m_addToolParentGroupId.clear();
    }
    ImGui::SameLine();
    if (iconBtn(Icons::Folder, "Group")) {
        m_showAddGroup = true;
        m_addGroupName.clear();
        m_addGroupParentId.clear();
    }
    ImGui::SameLine();
    if (iconBtn(Icons::Delete, "Delete")) {
        deleteSelected();
    }
    ImGui::SameLine();
    if (iconBtn(Icons::Import, "Import .vtdb")) {
        if (m_fileDialog) {
            std::vector<FileFilter> filters = {{".vtdb Tool Database", "*.vtdb"}};
            m_fileDialog->showOpen(
                "Import Tool Database", filters, [this](const std::string& path) {
                    if (!path.empty() && m_toolDatabase) {
                        int count = m_toolDatabase->importFromVtdb(Path(path));
                        if (count >= 0) {
                            log::infof("ToolBrowser", "Imported %d tools from %s", count, path.c_str());
                        } else {
                            log::errorf("ToolBrowser", "Failed to import from %s", path.c_str());
                        }
                        refresh();
                    }
                });
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(icons::ICON_REFRESH)) {
        refresh();
    }
}

// Recursive tree rendering helper
static void renderTreeNode(const std::vector<VtdbTreeEntry>& entries,
                           const std::string& parentId,
                           std::string& selectedId,
                           std::string& selectedGeomId) {
    for (const auto& entry : entries) {
        if (entry.parent_group_id != parentId) continue;

        bool isGroup = entry.tool_geometry_id.empty();
        bool isSelected = (entry.id == selectedId);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;
        if (!isGroup) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        const char* icon = isGroup ? Icons::Folder : Icons::Settings;
        std::string label = std::string(icon) + " " + entry.name + "###" + entry.id;

        if (isGroup) {
            bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                selectedId = entry.id;
                selectedGeomId.clear();
            }
            if (nodeOpen) {
                renderTreeNode(entries, entry.id, selectedId, selectedGeomId);
                ImGui::TreePop();
            }
        } else {
            ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked()) {
                selectedId = entry.id;
                selectedGeomId = entry.tool_geometry_id;
            }
        }
    }
}

void ToolBrowserPanel::renderTree() {
    renderTreeNode(m_treeEntries, "", m_selectedTreeEntryId, m_selectedGeometryId);

    // Root-level entries that have no parent
    // (already handled by passing "" as parentId above)
}

void ToolBrowserPanel::renderToolDetail() {
    if (m_selectedGeometryId.empty()) {
        ImGui::TextDisabled("Select a tool to view details");
        return;
    }

    // Geometry section
    ImGui::SeparatorText("Geometry");

    // Tool type dropdown
    const char* currentTypeName = toolTypeName(m_editGeometry.tool_type);
    if (ImGui::BeginCombo("Tool Type", currentTypeName)) {
        for (int i = 0; i < kToolTypeCount; ++i) {
            bool selected = (m_editGeometry.tool_type == kToolTypes[i]);
            if (ImGui::Selectable(toolTypeName(kToolTypes[i]), selected)) {
                m_editGeometry.tool_type = kToolTypes[i];
            }
        }
        ImGui::EndCombo();
    }

    // Units
    const char* unitsLabels[] = {"Metric (mm)", "Imperial (in)"};
    int unitsIdx = static_cast<int>(m_editGeometry.units);
    if (ImGui::Combo("Units", &unitsIdx, unitsLabels, 2)) {
        m_editGeometry.units = static_cast<VtdbUnits>(unitsIdx);
    }

    const char* unitSuffix = (m_editGeometry.units == VtdbUnits::Metric) ? "mm" : "in";

    // Core dimensions
    float diameter = static_cast<float>(m_editGeometry.diameter);
    if (ImGui::InputFloat("Diameter", &diameter, 0.01f, 0.1f, "%.4f")) {
        m_editGeometry.diameter = diameter;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unitSuffix);

    int flutes = m_editGeometry.num_flutes;
    if (ImGui::InputInt("Flutes", &flutes)) {
        m_editGeometry.num_flutes = std::max(1, flutes);
    }

    float fluteLen = static_cast<float>(m_editGeometry.flute_length);
    if (ImGui::InputFloat("Flute Length", &fluteLen, 0.01f, 0.1f, "%.4f")) {
        m_editGeometry.flute_length = fluteLen;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", unitSuffix);

    // V-bit specific fields
    if (m_editGeometry.tool_type == VtdbToolType::VBit) {
        float angle = static_cast<float>(m_editGeometry.included_angle);
        if (ImGui::InputFloat("Included Angle", &angle, 1.0f, 5.0f, "%.1f")) {
            m_editGeometry.included_angle = angle;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("deg");

        float flatDia = static_cast<float>(m_editGeometry.flat_diameter);
        if (ImGui::InputFloat("Flat Diameter", &flatDia, 0.001f, 0.01f, "%.4f")) {
            m_editGeometry.flat_diameter = flatDia;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", unitSuffix);
    }

    // Notes
    char notesBuf[512] = {};
    std::strncpy(notesBuf, m_editGeometry.notes.c_str(), sizeof(notesBuf) - 1);
    if (ImGui::InputTextMultiline("Notes##geom", notesBuf, sizeof(notesBuf), ImVec2(-1, 60))) {
        m_editGeometry.notes = notesBuf;
    }

    // Cutting Data section
    ImGui::Spacing();
    ImGui::SeparatorText("Cutting Data");

    // Material selector
    if (ImGui::BeginCombo("Material##cd", m_selectedMaterialId.empty()
                                              ? "(All Materials)"
                                              : m_selectedMaterialId.c_str())) {
        // Find the name for display
        if (ImGui::Selectable("(All Materials)", m_selectedMaterialId.empty())) {
            m_selectedMaterialId.clear();
            selectTool(m_selectedGeometryId); // reload cutting data
        }
        for (const auto& mat : m_materials) {
            bool sel = (mat.id == m_selectedMaterialId);
            if (ImGui::Selectable(mat.name.c_str(), sel)) {
                m_selectedMaterialId = mat.id;
                selectTool(m_selectedGeometryId);
            }
        }
        ImGui::EndCombo();
    }

    // Machine selector
    if (ImGui::BeginCombo("Machine##cd", m_selectedMachineId.empty()
                                             ? "(Default Machine)"
                                             : m_selectedMachineId.c_str())) {
        if (ImGui::Selectable("(Default Machine)", m_selectedMachineId.empty())) {
            m_selectedMachineId.clear();
            selectTool(m_selectedGeometryId);
        }
        for (const auto& mach : m_machines) {
            bool sel = (mach.id == m_selectedMachineId);
            std::string machLabel = mach.name;
            if (!mach.make.empty()) machLabel += " (" + mach.make + ")";
            if (ImGui::Selectable(machLabel.c_str(), sel)) {
                m_selectedMachineId = mach.id;
                selectTool(m_selectedGeometryId);
            }
        }
        ImGui::EndCombo();
    }

    if (m_hasCuttingData) {
        float feedRate = static_cast<float>(m_editCuttingData.feed_rate);
        if (ImGui::InputFloat("Feed Rate", &feedRate, 1.0f, 10.0f, "%.1f")) {
            m_editCuttingData.feed_rate = feedRate;
        }

        float plungeRate = static_cast<float>(m_editCuttingData.plunge_rate);
        if (ImGui::InputFloat("Plunge Rate", &plungeRate, 1.0f, 10.0f, "%.1f")) {
            m_editCuttingData.plunge_rate = plungeRate;
        }

        int spindleSpeed = m_editCuttingData.spindle_speed;
        if (ImGui::InputInt("Spindle Speed (RPM)", &spindleSpeed, 100, 1000)) {
            m_editCuttingData.spindle_speed = std::max(0, spindleSpeed);
        }

        float stepdown = static_cast<float>(m_editCuttingData.stepdown);
        if (ImGui::InputFloat("Stepdown", &stepdown, 0.01f, 0.1f, "%.4f")) {
            m_editCuttingData.stepdown = stepdown;
        }

        float stepover = static_cast<float>(m_editCuttingData.stepover);
        if (ImGui::InputFloat("Stepover", &stepover, 0.01f, 0.1f, "%.4f")) {
            m_editCuttingData.stepover = stepover;
        }

        int toolNum = m_editCuttingData.tool_number;
        if (ImGui::InputInt("Tool Number", &toolNum)) {
            m_editCuttingData.tool_number = std::max(0, toolNum);
        }
    } else {
        ImGui::TextDisabled("No cutting data for this material/machine combination");
        if (ImGui::Button("Add Cutting Data")) {
            VtdbCuttingData cd;
            cd.feed_rate = 100.0;
            cd.plunge_rate = 50.0;
            cd.spindle_speed = 18000;
            cd.stepdown = 0.1;
            cd.stepover = 0.4;
            m_toolDatabase->insertCuttingData(cd);

            // Create entity linking geometry + material + machine -> cutting data
            VtdbToolEntity entity;
            entity.tool_geometry_id = m_selectedGeometryId;
            entity.material_id = m_selectedMaterialId;
            entity.machine_id = m_selectedMachineId;
            entity.tool_cutting_data_id = cd.id;
            m_toolDatabase->insertEntity(entity);

            selectTool(m_selectedGeometryId);
        }
    }

    // Save button
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    char saveBtnLabel[64];
    std::snprintf(saveBtnLabel, sizeof(saveBtnLabel), "%s Save Changes", Icons::Save);
    if (ImGui::Button(saveBtnLabel, ImVec2(150, 0))) {
        m_toolDatabase->updateGeometry(m_editGeometry);
        if (m_hasCuttingData) {
            m_toolDatabase->updateCuttingData(m_editCuttingData);
        }
        log::info("ToolBrowser", "Tool changes saved");
    }
}

void ToolBrowserPanel::renderAddToolPopup() {
    if (m_showAddTool) {
        ImGui::OpenPopup("Add Tool");
        m_showAddTool = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Tool", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Tool type
        if (ImGui::BeginCombo("Type", toolTypeName(static_cast<VtdbToolType>(m_addToolType)))) {
            for (int i = 0; i < kToolTypeCount; ++i) {
                if (ImGui::Selectable(toolTypeName(kToolTypes[i]),
                                      m_addToolType == static_cast<int>(kToolTypes[i]))) {
                    m_addToolType = static_cast<int>(kToolTypes[i]);
                }
            }
            ImGui::EndCombo();
        }

        char nameBuf[256] = {};
        std::strncpy(nameBuf, m_addToolName.c_str(), sizeof(nameBuf) - 1);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            m_addToolName = nameBuf;
        }

        ImGui::InputFloat("Diameter", &m_addToolDiameter, 0.01f, 0.1f, "%.4f");
        ImGui::InputInt("Flutes", &m_addToolFlutes);
        m_addToolFlutes = std::max(1, m_addToolFlutes);

        // Parent group selector
        if (ImGui::BeginCombo("Parent Group", m_addToolParentGroupId.empty()
                                                  ? "(Root)"
                                                  : m_addToolParentGroupId.c_str())) {
            if (ImGui::Selectable("(Root)", m_addToolParentGroupId.empty())) {
                m_addToolParentGroupId.clear();
            }
            for (const auto& entry : m_treeEntries) {
                if (entry.tool_geometry_id.empty()) { // groups only
                    if (ImGui::Selectable(entry.name.c_str(),
                                          entry.id == m_addToolParentGroupId)) {
                        m_addToolParentGroupId = entry.id;
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            // Auto-generate name if empty
            if (m_addToolName.empty()) {
                char autoName[128];
                std::snprintf(autoName, sizeof(autoName), "%s %.4f\" %dF",
                              toolTypeName(static_cast<VtdbToolType>(m_addToolType)),
                              m_addToolDiameter, m_addToolFlutes);
                m_addToolName = autoName;
            }

            VtdbToolGeometry geom;
            geom.tool_type = static_cast<VtdbToolType>(m_addToolType);
            geom.diameter = m_addToolDiameter;
            geom.num_flutes = m_addToolFlutes;
            m_toolDatabase->insertGeometry(geom);

            VtdbTreeEntry te;
            te.parent_group_id = m_addToolParentGroupId;
            te.tool_geometry_id = geom.id;
            te.name = m_addToolName;
            m_toolDatabase->insertTreeEntry(te);

            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ToolBrowserPanel::renderAddGroupPopup() {
    if (m_showAddGroup) {
        ImGui::OpenPopup("Add Group");
        m_showAddGroup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Group", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char nameBuf[256] = {};
        std::strncpy(nameBuf, m_addGroupName.c_str(), sizeof(nameBuf) - 1);
        if (ImGui::InputText("Group Name", nameBuf, sizeof(nameBuf))) {
            m_addGroupName = nameBuf;
        }

        // Parent group selector
        if (ImGui::BeginCombo("Parent", m_addGroupParentId.empty()
                                            ? "(Root)"
                                            : m_addGroupParentId.c_str())) {
            if (ImGui::Selectable("(Root)", m_addGroupParentId.empty())) {
                m_addGroupParentId.clear();
            }
            for (const auto& entry : m_treeEntries) {
                if (entry.tool_geometry_id.empty()) {
                    if (ImGui::Selectable(entry.name.c_str(),
                                          entry.id == m_addGroupParentId)) {
                        m_addGroupParentId = entry.id;
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            if (!m_addGroupName.empty()) {
                VtdbTreeEntry te;
                te.parent_group_id = m_addGroupParentId;
                te.name = m_addGroupName;
                // tool_geometry_id left empty = group node
                m_toolDatabase->insertTreeEntry(te);
                refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ToolBrowserPanel::refresh() {
    m_needsRefresh = true;
}

void ToolBrowserPanel::loadData() {
    if (!m_toolDatabase) return;

    m_treeEntries = m_toolDatabase->getAllTreeEntries();
    m_materials = m_toolDatabase->findAllMaterials();
    m_machines = m_toolDatabase->findAllMachines();
    m_geometries = m_toolDatabase->findAllGeometries();

    // Sort tree entries by sibling_order
    std::sort(m_treeEntries.begin(), m_treeEntries.end(),
              [](const VtdbTreeEntry& a, const VtdbTreeEntry& b) {
                  return a.sibling_order < b.sibling_order;
              });

    // Reload selected tool if still valid
    if (!m_selectedGeometryId.empty()) {
        selectTool(m_selectedGeometryId);
    }
}

void ToolBrowserPanel::selectTool(const std::string& geometryId) {
    m_selectedGeometryId = geometryId;

    auto geom = m_toolDatabase->findGeometryById(geometryId);
    if (geom) {
        m_editGeometry = *geom;
    }

    // Try to load cutting data for this geometry + material + machine
    auto view = m_toolDatabase->getToolView(geometryId, m_selectedMaterialId, m_selectedMachineId);
    if (view) {
        m_editCuttingData = view->cutting_data;
        m_hasCuttingData = true;
    } else {
        m_editCuttingData = {};
        m_hasCuttingData = false;
    }
}

void ToolBrowserPanel::deleteSelected() {
    if (m_selectedTreeEntryId.empty() || !m_toolDatabase) return;

    // Find the tree entry
    for (const auto& entry : m_treeEntries) {
        if (entry.id != m_selectedTreeEntryId) continue;

        if (!entry.tool_geometry_id.empty()) {
            // Delete geometry and associated entities/cutting data
            auto entities = m_toolDatabase->findEntitiesForGeometry(entry.tool_geometry_id);
            for (const auto& e : entities) {
                m_toolDatabase->removeCuttingData(e.tool_cutting_data_id);
                m_toolDatabase->removeEntity(e.id);
            }
            m_toolDatabase->removeGeometry(entry.tool_geometry_id);
        }
        m_toolDatabase->removeTreeEntry(entry.id);
        break;
    }

    m_selectedTreeEntryId.clear();
    m_selectedGeometryId.clear();
    m_hasCuttingData = false;
    refresh();
}

} // namespace dw
