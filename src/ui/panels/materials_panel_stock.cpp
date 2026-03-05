#include "materials_panel.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <imgui.h>

#include "../../core/config/config.h"
#include "../../core/utils/board_foot.h"
#include "../../core/utils/unit_conversion.h"
#include "../icons.h"

namespace dw {

// ---------------------------------------------------------------------------
// Unit label helpers
// ---------------------------------------------------------------------------

static constexpr const char* kUnitLabels[] = {"sheet", "board foot", "linear foot", "each"};
static constexpr const char* kUnitDisplayLabels[] = {
    "Per Sheet", "Per Board Foot", "Per Linear Foot", "Per Each"};
static constexpr int kUnitCount = 4;

static int unitLabelToIndex(const std::string& label) {
    for (int i = 0; i < kUnitCount; ++i) {
        if (label == kUnitLabels[i]) return i;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Category-aware presets
// ---------------------------------------------------------------------------

struct DimensionPreset {
    const char* label;
    f64 thicknessMm;
    f64 widthMm;
    f64 heightMm;
};

// Sheet goods presets (Composite materials)
static constexpr DimensionPreset kSheetPresets[] = {
    {"4' x 8' (1220 x 2440 mm)", 0.0, 1219.2, 2438.4},
    {"5' x 5' (1524 x 1524 mm)", 0.0, 1524.0, 1524.0},
    {"2' x 4' (610 x 1220 mm)", 0.0, 609.6, 1219.2},
    {"2' x 2' (610 x 610 mm)", 0.0, 609.6, 609.6},
};

// Hardwood/Softwood thickness presets (quarter sizes)
static constexpr DimensionPreset kThicknessPresets[] = {
    {"4/4 (3/4\" / 19mm)", 19.05, 0.0, 0.0},
    {"5/4 (1\" / 25.4mm)", 25.4, 0.0, 0.0},
    {"6/4 (1-1/4\" / 31.75mm)", 31.75, 0.0, 0.0},
    {"8/4 (1-3/4\" / 44.45mm)", 44.45, 0.0, 0.0},
    {"10/4 (2-1/4\" / 57.15mm)", 57.15, 0.0, 0.0},
    {"12/4 (2-3/4\" / 69.85mm)", 69.85, 0.0, 0.0},
};

static void fillDimBuffer(char* buf, size_t bufSize, f64 valueMm, bool isMetric) {
    if (valueMm <= 0.0) {
        buf[0] = '\0';
        return;
    }
    std::string formatted = formatDimensionCompact(valueMm, isMetric);
    std::snprintf(buf, bufSize, "%s", formatted.c_str());
}

// ---------------------------------------------------------------------------
// Detail view
// ---------------------------------------------------------------------------

void MaterialsPanel::renderDetailView() {
    // Find the material record for the detail view
    const MaterialRecord* detailMat = nullptr;
    for (const auto& mat : m_allMaterials) {
        if (mat.id == m_detailMaterialId) {
            detailMat = &mat;
            break;
        }
    }

    if (!detailMat) {
        m_showDetailView = false;
        return;
    }

    // Header: material name + category badge + close button
    ImGui::Text("%s", detailMat->name.c_str());
    ImGui::SameLine();

    // Category badge
    const char* catName = materialCategoryToString(detailMat->category).c_str();
    ImGui::TextDisabled("(%s)", catName);
    ImGui::SameLine();

    // Close button (right-aligned)
    float closeW = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - closeW);
    if (ImGui::SmallButton("X")) {
        m_showDetailView = false;
        return;
    }

    ImGui::Separator();

    // Stock Sizes header with Add button
    ImGui::Text("Stock Sizes");
    ImGui::SameLine();
    if (ImGui::SmallButton(Icons::Add)) {
        // Open add popup
        m_editingStockId = -1;
        m_stockEditBuffer = StockSize{};
        m_stockEditBuffer.materialId = m_detailMaterialId;
        m_stockEditBuffer.unitLabel = "sheet";
        std::memset(m_stockDimBufs, 0, sizeof(m_stockDimBufs));
        std::memset(m_stockPriceBuf, 0, sizeof(m_stockPriceBuf));
        std::memset(m_stockNameBuf, 0, sizeof(m_stockNameBuf));
        m_stockUnitIndex = 0;
        m_showStockAddPopup = true;
        ImGui::OpenPopup("StockAddEdit");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add stock size");
    }

    ImGui::Spacing();

    if (m_stockSizes.empty()) {
        ImGui::TextDisabled("Add stock sizes to use this material in projects");
        return;
    }

    renderStockSizeTable();
}

// ---------------------------------------------------------------------------
// Stock size table
// ---------------------------------------------------------------------------

void MaterialsPanel::renderStockSizeTable() {
    bool isMetric = Config::instance().getDisplayUnitsMetric();

    constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                           ImGuiTableFlags_SizingStretchProp |
                                           ImGuiTableFlags_NoHostExtendX;

    if (!ImGui::BeginTable("StockSizeTable", 7, tableFlags)) {
        return;
    }

    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("Unit", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("BdFt", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("$/BF", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("##Actions", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    for (const auto& stock : m_stockSizes) {
        ImGui::PushID(static_cast<int>(stock.id));
        ImGui::TableNextRow();

        // Name
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(stock.name.c_str());

        // Dimensions (WxHxT)
        ImGui::TableNextColumn();
        {
            std::string dimStr;
            bool hasW = stock.widthMm > 0.0;
            bool hasH = stock.heightMm > 0.0;
            bool hasT = stock.thicknessMm > 0.0;

            if (hasW && hasH && hasT) {
                dimStr = formatDimensionCompact(stock.widthMm, isMetric) + " x " +
                         formatDimensionCompact(stock.heightMm, isMetric) + " x " +
                         formatDimensionCompact(stock.thicknessMm, isMetric);
            } else if (hasT) {
                dimStr = formatDimensionCompact(stock.thicknessMm, isMetric) + " thick";
            } else {
                dimStr = "--";
            }
            ImGui::TextUnformatted(dimStr.c_str());
        }

        // Unit
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(stock.unitLabel.c_str());

        // Price
        ImGui::TableNextColumn();
        {
            char priceBuf[32];
            std::snprintf(priceBuf, sizeof(priceBuf), "$%.2f", stock.pricePerUnit);
            ImGui::TextUnformatted(priceBuf);
        }

        // Board feet
        ImGui::TableNextColumn();
        {
            f64 bf = calcBoardFeet(stock.thicknessMm, stock.widthMm, stock.heightMm);
            ImGui::TextUnformatted(formatBoardFeet(bf).c_str());
        }

        // Cost per board foot
        ImGui::TableNextColumn();
        {
            f64 costBF = calcCostPerBoardFoot(
                stock.pricePerUnit, stock.unitLabel, stock.thicknessMm, stock.widthMm,
                stock.heightMm);
            ImGui::TextUnformatted(formatCostPerBoardFoot(costBF).c_str());
        }

        // Actions: Edit + Delete
        ImGui::TableNextColumn();
        {
            // Edit button (pencil = settings icon as substitute)
            if (ImGui::SmallButton(Icons::Settings)) {
                // Populate edit buffer
                m_editingStockId = stock.id;
                m_stockEditBuffer = stock;
                m_stockUnitIndex = unitLabelToIndex(stock.unitLabel);

                bool metric = Config::instance().getDisplayUnitsMetric();
                fillDimBuffer(m_stockDimBufs[0], sizeof(m_stockDimBufs[0]), stock.widthMm, metric);
                fillDimBuffer(
                    m_stockDimBufs[1], sizeof(m_stockDimBufs[1]), stock.heightMm, metric);
                fillDimBuffer(
                    m_stockDimBufs[2], sizeof(m_stockDimBufs[2]), stock.thicknessMm, metric);
                std::snprintf(
                    m_stockPriceBuf, sizeof(m_stockPriceBuf), "%.2f", stock.pricePerUnit);
                std::snprintf(m_stockNameBuf, sizeof(m_stockNameBuf), "%s", stock.name.c_str());

                m_showStockAddPopup = true;
                ImGui::OpenPopup("StockAddEdit");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Edit stock size");
            }

            ImGui::SameLine();

            // Delete button
            if (ImGui::SmallButton(Icons::Delete)) {
                m_stockDeleteId = stock.id;
                m_showStockDeleteConfirm = true;
                ImGui::OpenPopup("StockDeleteConfirm");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Delete stock size");
            }
        }

        ImGui::PopID();
    }

    ImGui::EndTable();
}

// ---------------------------------------------------------------------------
// Add / Edit popup
// ---------------------------------------------------------------------------

void MaterialsPanel::renderStockAddEditPopup() {
    if (!m_showStockAddPopup) return;

    bool isMetric = Config::instance().getDisplayUnitsMetric();
    const char* popupTitle = (m_editingStockId < 0) ? "Add Stock Size" : "Edit Stock Size";

    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always); // auto-size
    if (ImGui::BeginPopupModal(popupTitle, &m_showStockAddPopup, ImGuiWindowFlags_AlwaysAutoResize)) {

        // Name (optional)
        ImGui::Text("Name (optional):");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##StockName", m_stockNameBuf, sizeof(m_stockNameBuf));

        ImGui::Spacing();

        // Pricing unit dropdown
        ImGui::Text("Pricing Unit:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##PricingUnit", &m_stockUnitIndex, kUnitDisplayLabels, kUnitCount)) {
            m_stockEditBuffer.unitLabel = kUnitLabels[m_stockUnitIndex];
        }

        ImGui::Spacing();

        // Category-aware presets
        {
            // Find material category
            MaterialCategory category = MaterialCategory::Hardwood;
            for (const auto& mat : m_allMaterials) {
                if (mat.id == m_detailMaterialId) {
                    category = mat.category;
                    break;
                }
            }

            const DimensionPreset* presets = nullptr;
            int presetCount = 0;

            if (category == MaterialCategory::Composite) {
                presets = kSheetPresets;
                presetCount = static_cast<int>(sizeof(kSheetPresets) / sizeof(kSheetPresets[0]));
            } else {
                // Hardwood, Softwood, Domestic all get thickness presets
                presets = kThicknessPresets;
                presetCount =
                    static_cast<int>(sizeof(kThicknessPresets) / sizeof(kThicknessPresets[0]));
            }

            ImGui::Text("Common Sizes:");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##Presets", "Select preset...")) {
                for (int i = 0; i < presetCount; ++i) {
                    if (ImGui::Selectable(presets[i].label)) {
                        if (presets[i].widthMm > 0.0) {
                            fillDimBuffer(m_stockDimBufs[0], sizeof(m_stockDimBufs[0]),
                                          presets[i].widthMm, isMetric);
                        }
                        if (presets[i].heightMm > 0.0) {
                            fillDimBuffer(m_stockDimBufs[1], sizeof(m_stockDimBufs[1]),
                                          presets[i].heightMm, isMetric);
                        }
                        if (presets[i].thicknessMm > 0.0) {
                            fillDimBuffer(m_stockDimBufs[2], sizeof(m_stockDimBufs[2]),
                                          presets[i].thicknessMm, isMetric);
                        }
                        // Jump focus to price field
                        ImGui::SetKeyboardFocusHere(3); // skip 3 fields ahead to price
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Dimension inputs
        float inputWidth = ImGui::GetContentRegionAvail().x;
        const char* unitHint = isMetric ? "mm" : "inches (or use \" ' mm cm m)";

        ImGui::Text("Thickness *:");
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputTextWithHint("##Thickness", unitHint, m_stockDimBufs[2],
                                 sizeof(m_stockDimBufs[2]));

        ImGui::Text("Width:");
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputTextWithHint("##Width", unitHint, m_stockDimBufs[0],
                                 sizeof(m_stockDimBufs[0]));

        ImGui::Text("Height / Length:");
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputTextWithHint("##Height", unitHint, m_stockDimBufs[1],
                                 sizeof(m_stockDimBufs[1]));

        ImGui::Spacing();

        // Price input
        ImGui::Text("Price:");
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##Price", m_stockPriceBuf, sizeof(m_stockPriceBuf),
                         ImGuiInputTextFlags_CharsDecimal);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Save / Cancel buttons
        bool canSave = (std::strlen(m_stockDimBufs[2]) > 0); // Thickness required

        if (!canSave) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Save")) {
            // Parse dimensions
            auto thickness = parseDimension(m_stockDimBufs[2], isMetric);
            auto width = parseDimension(m_stockDimBufs[0], isMetric);
            auto height = parseDimension(m_stockDimBufs[1], isMetric);

            if (thickness.has_value() && *thickness > 0.0) {
                m_stockEditBuffer.thicknessMm = *thickness;
                m_stockEditBuffer.widthMm = width.value_or(0.0);
                m_stockEditBuffer.heightMm = height.value_or(0.0);
                m_stockEditBuffer.unitLabel = kUnitLabels[m_stockUnitIndex];

                // Parse price
                try {
                    m_stockEditBuffer.pricePerUnit = std::stod(m_stockPriceBuf);
                } catch (...) {
                    m_stockEditBuffer.pricePerUnit = 0.0;
                }

                // Name: use custom or auto-generate
                if (std::strlen(m_stockNameBuf) > 0) {
                    m_stockEditBuffer.name = m_stockNameBuf;
                } else {
                    // Auto-generate: "MaterialName WxHxT"
                    std::string matName;
                    for (const auto& mat : m_allMaterials) {
                        if (mat.id == m_detailMaterialId) {
                            matName = mat.name;
                            break;
                        }
                    }
                    std::string dimStr = formatDimensionCompact(m_stockEditBuffer.thicknessMm,
                                                                isMetric);
                    if (m_stockEditBuffer.widthMm > 0.0) {
                        dimStr = formatDimensionCompact(m_stockEditBuffer.widthMm, isMetric) +
                                 "x" +
                                 (m_stockEditBuffer.heightMm > 0.0
                                      ? formatDimensionCompact(m_stockEditBuffer.heightMm,
                                                               isMetric) +
                                            "x"
                                      : "") +
                                 formatDimensionCompact(m_stockEditBuffer.thicknessMm, isMetric);
                    }
                    m_stockEditBuffer.name = matName + " " + dimStr;
                }

                m_stockEditBuffer.materialId = m_detailMaterialId;

                if (m_editingStockId < 0) {
                    m_materialManager->addStockSize(m_stockEditBuffer);
                } else {
                    m_stockEditBuffer.id = m_editingStockId;
                    m_materialManager->updateStockSize(m_stockEditBuffer);
                }

                refreshStockSizes();
                m_showStockAddPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }
        if (!canSave) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            m_showStockAddPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } else {
        // Popup wasn't open yet -- need to open it
        if (m_showStockAddPopup) {
            const char* title = (m_editingStockId < 0) ? "Add Stock Size" : "Edit Stock Size";
            ImGui::OpenPopup(title);
        }
    }
}

// ---------------------------------------------------------------------------
// Delete confirmation
// ---------------------------------------------------------------------------

void MaterialsPanel::renderStockDeleteConfirm() {
    if (!m_showStockDeleteConfirm) return;

    if (ImGui::BeginPopupModal("Delete Stock Size?", &m_showStockDeleteConfirm,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        // Find name
        std::string stockName;
        for (const auto& s : m_stockSizes) {
            if (s.id == m_stockDeleteId) {
                stockName = s.name;
                break;
            }
        }

        ImGui::Text("Delete stock size '%s'?", stockName.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Delete")) {
            if (m_materialManager) {
                m_materialManager->removeStockSize(m_stockDeleteId);
                refreshStockSizes();
            }
            m_showStockDeleteConfirm = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            m_showStockDeleteConfirm = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } else {
        if (m_showStockDeleteConfirm) {
            ImGui::OpenPopup("Delete Stock Size?");
        }
    }
}

} // namespace dw
