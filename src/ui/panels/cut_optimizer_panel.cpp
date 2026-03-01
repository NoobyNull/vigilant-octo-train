#include "cut_optimizer_panel.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <imgui.h>

#include "../../core/database/model_repository.h"
#include "../../core/optimizer/cut_list_file.h"
#include "../../core/project/project.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

CutOptimizerPanel::CutOptimizerPanel() : Panel("Cut Optimizer") {
    m_canvas.zoomMax = 5.0f;
    std::strncpy(m_sheetName, "Plywood", sizeof(m_sheetName));
    m_sheet.cost = 45.0f;
    m_sheet.name = "Plywood";
}

// ---------------------------------------------------------------------------
// Main render — 3-column layout
// ---------------------------------------------------------------------------
void CutOptimizerPanel::render() {
    if (!m_open)
        return;

    applyMinSize(28, 14);
    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();
        ImGui::Separator();

        float avail = ImGui::GetContentRegionAvail().x;
        float availH = ImGui::GetContentRegionAvail().y;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float leftW = avail * 0.22f;
        float rightW = avail * 0.16f;
        float centerW = avail - leftW - rightW - spacing * 2;

        if (centerW < avail * 0.2f) {
            // Too narrow for 3-column — fall back to stacked
            float stackH = availH * 0.35f;
            float centerH = availH * 0.4f;
            float rightH = availH * 0.2f;

            ImGui::BeginChild("Left", ImVec2(0, stackH), ImGuiChildFlags_Borders);
            renderCutListTable();
            ImGui::Separator();
            renderStockSheets();
            ImGui::Separator();
            renderSettings();
            ImGui::EndChild();

            ImGui::BeginChild("Center", ImVec2(0, centerH), ImGuiChildFlags_Borders);
            if (m_hasResults) {
                renderVisualization();
            } else {
                ImGui::TextDisabled("Add parts and click Optimize");
            }
            ImGui::EndChild();

            if (m_hasResults) {
                ImGui::BeginChild("Right", ImVec2(0, rightH), ImGuiChildFlags_Borders);
                renderLayoutsSidebar();
                ImGui::Separator();
                renderResultsPanel();
                ImGui::EndChild();
            }
        } else {
            // --- 3-column layout ---
            float colH = -ImGui::GetFrameHeightWithSpacing() - 4;
            // Left column
            ImGui::BeginChild("Left", ImVec2(leftW, colH), ImGuiChildFlags_Borders);
            renderCutListTable();
            ImGui::Separator();
            renderStockSheets();
            ImGui::Separator();
            renderSettings();
            ImGui::EndChild();

            ImGui::SameLine();

            // Center column
            ImGui::BeginChild("Center", ImVec2(centerW, colH), ImGuiChildFlags_Borders);
            if (m_hasResults) {
                // Pagination header
                int sheetCount = static_cast<int>(m_result.sheets.size());
                if (sheetCount > 1) {
                    if (ImGui::ArrowButton("##prev", ImGuiDir_Left)) {
                        m_selectedSheet = std::max(0, m_selectedSheet - 1);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%d / %d", m_selectedSheet + 1, sheetCount);
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##next", ImGuiDir_Right)) {
                        m_selectedSheet = std::min(sheetCount - 1, m_selectedSheet + 1);
                    }
                    ImGui::Separator();
                }

                renderVisualization();

                // Info line below visualization
                if (m_selectedSheet < static_cast<int>(m_result.sheets.size())) {
                    const auto& sr = m_result.sheets[static_cast<size_t>(m_selectedSheet)];
                    int pieceCount = static_cast<int>(sr.placements.size());
                    float eff = sr.efficiency() * 100.0f;
                    ImGui::Text("%d pieces  %s  %.0fx%.0f  %.1f%%",
                                pieceCount, Icons::Optimizer,
                                static_cast<double>(m_sheet.width),
                                static_cast<double>(m_sheet.height),
                                static_cast<double>(eff));
                }
            } else {
                ImGui::TextDisabled("Add parts and click Optimize");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // Right column
            ImGui::BeginChild("Right", ImVec2(rightW, colH), ImGuiChildFlags_Borders);
            renderLayoutsSidebar();
            ImGui::Separator();
            renderResultsPanel();
            ImGui::EndChild();
        }

        // Status bar at the bottom
        renderStatusBar();
    }
    ImGui::End();

    // Render popups (must be outside Begin/End child)
    renderImportPopup();
}

// ---------------------------------------------------------------------------
// loadCutPlan / clear / saveCutPlan — same logic, minor adjustments
// ---------------------------------------------------------------------------
void CutOptimizerPanel::loadCutPlan(const CutListFile::LoadResult& lr) {
    m_sheet = lr.sheet;
    std::strncpy(m_sheetName, lr.sheet.name.c_str(), sizeof(m_sheetName));
    m_sheetName[sizeof(m_sheetName) - 1] = '\0';
    m_parts = lr.parts;
    m_result = lr.result;
    m_hasResults = !lr.result.sheets.empty();
    m_selectedSheet = 0;
    m_allowRotation = lr.allowRotation;
    m_kerf = lr.kerf;
    m_margin = lr.margin;
    m_algorithm = (lr.algorithm == "first_fit_decreasing")
                      ? optimizer::Algorithm::FirstFitDecreasing
                      : optimizer::Algorithm::Guillotine;
}

void CutOptimizerPanel::clear() {
    m_parts.clear();
    m_result = optimizer::CutPlan{};
    m_hasResults = false;
    m_selectedSheet = 0;
    m_editPartIdx = -1;
    m_canvas.reset();
}

void CutOptimizerPanel::saveCutPlan(const char* name) {
    if (!m_cutListFile || !m_hasResults)
        return;

    std::string algorithm = (m_algorithm == optimizer::Algorithm::Guillotine)
                                ? "guillotine"
                                : "first_fit_decreasing";

    if (m_cutListFile->save(name, m_sheet, m_parts, m_result,
                            algorithm, m_allowRotation, m_kerf, m_margin)) {
        ToastManager::instance().show(ToastType::Success, "Saved", "Cut plan saved");
    }
}

std::string CutOptimizerPanel::nextPartLabel() const {
    // A, B, C, ... Z, AA, AB, ...
    int n = static_cast<int>(m_parts.size());
    std::string label;
    do {
        label.insert(label.begin(), static_cast<char>('A' + (n % 26)));
        n = n / 26 - 1;
    } while (n >= 0);
    return label;
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderToolbar() {
    if (ImGui::Button("Optimize"))
        runOptimization();

    ImGui::SameLine();

    // Save
    if (!m_hasResults || !m_cutListFile)
        ImGui::BeginDisabled();
    if (ImGui::Button(Icons::Save))
        ImGui::OpenPopup("Save Cut Plan");
    if (!m_hasResults || !m_cutListFile)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // Load
    if (!m_cutListFile)
        ImGui::BeginDisabled();
    if (ImGui::Button("Load"))
        ImGui::OpenPopup("Load Cut Plan");
    if (!m_cutListFile)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // Import from Project
    bool canImport = m_modelRepo && m_projectManager && m_projectManager->currentProject();
    if (!canImport)
        ImGui::BeginDisabled();
    if (ImGui::Button("Import from Project")) {
        m_importPopupOpen = true;
    }
    if (!canImport)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // Algorithm combo
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("Guillotine__").x + ImGui::GetStyle().FramePadding.x * 2);
    const char* algorithms[] = {"First Fit", "Guillotine"};
    int algoIdx = static_cast<int>(m_algorithm);
    if (ImGui::Combo("##algo", &algoIdx, algorithms, 2))
        m_algorithm = static_cast<optimizer::Algorithm>(algoIdx);

    ImGui::SameLine();

    if (ImGui::Button("Clear"))
        clear();

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
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Load popup
    if (ImGui::BeginPopup("Load Cut Plan")) {
        auto plans = m_cutListFile ? m_cutListFile->list() : std::vector<CutListMeta>{};
        if (plans.empty()) {
            ImGui::TextDisabled("No saved plans");
        } else {
            for (const auto& meta : plans) {
                char label[256];
                std::snprintf(label, sizeof(label), "%s (%d sheets, %d%%)",
                              meta.name.c_str(), meta.sheetsUsed,
                              static_cast<int>(meta.efficiency * 100));
                if (ImGui::Selectable(label)) {
                    auto loaded = m_cutListFile->load(meta.filePath);
                    if (loaded) {
                        loadCutPlan(*loaded);
                        m_loadedPlanPath = meta.filePath;
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// CUT LIST — editable table with inline editing
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderCutListTable() {
    ImGui::Text("%s CUT LIST", Icons::Model);

    if (m_parts.empty()) {
        ImGui::TextDisabled("No parts added");
    }

    // Cost rate per mm²
    float sheetArea = m_sheet.area();
    float costPerMm2 = (sheetArea > 0.0f && m_sheet.cost > 0.0f) ? m_sheet.cost / sheetArea : 0.0f;

    // Parts table
    if (!m_parts.empty() &&
        ImGui::BeginTable("##parts", 7,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
                          ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 8))) {
        float charW = ImGui::CalcTextSize("0").x;
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed, charW * 5);
        ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthFixed, charW * 5);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, charW * 3);
        ImGui::TableSetupColumn("$", ImGuiTableColumnFlags_WidthFixed, charW * 5);
        ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_WidthFixed, charW * 2);
        ImGui::TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed, charW * 2);
        ImGui::TableHeadersRow();

        int toRemove = -1;
        for (int i = 0; i < static_cast<int>(m_parts.size()); ++i) {
            auto& part = m_parts[static_cast<size_t>(i)];
            ImGui::PushID(i);
            ImGui::TableNextRow();

            bool editing = (m_editPartIdx == i);

            // Label
            ImGui::TableNextColumn();
            if (editing) {
                static char editBuf[64];
                if (ImGui::IsWindowAppearing())
                    std::strncpy(editBuf, part.name.c_str(), sizeof(editBuf));
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##lbl", editBuf, sizeof(editBuf),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
                    part.name = editBuf;
                    m_editPartIdx = -1;
                }
            } else {
                ImGui::TextUnformatted(part.name.c_str());
                if (ImGui::IsItemClicked())
                    m_editPartIdx = i;
            }

            // Width
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##w", &part.width, 0, 0, "%.0f"))
                m_hasResults = false;

            // Height
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##h", &part.height, 0, 0, "%.0f"))
                m_hasResults = false;

            // Quantity
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputInt("##q", &part.quantity, 0)) {
                if (part.quantity < 1) part.quantity = 1;
                m_hasResults = false;
            }

            // Per-part cost
            ImGui::TableNextColumn();
            {
                float partCost = part.area() * static_cast<float>(part.quantity) * costPerMm2;
                ImGui::Text("%.2f", static_cast<double>(partCost));
            }

            // Rotation toggle
            ImGui::TableNextColumn();
            if (ImGui::Checkbox("##r", &part.canRotate))
                m_hasResults = false;

            // Delete
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("X"))
                toRemove = i;

            ImGui::PopID();
        }
        ImGui::EndTable();

        if (toRemove >= 0) {
            m_parts.erase(m_parts.begin() + toRemove);
            if (m_editPartIdx == toRemove)
                m_editPartIdx = -1;
            else if (m_editPartIdx > toRemove)
                --m_editPartIdx;
            m_hasResults = false;
        }
    }

    // Add row
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("0000.0").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputFloat("##nw", &m_newPartWidth, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("0000.0").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputFloat("##nh", &m_newPartHeight, 0, 0, "%.0f");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("000").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputInt("##nq", &m_newPartQuantity, 0);
    if (m_newPartQuantity < 1) m_newPartQuantity = 1;
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Add")) {
        optimizer::Part part;
        part.width = m_newPartWidth;
        part.height = m_newPartHeight;
        part.quantity = m_newPartQuantity;
        part.name = m_newPartName[0] ? m_newPartName
                                     : nextPartLabel();
        part.id = static_cast<i64>(m_parts.size());
        m_parts.push_back(part);
        m_newPartName[0] = '\0';
        m_hasResults = false;
    }
}

// ---------------------------------------------------------------------------
// STOCK SHEETS — presets + editable current sheet
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderStockSheets() {
    ImGui::Text("%s STOCK", Icons::Folder);

    // Preset buttons
    for (const auto& preset : kPresets) {
        char lbl[64];
        std::snprintf(lbl, sizeof(lbl), "%s $%.0f", preset.name, static_cast<double>(preset.cost));
        if (ImGui::Button(lbl)) {
            m_sheet.width = preset.width;
            m_sheet.height = preset.height;
            m_sheet.cost = preset.cost;
            m_sheet.name = preset.name;
            std::strncpy(m_sheetName, preset.name, sizeof(m_sheetName));
            m_hasResults = false;
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // Editable current sheet fields
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("Name##sheet", m_sheetName, sizeof(m_sheetName)))
        m_sheet.name = m_sheetName;

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000.00").x + ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::InputFloat("W##sheet", &m_sheet.width, 0, 0, "%.0f"))
        m_hasResults = false;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000.00").x + ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::InputFloat("H##sheet", &m_sheet.height, 0, 0, "%.0f"))
        m_hasResults = false;

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000.00").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputFloat("$/sheet", &m_sheet.cost, 0, 0, "%.2f");

    // Grain direction indicator
    const char* grainLabel = m_sheet.grainHorizontal ? "Grain: \xe2\x86\x94 Horizontal"
                                                     : "Grain: \xe2\x86\x95 Vertical";
    if (ImGui::Button(grainLabel, ImVec2(-1, 0)))
        m_sheet.grainHorizontal = !m_sheet.grainHorizontal;
}

// ---------------------------------------------------------------------------
// SETTINGS — kerf, padding, Optimize button
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderSettings() {
    ImGui::Text("%s SETTINGS", Icons::Settings);

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000.00").x + ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::InputFloat("Kerf", &m_kerf, 0, 0, "%.1f"))
        m_hasResults = false;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("00000.00").x + ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::InputFloat("Padding", &m_margin, 0, 0, "%.1f"))
        m_hasResults = false;

    ImGui::Checkbox("Allow Rotation", &m_allowRotation);

    // Full-width Optimize button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.33f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.60f, 0.40f, 1.0f));
    if (ImGui::Button("Optimize", ImVec2(-1, ImGui::GetFrameHeight() * 1.3f)))
        runOptimization();
    ImGui::PopStyleColor(2);
}

// ---------------------------------------------------------------------------
// VISUALIZATION — reuse existing Canvas2D
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderVisualization() {
    if (m_result.sheets.empty())
        return;

    auto area = m_canvas.begin();
    if (!area)
        return;

    // Background
    area.drawList->AddRectFilled(area.pos,
                                 ImVec2(area.pos.x + area.size.x, area.pos.y + area.size.y),
                                 IM_COL32(40, 40, 40, 255));

    // Scale to fit sheet
    float scaleX = (area.size.x - 20) / m_sheet.width;
    float scaleY = (area.size.y - 20) / m_sheet.height;
    float scale = m_canvas.effectiveScale(std::min(scaleX, scaleY));

    float offsetX = area.pos.x + 10 + m_canvas.panX;
    float offsetY = area.pos.y + 10 + m_canvas.panY;

    // Sheet outline
    ImVec2 sheetP1(offsetX, offsetY);
    ImVec2 sheetP2(offsetX + m_sheet.width * scale, offsetY + m_sheet.height * scale);
    area.drawList->AddRectFilled(sheetP1, sheetP2, IM_COL32(60, 60, 60, 255));
    area.drawList->AddRect(sheetP1, sheetP2, IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Grain direction lines
    {
        ImU32 grainColor = IM_COL32(80, 75, 65, 80);
        float spacing = 12.0f;
        if (m_sheet.grainHorizontal) {
            for (float gy = sheetP1.y + spacing; gy < sheetP2.y; gy += spacing)
                area.drawList->AddLine(ImVec2(sheetP1.x, gy), ImVec2(sheetP2.x, gy), grainColor);
        } else {
            for (float gx = sheetP1.x + spacing; gx < sheetP2.x; gx += spacing)
                area.drawList->AddLine(ImVec2(gx, sheetP1.y), ImVec2(gx, sheetP2.y), grainColor);
        }
    }

    // Draw placements for selected sheet
    if (m_selectedSheet < static_cast<int>(m_result.sheets.size())) {
        const auto& sheetResult = m_result.sheets[static_cast<size_t>(m_selectedSheet)];

        ImU32 colors[] = {
            IM_COL32(66, 133, 244, 200),  // Blue
            IM_COL32(234, 67, 53, 200),   // Red
            IM_COL32(251, 188, 5, 200),   // Yellow
            IM_COL32(52, 168, 83, 200),   // Green
            IM_COL32(171, 71, 188, 200),  // Purple
            IM_COL32(255, 112, 67, 200),  // Orange
            IM_COL32(0, 172, 193, 200),   // Cyan
            IM_COL32(124, 179, 66, 200),  // Lime
        };
        const int numColors = sizeof(colors) / sizeof(colors[0]);

        for (const auto& placement : sheetResult.placements) {
            float w = placement.getWidth() * scale;
            float h = placement.getHeight() * scale;
            float x = offsetX + placement.x * scale;
            float y = offsetY + placement.y * scale;

            ImU32 color = colors[placement.partIndex % numColors];

            area.drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
            area.drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
                                   IM_COL32(255, 255, 255, 128));

            // Part label
            if (placement.part && w > 30 && h > 15) {
                ImVec2 textPos(x + 3, y + 2);
                area.drawList->AddText(textPos, IM_COL32(255, 255, 255, 255),
                                       placement.part->name.c_str());
            }
        }
    }

    m_canvas.handleInput("cut_canvas");
}

// ---------------------------------------------------------------------------
// LAYOUTS SIDEBAR — per-sheet efficiency bars
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderLayoutsSidebar() {
    int sheetCount = m_hasResults ? static_cast<int>(m_result.sheets.size()) : 0;
    ImGui::Text("%s LAYOUTS  %d sheets", Icons::Optimizer, sheetCount);

    if (!m_hasResults || m_result.sheets.empty()) {
        ImGui::TextDisabled("No results");
        return;
    }

    for (int i = 0; i < static_cast<int>(m_result.sheets.size()); ++i) {
        const auto& sr = m_result.sheets[static_cast<size_t>(i)];
        float eff = sr.efficiency();
        int pieces = static_cast<int>(sr.placements.size());

        ImGui::PushID(i);

        bool selected = (m_selectedSheet == i);
        char label[64];
        std::snprintf(label, sizeof(label), "Sheet %d", i + 1);

        if (ImGui::Selectable(label, selected)) {
            m_selectedSheet = i;
        }

        // Piece count + efficiency on same line
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("00pc 100%").x);
        ImGui::Text("%dpc %d%%", pieces, static_cast<int>(eff * 100));

        // Color-coded progress bar
        ImVec4 barColor;
        if (eff > 0.75f)
            barColor = ImVec4(0.2f, 0.7f, 0.3f, 1.0f); // green
        else if (eff > 0.50f)
            barColor = ImVec4(0.85f, 0.65f, 0.1f, 1.0f); // yellow
        else
            barColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // red

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
        ImGui::ProgressBar(eff, ImVec2(-1, 4), "");
        ImGui::PopStyleColor();

        ImGui::PopID();
    }
}

// ---------------------------------------------------------------------------
// RESULTS PANEL — stats + cost
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderResultsPanel() {
    ImGui::Text("%s RESULTS", Icons::Info);

    if (!m_hasResults) {
        ImGui::TextDisabled("Run optimizer first");
        return;
    }

    float eff = m_result.overallEfficiency() * 100.0f;

    // Large efficiency number
    ImGui::PushFont(nullptr); // default font, but big text via separator trick
    ImGui::Text("%.1f%%", static_cast<double>(eff));
    ImGui::PopFont();
    ImGui::TextDisabled("Efficiency");

    ImGui::Spacing();
    ImGui::Text("Sheets: %d", m_result.sheetsUsed);

    // Cost breakdown
    float totalCost = static_cast<float>(m_result.sheetsUsed) * m_sheet.cost;
    float sheetArea = m_sheet.area();
    float rate = (sheetArea > 0.0f) ? m_sheet.cost / sheetArea : 0.0f;

    // Parts value: sum of placed part areas
    float partsValue = m_result.totalUsedArea * rate;

    // Kerf (dust): estimate from placements — kerf strip on 2 edges per piece
    float kerfArea = 0.0f;
    for (const auto& sr : m_result.sheets) {
        for (const auto& pl : sr.placements) {
            float pw = pl.getWidth();
            float ph = pl.getHeight();
            kerfArea += m_kerf * (pw + ph);
        }
    }
    float dustValue = kerfArea * rate;

    // Remaining offcut value
    float offcutValue = totalCost - partsValue - dustValue;
    if (offcutValue < 0.0f) offcutValue = 0.0f;

    ImGui::Spacing();
    ImGui::Text("Cost: $%.2f", static_cast<double>(totalCost));
    ImGui::TextDisabled("%d x $%.2f", m_result.sheetsUsed, static_cast<double>(m_sheet.cost));
    ImGui::Text("  Parts:  $%.2f", static_cast<double>(partsValue));
    ImGui::Text("  Offcut: $%.2f", static_cast<double>(offcutValue));
    ImGui::Text("  Dust:   $%.2f", static_cast<double>(dustValue));

    // Unplaced warning
    if (!m_result.unplacedParts.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("%zu parts could not be placed!",
                           m_result.unplacedParts.size());
        ImGui::PopStyleColor();
    }

    // Add to Cost Estimate button
    ImGui::Spacing();
    if (!m_onAddToCost)
        ImGui::BeginDisabled();
    if (ImGui::Button("Add to Cost Est.", ImVec2(-1, 0))) {
        if (m_onAddToCost) {
            m_onAddToCost(m_sheet.name.empty() ? "Cut Plan Sheets" : m_sheet.name,
                          m_result.sheetsUsed,
                          m_sheet.cost,
                          totalCost);
            ToastManager::instance().show(ToastType::Success, "Added",
                                          "Cut plan cost added to estimate");
        }
    }
    if (!m_onAddToCost)
        ImGui::EndDisabled();
}

// ---------------------------------------------------------------------------
// STATUS BAR
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderStatusBar() {
    // Count unique part types and total pieces
    int types = static_cast<int>(m_parts.size());
    int totalPieces = 0;
    for (const auto& p : m_parts)
        totalPieces += p.quantity;

    int sheetsUsed = m_hasResults ? m_result.sheetsUsed : 0;
    float eff = m_hasResults ? m_result.overallEfficiency() * 100.0f : 0.0f;
    int placed = 0;
    if (m_hasResults) {
        for (const auto& sr : m_result.sheets)
            placed += static_cast<int>(sr.placements.size());
    }

    ImGui::Separator();
    ImGui::Text("%d types  %s  %d pieces    %s    %d sheets  %s  %.1f%% eff    %s    %d placed",
                types, "\xc2\xb7", totalPieces,
                "\xc2\xb7",
                sheetsUsed, "\xc2\xb7", static_cast<double>(eff),
                "\xc2\xb7",
                placed);
}

// ---------------------------------------------------------------------------
// IMPORT FROM PROJECT popup
// ---------------------------------------------------------------------------
void CutOptimizerPanel::renderImportPopup() {
    if (m_importPopupOpen) {
        ImGui::OpenPopup("Import from Project");
        m_importPopupOpen = false;
    }

    if (ImGui::BeginPopupModal("Import from Project", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!m_modelRepo || !m_projectManager || !m_projectManager->currentProject()) {
            ImGui::Text("No project open");
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        auto& modelIds = m_projectManager->currentProject()->modelIds();
        if (m_importSelection.size() != modelIds.size())
            m_importSelection.assign(modelIds.size(), 1);

        ImGui::Text("Select models to import as cut parts:");
        ImGui::Text("Projection: XZ plane (top-down bounding box)");
        ImGui::Separator();

        const auto* vp = ImGui::GetMainViewport();
        ImVec2 listSize(vp->WorkSize.x * 0.3f, vp->WorkSize.y * 0.3f);
        if (ImGui::BeginChild("ModelList", listSize, ImGuiChildFlags_Borders)) {
            for (size_t i = 0; i < modelIds.size(); ++i) {
                auto record = m_modelRepo->findById(modelIds[i]);
                if (!record)
                    continue;

                ImGui::PushID(static_cast<int>(i));
                bool sel = m_importSelection[i] != 0;
                if (ImGui::Checkbox("##sel", &sel))
                    m_importSelection[i] = sel ? 1 : 0;
                ImGui::SameLine();

                // Model name + XZ dimensions
                Vec3 size = record->boundsMax - record->boundsMin;
                float partW = size.x; // XZ projection
                float partH = size.z;
                ImGui::Text("%s  (%.0f x %.0f mm)",
                            record->name.c_str(),
                            static_cast<double>(partW),
                            static_cast<double>(partH));
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Import Selected")) {
            int imported = 0;
            for (size_t i = 0; i < modelIds.size(); ++i) {
                if (!m_importSelection[i])
                    continue; // NOLINT
                auto record = m_modelRepo->findById(modelIds[i]);
                if (!record)
                    continue;

                Vec3 size = record->boundsMax - record->boundsMin;
                optimizer::Part part;
                part.id = record->id;
                part.name = record->name;
                part.width = size.x;
                part.height = size.z;
                part.quantity = 1;
                m_parts.push_back(part);
                ++imported;
            }
            m_hasResults = false;
            m_importSelection.clear();
            ToastManager::instance().show(ToastType::Success, "Imported",
                                          std::to_string(imported) + " parts added");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_importSelection.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// runOptimization — same as before
// ---------------------------------------------------------------------------
void CutOptimizerPanel::runOptimization() {
    if (m_parts.empty()) {
        ToastManager::instance().show(ToastType::Warning, "No Parts",
                                      "Add parts before running optimization");
        return;
    }

    auto optimizer = optimizer::CutOptimizer::create(m_algorithm);
    if (!optimizer) {
        ToastManager::instance().show(ToastType::Error, "Optimizer Error",
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

    int unplacedCount = static_cast<int>(m_result.unplacedParts.size());
    if (unplacedCount > 0) {
        ToastManager::instance().show(ToastType::Warning, "Incomplete Layout",
                                      std::to_string(unplacedCount) + " parts could not be placed");
    } else {
        ToastManager::instance().show(ToastType::Success, "Optimization Complete",
                                      "All parts placed successfully");
    }
}

} // namespace dw
