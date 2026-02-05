#include "cut_optimizer_panel.h"

#include "../icons.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>

namespace dw {

CutOptimizerPanel::CutOptimizerPanel() : Panel("Cut Optimizer") {}

void CutOptimizerPanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();

        // Two-column layout: config on left, results on right
        float configWidth = 300.0f;
        float availWidth = ImGui::GetContentRegionAvail().x;

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
    ImGui::End();
}

void CutOptimizerPanel::clear() {
    m_parts.clear();
    m_result = optimizer::CutPlan{};
    m_hasResults = false;
    m_selectedSheet = 0;
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
    ImGui::Separator();
    ImGui::SameLine();

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
    if (m_newPartQuantity < 1) m_newPartQuantity = 1;
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
        m_hasResults = false;  // Invalidate results
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

            ImGui::Text("%s", part.name.c_str());
            ImGui::SameLine(150);
            ImGui::Text("%.0f x %.0f", static_cast<double>(part.width),
                        static_cast<double>(part.height));
            ImGui::SameLine(230);
            ImGui::Text("x%d", part.quantity);
            ImGui::SameLine(260);
            if (ImGui::SmallButton("X")) {
                toRemove = static_cast<int>(i);
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
    ImGui::SameLine();
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
    ImGui::SameLine();
    if (ImGui::Button("5x5 ft")) {
        m_sheet.width = 1524.0f;
        m_sheet.height = 1524.0f;
        m_hasResults = false;
    }
    ImGui::SameLine();
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
        ImGui::Text("Warning: %zu parts could not be placed!", m_result.unplacedParts.size());
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
    if (m_result.sheets.empty()) return;

    // Canvas for 2D visualization
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 50 || canvasSize.y < 50) return;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasPos,
                            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(40, 40, 40, 255));

    // Calculate scale to fit sheet
    float scaleX = (canvasSize.x - 20) / m_sheet.width;
    float scaleY = (canvasSize.y - 20) / m_sheet.height;
    float scale = std::min(scaleX, scaleY) * m_zoom;

    float offsetX = canvasPos.x + 10 + m_panX;
    float offsetY = canvasPos.y + 10 + m_panY;

    // Draw sheet outline
    ImVec2 sheetP1(offsetX, offsetY);
    ImVec2 sheetP2(offsetX + m_sheet.width * scale, offsetY + m_sheet.height * scale);
    drawList->AddRectFilled(sheetP1, sheetP2, IM_COL32(60, 60, 60, 255));
    drawList->AddRect(sheetP1, sheetP2, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Draw placements for selected sheet
    if (m_selectedSheet < static_cast<int>(m_result.sheets.size())) {
        const auto& sheetResult = m_result.sheets[static_cast<size_t>(m_selectedSheet)];

        // Color palette for parts
        ImU32 colors[] = {
            IM_COL32(66, 133, 244, 200),   // Blue
            IM_COL32(234, 67, 53, 200),    // Red
            IM_COL32(251, 188, 5, 200),    // Yellow
            IM_COL32(52, 168, 83, 200),    // Green
            IM_COL32(171, 71, 188, 200),   // Purple
            IM_COL32(255, 112, 67, 200),   // Orange
            IM_COL32(0, 172, 193, 200),    // Cyan
            IM_COL32(124, 179, 66, 200),   // Lime
        };
        const int numColors = sizeof(colors) / sizeof(colors[0]);

        for (const auto& placement : sheetResult.placements) {
            float w = placement.getWidth() * scale;
            float h = placement.getHeight() * scale;
            float x = offsetX + placement.x * scale;
            float y = offsetY + placement.y * scale;

            ImU32 color = colors[placement.partIndex % numColors];

            drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
            drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
                              IM_COL32(255, 255, 255, 128));

            // Part label
            if (placement.part && w > 30 && h > 15) {
                const char* name = placement.part->name.c_str();
                ImVec2 textPos(x + 3, y + 2);
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), name);
            }
        }
    }

    // Handle input for pan/zoom
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("canvas", canvasSize);
    if (ImGui::IsItemHovered()) {
        auto& io = ImGui::GetIO();
        if (io.MouseWheel != 0) {
            m_zoom *= (1.0f + io.MouseWheel * 0.1f);
            m_zoom = std::max(0.1f, std::min(5.0f, m_zoom));
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            m_panX += io.MouseDelta.x;
            m_panY += io.MouseDelta.y;
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            m_zoom = 1.0f;
            m_panX = 0;
            m_panY = 0;
        }
    }
}

void CutOptimizerPanel::runOptimization() {
    if (m_parts.empty()) {
        return;
    }

    auto optimizer = optimizer::CutOptimizer::create(m_algorithm);
    optimizer->setAllowRotation(m_allowRotation);
    optimizer->setKerf(m_kerf);
    optimizer->setMargin(m_margin);

    std::vector<optimizer::Sheet> sheets = {m_sheet};
    m_result = optimizer->optimize(m_parts, sheets);
    m_hasResults = true;
    m_selectedSheet = 0;
}

}  // namespace dw
