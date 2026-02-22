#include "cut_optimizer_panel.h"

#include <algorithm>
#include <cstring>

#include <imgui.h>

#include "../../core/project/project.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

CutOptimizerPanel::CutOptimizerPanel() : Panel("Cut Optimizer") {
    m_canvas.zoomMax = 5.0f;
}

void CutOptimizerPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();

        // Responsive layout: side-by-side when wide, stacked when narrow
        float availWidth = ImGui::GetContentRegionAvail().x;

        if (availWidth < 500.0f) {
            // Stacked layout for narrow docks
            ImGui::BeginChild("Config", ImVec2(0, 300), true);
            renderPartsEditor();
            ImGui::Separator();
            renderSheetConfig();
            ImGui::EndChild();

            // Results below
            ImGui::BeginChild("Results", ImVec2(0, 0), true);
            if (m_hasResults) {
                renderResults();
                ImGui::Separator();
                renderVisualization();
            } else {
                ImGui::TextDisabled("Add parts and click 'Optimize' to see results");
            }
            ImGui::EndChild();
        } else {
            // Side-by-side layout for wide docks
            float configWidth = std::min(300.0f, availWidth * 0.45f);

            // Left column: configuration
            ImGui::BeginChild("Config", ImVec2(configWidth, 0), true);
            renderPartsEditor();
            ImGui::Separator();
            renderSheetConfig();
            ImGui::EndChild();

            ImGui::SameLine();

            // Right column: results
            ImGui::BeginChild("Results", ImVec2(availWidth - configWidth - 8, 0), true);
            if (m_hasResults) {
                renderResults();
                ImGui::Separator();
                renderVisualization();
            } else {
                ImGui::TextDisabled("Add parts and click 'Optimize' to see results");
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void CutOptimizerPanel::loadCutPlan(const CutPlanRecord& record) {
    // Deserialize sheet config
    if (!record.sheetConfigJson.empty()) {
        m_sheet = CutPlanRepository::jsonToSheet(record.sheetConfigJson);
    }

    // Deserialize parts
    if (!record.partsJson.empty()) {
        m_parts = CutPlanRepository::jsonToParts(record.partsJson);
    } else {
        m_parts.clear();
    }

    // Deserialize result
    if (!record.resultJson.empty()) {
        m_result = CutPlanRepository::jsonToCutPlan(record.resultJson);
        m_hasResults = true;
        m_selectedSheet = 0;
    } else {
        m_result = optimizer::CutPlan{};
        m_hasResults = false;
    }

    // Track loaded plan for updates
    m_loadedPlanId = record.id;

    // Restore settings
    m_allowRotation = record.allowRotation;
    m_kerf = record.kerf;
    m_margin = record.margin;

    // Restore algorithm
    if (record.algorithm == "first_fit_decreasing") {
        m_algorithm = optimizer::Algorithm::FirstFitDecreasing;
    } else {
        m_algorithm = optimizer::Algorithm::Guillotine;
    }
}

void CutOptimizerPanel::clear() {
    m_parts.clear();
    m_result = optimizer::CutPlan{};
    m_hasResults = false;
    m_selectedSheet = 0;
    m_canvas.reset();
}

void CutOptimizerPanel::renderToolbar() {
    if (ImGui::Button("Optimize")) {
        runOptimization();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        clear();
    }

    ImGui::SameLine();

    // Save button — enabled when we have results and a repository
    if (!m_hasResults || !m_cutPlanRepo) ImGui::BeginDisabled();
    if (ImGui::Button("Save")) {
        ImGui::OpenPopup("Save Cut Plan");
    }
    if (!m_hasResults || !m_cutPlanRepo) ImGui::EndDisabled();

    ImGui::SameLine();

    // Load button — show list of saved plans
    if (!m_cutPlanRepo) ImGui::BeginDisabled();
    if (ImGui::Button("Load")) {
        ImGui::OpenPopup("Load Cut Plan");
    }
    if (!m_cutPlanRepo) ImGui::EndDisabled();

    // Save popup
    if (ImGui::BeginPopup("Save Cut Plan")) {
        static char planName[128] = {};
        ImGui::InputText("Name", planName, sizeof(planName));
        if (ImGui::Button("Save##confirm") && planName[0] != '\0') {
            saveCutPlan(planName);
            planName[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Load popup
    if (ImGui::BeginPopup("Load Cut Plan")) {
        auto plans = m_cutPlanRepo ? m_cutPlanRepo->findAll() : std::vector<CutPlanRecord>{};
        if (plans.empty()) {
            ImGui::TextDisabled("No saved plans");
        } else {
            for (const auto& plan : plans) {
                std::string label = plan.name + " (" +
                    std::to_string(plan.sheetsUsed) + " sheets, " +
                    std::to_string(static_cast<int>(plan.efficiency * 100)) + "%%)";
                if (ImGui::Selectable(label.c_str())) {
                    loadCutPlan(plan);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    float availWidth = ImGui::GetContentRegionAvail().x;

    // Wrap Algorithm combo and checkbox to new line at narrow widths
    if (availWidth < 400.0f) {
        // Not enough space, put controls on new line
        ImGui::NewLine();
        availWidth = ImGui::GetContentRegionAvail().x;
    }

    // Algorithm selection
    ImGui::SetNextItemWidth(120);
    const char* algorithms[] = {"First Fit", "Guillotine"};
    int algoIdx = static_cast<int>(m_algorithm);
    if (ImGui::Combo("Algorithm", &algoIdx, algorithms, 2)) {
        m_algorithm = static_cast<optimizer::Algorithm>(algoIdx);
    }

    ImGui::SameLine();
    ImGui::Checkbox("Allow Rotation", &m_allowRotation);
}

void CutOptimizerPanel::renderPartsEditor() {
    ImGui::Text("%s Parts", Icons::Model);

    // Add new part
    ImGui::SetNextItemWidth(60);
    ImGui::InputFloat("##w", &m_newPartWidth, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputFloat("##h", &m_newPartHeight, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::Text("mm");

    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("##qty", &m_newPartQuantity);
    if (m_newPartQuantity < 1)
        m_newPartQuantity = 1;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##name", m_newPartName, sizeof(m_newPartName));
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        optimizer::Part part;
        part.width = m_newPartWidth;
        part.height = m_newPartHeight;
        part.quantity = m_newPartQuantity;
        part.name = m_newPartName[0] ? m_newPartName : "Part " + std::to_string(m_parts.size() + 1);
        part.id = static_cast<i64>(m_parts.size());
        m_parts.push_back(part);
        m_newPartName[0] = '\0';
        m_hasResults = false; // Invalidate results
    }

    ImGui::Separator();

    // Parts list
    if (m_parts.empty()) {
        ImGui::TextDisabled("No parts added");
    } else {
        ImGui::BeginChild("PartsList", ImVec2(0, 150), true);
        int toRemove = -1;
        for (size_t i = 0; i < m_parts.size(); ++i) {
            auto& part = m_parts[i];
            ImGui::PushID(static_cast<int>(i));

            float availWidth = ImGui::GetContentRegionAvail().x;

            if (availWidth < 200.0f) {
                // Very narrow: stack everything vertically
                ImGui::TextWrapped("%s", part.name.c_str());
                ImGui::Text("%.0f x %.0f",
                            static_cast<double>(part.width),
                            static_cast<double>(part.height));
                ImGui::Text("x%d", part.quantity);
                if (ImGui::SmallButton("X")) {
                    toRemove = static_cast<int>(i);
                }
            } else {
                // Use proportional positioning for side-by-side layout
                ImGui::TextWrapped("%s", part.name.c_str());
                ImGui::SameLine(availWidth * 0.45f);
                ImGui::Text("%.0f x %.0f",
                            static_cast<double>(part.width),
                            static_cast<double>(part.height));
                ImGui::SameLine(availWidth * 0.7f);
                ImGui::Text("x%d", part.quantity);
                ImGui::SameLine(availWidth * 0.82f);
                if (ImGui::SmallButton("X")) {
                    toRemove = static_cast<int>(i);
                }
            }

            ImGui::PopID();
        }
        ImGui::EndChild();

        if (toRemove >= 0) {
            m_parts.erase(m_parts.begin() + toRemove);
            m_hasResults = false;
        }
    }
}

void CutOptimizerPanel::renderSheetConfig() {
    ImGui::Text("%s Sheet Settings", Icons::Folder);

    float availWidth = ImGui::GetContentRegionAvail().x;

    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("Width##sheet", &m_sheet.width, 0, 0, "%.0f")) {
        m_hasResults = false;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("Height##sheet", &m_sheet.height, 0, 0, "%.0f")) {
        m_hasResults = false;
    }
    ImGui::SameLine();
    ImGui::Text("mm");

    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("Kerf", &m_kerf, 0, 0, "%.1f")) {
        m_hasResults = false;
    }

    // Stack Margin at narrow widths
    if (availWidth < 250.0f) {
        // Narrow: stack Margin below Kerf
    } else {
        ImGui::SameLine();
    }

    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("Margin", &m_margin, 0, 0, "%.1f")) {
        m_hasResults = false;
    }

    // Common presets
    if (ImGui::Button("4x8 ft")) {
        m_sheet.width = 2440.0f;
        m_sheet.height = 1220.0f;
        m_hasResults = false;
    }

    // Stack preset buttons at narrow widths
    if (availWidth < 300.0f) {
        // Narrow: stack preset buttons
    } else {
        ImGui::SameLine();
    }

    if (ImGui::Button("5x5 ft")) {
        m_sheet.width = 1524.0f;
        m_sheet.height = 1524.0f;
        m_hasResults = false;
    }

    if (availWidth < 300.0f) {
        // Narrow: stack preset buttons
    } else {
        ImGui::SameLine();
    }

    if (ImGui::Button("MDF")) {
        m_sheet.width = 2440.0f;
        m_sheet.height = 1220.0f;
        m_hasResults = false;
    }
}

void CutOptimizerPanel::renderResults() {
    ImGui::Text("%s Results", Icons::Info);

    // Summary statistics
    float efficiency = m_result.overallEfficiency() * 100.0f;
    ImGui::Text("Sheets Used: %d", m_result.sheetsUsed);
    ImGui::Text("Efficiency: %.1f%%", static_cast<double>(efficiency));
    ImGui::Text("Used Area: %.0f mm²", static_cast<double>(m_result.totalUsedArea));
    ImGui::Text("Waste Area: %.0f mm²", static_cast<double>(m_result.totalWasteArea));

    // Unplaced parts warning
    if (!m_result.unplacedParts.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("Warning: %zu parts could not be placed!",
                           m_result.unplacedParts.size());
        ImGui::PopStyleColor();
    }

    // Sheet selector
    if (m_result.sheetsUsed > 1) {
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputInt("View Sheet", &m_selectedSheet)) {
            m_selectedSheet = std::max(0, std::min(m_selectedSheet, m_result.sheetsUsed - 1));
        }
    }
}

void CutOptimizerPanel::renderVisualization() {
    if (m_result.sheets.empty())
        return;

    // Canvas for 2D visualization
    auto area = m_canvas.begin();
    if (!area)
        return;

    // Background
    area.drawList->AddRectFilled(area.pos,
                                 ImVec2(area.pos.x + area.size.x, area.pos.y + area.size.y),
                                 IM_COL32(40, 40, 40, 255));

    // Calculate scale to fit sheet
    float scaleX = (area.size.x - 20) / m_sheet.width;
    float scaleY = (area.size.y - 20) / m_sheet.height;
    float scale = m_canvas.effectiveScale(std::min(scaleX, scaleY));

    float offsetX = area.pos.x + 10 + m_canvas.panX;
    float offsetY = area.pos.y + 10 + m_canvas.panY;

    // Draw sheet outline
    ImVec2 sheetP1(offsetX, offsetY);
    ImVec2 sheetP2(offsetX + m_sheet.width * scale, offsetY + m_sheet.height * scale);
    area.drawList->AddRectFilled(sheetP1, sheetP2, IM_COL32(60, 60, 60, 255));
    area.drawList->AddRect(sheetP1, sheetP2, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Draw placements for selected sheet
    if (m_selectedSheet < static_cast<int>(m_result.sheets.size())) {
        const auto& sheetResult = m_result.sheets[static_cast<size_t>(m_selectedSheet)];

        // Color palette for parts
        ImU32 colors[] = {
            IM_COL32(66, 133, 244, 200), // Blue
            IM_COL32(234, 67, 53, 200),  // Red
            IM_COL32(251, 188, 5, 200),  // Yellow
            IM_COL32(52, 168, 83, 200),  // Green
            IM_COL32(171, 71, 188, 200), // Purple
            IM_COL32(255, 112, 67, 200), // Orange
            IM_COL32(0, 172, 193, 200),  // Cyan
            IM_COL32(124, 179, 66, 200), // Lime
        };
        const int numColors = sizeof(colors) / sizeof(colors[0]);

        for (const auto& placement : sheetResult.placements) {
            float w = placement.getWidth() * scale;
            float h = placement.getHeight() * scale;
            float x = offsetX + placement.x * scale;
            float y = offsetY + placement.y * scale;

            ImU32 color = colors[placement.partIndex % numColors];

            area.drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
            area.drawList->AddRect(ImVec2(x, y),
                                   ImVec2(x + w, y + h),
                                   IM_COL32(255, 255, 255, 128));

            // Part label
            if (placement.part && w > 30 && h > 15) {
                const char* name = placement.part->name.c_str();
                ImVec2 textPos(x + 3, y + 2);
                area.drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), name);
            }
        }
    }

    // Handle input for pan/zoom
    m_canvas.handleInput("cut_canvas");
}

void CutOptimizerPanel::saveCutPlan(const char* name) {
    if (!m_cutPlanRepo || !m_hasResults) return;

    CutPlanRecord record;
    record.name = name;
    record.algorithm = (m_algorithm == optimizer::Algorithm::Guillotine)
        ? "guillotine" : "first_fit_decreasing";
    record.sheetConfigJson = CutPlanRepository::sheetToJson(m_sheet);
    record.partsJson = CutPlanRepository::partsToJson(m_parts);
    record.resultJson = CutPlanRepository::cutPlanToJson(m_result);
    record.allowRotation = m_allowRotation;
    record.kerf = m_kerf;
    record.margin = m_margin;
    record.sheetsUsed = m_result.sheetsUsed;
    record.efficiency = m_result.overallEfficiency();

    // Link to current project if one is open
    if (m_projectManager && m_projectManager->currentProject()) {
        record.projectId = m_projectManager->currentProject()->id();
    }

    auto id = m_cutPlanRepo->insert(record);
    if (id) {
        m_loadedPlanId = *id;
        ToastManager::instance().show(ToastType::Success,
            "Saved", "Cut plan saved");
    }
}

void CutOptimizerPanel::runOptimization() {
    if (m_parts.empty()) {
        ToastManager::instance().show(ToastType::Warning,
                                      "No Parts",
                                      "Add parts before running optimization");
        return;
    }

    auto optimizer = optimizer::CutOptimizer::create(m_algorithm);
    if (!optimizer) {
        ToastManager::instance().show(ToastType::Error,
                                      "Optimizer Error",
                                      "Failed to create optimizer");
        return;
    }

    optimizer->setAllowRotation(m_allowRotation);
    optimizer->setKerf(m_kerf);
    optimizer->setMargin(m_margin);

    std::vector<optimizer::Sheet> sheets = {m_sheet};
    m_result = optimizer->optimize(m_parts, sheets);
    m_hasResults = true;
    m_selectedSheet = 0;

    // Check for unplaced parts
    int unplacedCount = static_cast<int>(m_result.unplacedParts.size());

    if (unplacedCount > 0) {
        ToastManager::instance().show(ToastType::Warning,
                                      "Incomplete Layout",
                                      std::to_string(unplacedCount) + " parts could not be placed");
    } else {
        ToastManager::instance().show(ToastType::Success,
                                      "Optimization Complete",
                                      "All parts placed successfully");
    }
}

} // namespace dw
