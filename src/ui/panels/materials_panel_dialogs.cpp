// materials_panel_dialogs.cpp — Extracted dialog rendering for MaterialsPanel
// Contains: renderAddDialog, setGeneratedResult, renderEditForm, renderDeleteConfirm

#include "materials_panel.h"

#include <cstring>

#include <imgui.h>

#include "../../core/materials/material_archive.h"
#include "../../core/utils/log.h"
#include "../icons.h"

namespace dw {

// ---------------------------------------------------------------------------
// Add Material dialog (AI-first)
// ---------------------------------------------------------------------------

void MaterialsPanel::renderAddDialog() {
    if (m_showAddDialog) {
        ImGui::OpenPopup("Add Material");
        m_showAddDialog = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Add Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        bool closing = m_closeAddDialog;
        if (closing) {
            m_closeAddDialog = false;
        }

        ImGui::Text("Material Name");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(m_isGenerating);
        ImGui::InputTextWithHint("##AddName",
                                 "e.g. Walnut, Cherry, Maple...",
                                 m_generatePrompt,
                                 sizeof(m_generatePrompt));
        ImGui::EndDisabled();

        ImGui::Spacing();
        bool promptEmpty = (m_generatePrompt[0] == '\0');

        // Generate with AI button (primary action)
        ImGui::BeginDisabled(promptEmpty || m_isGenerating);
        std::string genLabel = std::string(Icons::Wand) + " Generate with AI";
        if (ImGui::Button(genLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            if (m_onGenerate && !promptEmpty) {
                m_isGenerating = true;
                m_onGenerate(std::string(m_generatePrompt));
            }
        }
        ImGui::EndDisabled();

        if (m_isGenerating) {
            ImGui::TextDisabled("Generating material via AI...");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Manual entry button
        ImGui::BeginDisabled(m_isGenerating);
        if (ImGui::Button("Manual Entry", ImVec2(120, 0))) {
            m_editBuffer = MaterialRecord{};
            m_editBuffer.name = promptEmpty ? "New Material" : std::string(m_generatePrompt);
            m_isNewMaterial = true;
            m_showEditForm = true;
            closing = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_isGenerating = false;
            closing = true;
        }

        if (closing) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MaterialsPanel::setGeneratedResult(const MaterialRecord& record, const Path& dwmatPath) {
    m_isGenerating = false;
    m_editBuffer = record;
    m_editBuffer.archivePath = dwmatPath;
    m_isNewMaterial = true;
    m_showEditForm = true;
    m_closeAddDialog = true; // Signal renderAddDialog() to close on next frame
}

// ---------------------------------------------------------------------------
// Edit form (modal)
// ---------------------------------------------------------------------------

void MaterialsPanel::renderEditForm() {
    if (m_showEditForm) {
        ImGui::OpenPopup("Edit Material");
        m_showEditForm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 0), ImVec2(500, FLT_MAX));

    if (ImGui::BeginPopupModal("Edit Material", nullptr, 0)) {
        ImGui::Text(m_isNewMaterial ? "New Material" : "Edit Material");
        ImGui::Separator();
        ImGui::Spacing();

        // Name
        char nameBuf[256];
        std::strncpy(nameBuf, m_editBuffer.name.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##MatName", nameBuf, sizeof(nameBuf))) {
            m_editBuffer.name = nameBuf;
        }
        ImGui::SameLine(0, 0);
        ImGui::TextDisabled(" Name");

        ImGui::Spacing();

        // Category combo
        const char* categories[] = {"Hardwood", "Softwood", "Domestic", "Composite"};
        int catIndex = static_cast<int>(m_editBuffer.category);
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::Combo("Category", &catIndex, categories, 4)) {
            m_editBuffer.category = static_cast<MaterialCategory>(catIndex);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("CNC Parameters");
        ImGui::Spacing();

        // Janka Hardness
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat(
            "Janka Hardness (lbf)", &m_editBuffer.jankaHardness, 10.0f, 100.0f, "%.0f");

        // Feed Rate
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Feed Rate (in/min)", &m_editBuffer.feedRate, 1.0f, 10.0f, "%.1f");

        // Spindle Speed
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat(
            "Spindle Speed (RPM)", &m_editBuffer.spindleSpeed, 100.0f, 1000.0f, "%.0f");

        // Depth of Cut
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat("Depth of Cut (in)", &m_editBuffer.depthOfCut, 0.01f, 0.1f, "%.3f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Economics");
        ImGui::Spacing();

        // Cost per Board Foot
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputFloat(
            "Cost per Board Foot ($)", &m_editBuffer.costPerBoardFoot, 0.1f, 1.0f, "%.2f");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Grain");
        ImGui::Spacing();

        // Grain Direction
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat(
            "Grain Direction (deg)", &m_editBuffer.grainDirectionDeg, 0.0f, 360.0f, "%.1f deg");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            if (m_materialManager && !m_editBuffer.name.empty()) {
                if (m_isNewMaterial && !m_editBuffer.archivePath.empty()) {
                    // AI-generated: import the .dwmat archive, then apply user edits
                    auto importedId = m_materialManager->importMaterial(m_editBuffer.archivePath);
                    if (importedId) {
                        m_editBuffer.id = *importedId;
                        m_materialManager->updateMaterial(m_editBuffer);
                        refresh();
                    } else {
                        log::error("MaterialsPanel", "Failed to import AI-generated material");
                    }
                } else if (m_isNewMaterial) {
                    auto newId = m_materialManager->addMaterial(m_editBuffer);
                    if (newId) {
                        refresh();
                    } else {
                        log::error("MaterialsPanel", "Failed to add new material");
                    }
                } else {
                    if (m_materialManager->updateMaterial(m_editBuffer)) {
                        refresh();
                    } else {
                        log::error("MaterialsPanel", "Failed to update material");
                    }
                }
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

// ---------------------------------------------------------------------------
// Delete confirmation
// ---------------------------------------------------------------------------

void MaterialsPanel::renderDeleteConfirm() {
    if (m_showDeleteConfirm) {
        ImGui::OpenPopup("Delete Material?");
        m_showDeleteConfirm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Delete Material?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete \"%s\"?", m_deleteName.c_str());
        ImGui::TextDisabled("This action cannot be undone.");
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (m_materialManager && m_deleteId != -1) {
                if (m_materialManager->removeMaterial(m_deleteId)) {
                    if (m_selectedMaterialId == m_deleteId) {
                        m_selectedMaterialId = -1;
                    }
                    refresh();
                } else {
                    log::error("MaterialsPanel", "Failed to delete material");
                }
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

} // namespace dw
