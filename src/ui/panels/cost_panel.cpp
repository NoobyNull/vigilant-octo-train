#include "ui/panels/cost_panel.h"

#include <cstdio>
#include <cstring>

#include "ui/icons.h"
#include "ui/widgets/toast.h"

namespace dw {

// Display names for the 5 cost categories
static const char* categoryDisplayName(const std::string& cat) {
    if (cat == CostCat::Material) return "Material Costs";
    if (cat == CostCat::Tooling) return "Tooling Costs";
    if (cat == CostCat::Consumable) return "Consumable Costs";
    if (cat == CostCat::Labor) return "Labor Costs";
    if (cat == CostCat::Overhead) return "Overhead";
    return "Other";
}

CostingPanel::CostingPanel(CostRepository* repo, RateCategoryRepository* rateCatRepo)
    : Panel("Project Costing"), m_repo(repo), m_rateCatRepo(rateCatRepo) {
    if (m_repo) {
        m_records = m_repo->findAll();
    }
    refreshRateCategories();
}

void CostingPanel::render() {
    if (!m_open) {
        return;
    }

    applyMinSize(22, 10);
    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    renderToolbar();
    ImGui::Separator();

    // Two-column layout: list (left) | editor (right)
    float listWidth = ImGui::GetContentRegionAvail().x * 0.22f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("RecordList", ImVec2(listWidth, avail.y), true)) {
        renderRecordList();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("RecordEditor", ImVec2(0, avail.y), true)) {
        if (m_selectedIndex >= 0 || m_isNewRecord) {
            renderRecordEditor();
        } else {
            ImGui::TextDisabled("Select or create a cost record");
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void CostingPanel::renderToolbar() {
    if (ImGui::Button(Icons::Add)) {
        // Create new record
        m_editBuffer = CostingRecord{};
        m_editBuffer.name = "New Record";
        std::memset(m_editName, 0, sizeof(m_editName));
        std::snprintf(m_editName, sizeof(m_editName), "%s", m_editBuffer.name.c_str());
        std::memset(m_editNotes, 0, sizeof(m_editNotes));
        m_editTaxRate = 0.0f;
        m_editDiscountRate = 0.0f;
        m_isNewRecord = true;
        m_selectedIndex = -1; // Will be set after insert
        m_engine.setEntries({});
        refreshRateCategories();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("New Record");
    }

    ImGui::SameLine();

    bool hasSelection = m_selectedIndex >= 0 &&
                        m_selectedIndex < static_cast<int>(m_records.size());
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button(Icons::Delete)) {
        if (hasSelection) {
            i64 id = m_records[static_cast<size_t>(m_selectedIndex)].id;
            if (m_repo->remove(id)) {
                ToastManager::instance().show(ToastType::Success, "Record Deleted");
                m_records = m_repo->findAll();
                m_selectedIndex = -1;
                m_isNewRecord = false;
                m_engine.setEntries({});
                refreshRateCategories();
            } else {
                ToastManager::instance().show(ToastType::Error, "Delete Failed");
            }
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Delete Record");
    }

    ImGui::SameLine();

    if (ImGui::Button(Icons::Refresh)) {
        if (m_repo) {
            m_records = m_repo->findAll();
            m_selectedIndex = -1;
            m_isNewRecord = false;
            m_engine.setEntries({});
            refreshRateCategories();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh");
    }
}

void CostingPanel::renderRecordList() {
    for (int i = 0; i < static_cast<int>(m_records.size()); ++i) {
        const auto& rec = m_records[static_cast<size_t>(i)];
        char label[128];
        std::snprintf(label, sizeof(label), "%s\n$%.2f", rec.name.c_str(), rec.total);

        bool selected = (i == m_selectedIndex && !m_isNewRecord);
        if (ImGui::Selectable(label, selected, 0, ImVec2(0, ImGui::GetTextLineHeight() * 2.5f))) {
            m_selectedIndex = i;
            m_isNewRecord = false;
            m_editBuffer = rec;
            std::memset(m_editName, 0, sizeof(m_editName));
            std::snprintf(m_editName, sizeof(m_editName), "%s", rec.name.c_str());
            std::memset(m_editNotes, 0, sizeof(m_editNotes));
            std::snprintf(m_editNotes, sizeof(m_editNotes), "%s", rec.notes.c_str());
            m_editTaxRate = static_cast<float>(rec.taxRate);
            m_editDiscountRate = static_cast<float>(rec.discountRate);
            syncEngineFromRecord();
            refreshRateCategories();
        }
    }

    // Show new record entry if creating
    if (m_isNewRecord) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        ImGui::Selectable("(New Record)", true);
        ImGui::PopStyleColor();
    }
}

void CostingPanel::renderRecordEditor() {
    // Name input
    ImGui::Text("Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##RecName", m_editName, sizeof(m_editName));

    ImGui::Spacing();

    // Sync engine from record and recalculate
    syncEngineFromRecord();
    m_engine.recalculate();

    // Category-grouped sections
    renderCategorySection(CostCat::Material, "Material Costs");
    renderCategorySection(CostCat::Tooling, "Tooling Costs");
    renderCategorySection(CostCat::Consumable, "Consumable Costs");
    renderCategorySection(CostCat::Labor, "Labor Costs");
    renderCategorySection(CostCat::Overhead, "Overhead");

    ImGui::Spacing();
    ImGui::Separator();

    // Summary totals
    renderCostingSummary();

    ImGui::Spacing();

    // Tax and discount
    const auto& style = ImGui::GetStyle();
    float percentFieldW = ImGui::CalcTextSize("100.00%").x + style.FramePadding.x * 2;
    ImGui::SetNextItemWidth(percentFieldW);
    ImGui::InputFloat("Tax %", &m_editTaxRate, 0, 0, "%.2f");
    ImGui::SameLine(0, style.ItemSpacing.x * 3);
    ImGui::SetNextItemWidth(percentFieldW);
    ImGui::InputFloat("Discount %", &m_editDiscountRate, 0, 0, "%.2f");

    // Calculate totals
    m_editBuffer.taxRate = static_cast<f64>(m_editTaxRate);
    m_editBuffer.discountRate = static_cast<f64>(m_editDiscountRate);
    syncRecordFromEngine();
    recalculateEditBuffer();

    ImGui::Spacing();

    // Notes
    ImGui::Text("Notes:");
    ImGui::InputTextMultiline("##RecNotes", m_editNotes, sizeof(m_editNotes), ImVec2(-1, 60));

    ImGui::Spacing();

    // Save button
    std::string saveLabel = std::string(Icons::Save) + " Save";
    float saveBtnW = ImGui::CalcTextSize(saveLabel.c_str()).x + style.FramePadding.x * 4;
    if (ImGui::Button(saveLabel.c_str(), ImVec2(saveBtnW, 0))) {
        m_editBuffer.name = m_editName;
        m_editBuffer.notes = m_editNotes;

        bool ok = false;
        if (m_isNewRecord) {
            auto newId = m_repo->insert(m_editBuffer);
            ok = newId.has_value();
            if (ok) {
                m_isNewRecord = false;
            }
        } else {
            ok = m_repo->update(m_editBuffer);
        }

        if (ok) {
            ToastManager::instance().show(ToastType::Success, "Record Saved");
            m_records = m_repo->findAll();
            // Re-select by name
            for (int i = 0; i < static_cast<int>(m_records.size()); ++i) {
                if (m_records[static_cast<size_t>(i)].name == m_editBuffer.name) {
                    m_selectedIndex = i;
                    m_editBuffer = m_records[static_cast<size_t>(i)];
                    break;
                }
            }
        } else {
            ToastManager::instance().show(ToastType::Error, "Save Failed");
        }
    }

    // Rate categories section (after record editor)
    ImGui::Spacing();
    ImGui::Separator();
    renderRateCategories();
}

void CostingPanel::renderCategorySection(const std::string& category,
                                          const std::string& displayName) {
    if (!ImGui::CollapsingHeader(displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // Collect entries for this category
    const auto& allEntries = m_engine.entries();
    std::vector<size_t> categoryIndices;
    for (size_t i = 0; i < allEntries.size(); ++i) {
        if (allEntries[i].category == category) {
            categoryIndices.push_back(i);
        }
    }

    const auto& style = ImGui::GetStyle();

    // Table with calculated column widths
    float nameColW = ImGui::CalcTextSize("MMMMMMMMMMMMMMMMMMMM").x; // ~20 chars
    float numColW = ImGui::CalcTextSize("00000.00").x + style.FramePadding.x * 2;
    float moneyColW = ImGui::CalcTextSize("$000000.00").x + style.FramePadding.x * 2;
    float actionColW = ImGui::CalcTextSize("X").x + style.FramePadding.x * 4;

    std::string tableId = "CostCat_" + category;
    if (ImGui::BeginTable(tableId.c_str(), 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, nameColW);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, numColW);
        ImGui::TableSetupColumn("Rate", ImGuiTableColumnFlags_WidthFixed, moneyColW);
        ImGui::TableSetupColumn("Estimated", ImGuiTableColumnFlags_WidthFixed, moneyColW);
        ImGui::TableSetupColumn("Actual", ImGuiTableColumnFlags_WidthFixed, moneyColW);
        ImGui::TableSetupColumn("##Del", ImGuiTableColumnFlags_WidthFixed, actionColW);
        ImGui::TableHeadersRow();

        std::string removeId;
        for (size_t idx : categoryIndices) {
            auto* entry = const_cast<CostingEntry*>(&allEntries[idx]);
            ImGui::PushID(static_cast<int>(idx));
            ImGui::TableNextRow();

            // Name
            ImGui::TableNextColumn();
            char nameBuf[256];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", entry->name.c_str());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
                entry->name = nameBuf;
            }

            // Qty
            ImGui::TableNextColumn();
            float qty = static_cast<float>(entry->quantity);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##Qty", &qty, 0, 0, "%.2f")) {
                entry->quantity = static_cast<f64>(qty);
                entry->estimatedTotal = entry->quantity * entry->unitRate;
            }

            // Rate
            ImGui::TableNextColumn();
            bool isMaterialSnapshot = (category == CostCat::Material && entry->snapshot.dbId > 0);
            if (isMaterialSnapshot) {
                // Read-only for snapshotted materials
                ImGui::Text("$%.2f", entry->unitRate);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Snapshot price from database (captured at $%.2f)",
                                      entry->snapshot.priceAtCapture);
                }
            } else {
                float rate = static_cast<float>(entry->unitRate);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputFloat("##Rate", &rate, 0, 0, "%.2f")) {
                    entry->unitRate = static_cast<f64>(rate);
                    entry->estimatedTotal = entry->quantity * entry->unitRate;
                }
            }

            // Estimated (read-only)
            ImGui::TableNextColumn();
            ImGui::Text("$%.2f", entry->estimatedTotal);

            // Actual (editable)
            ImGui::TableNextColumn();
            float actual = static_cast<float>(entry->actualTotal);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##Actual", &actual, 0, 0, "%.2f")) {
                entry->actualTotal = static_cast<f64>(actual);
            }

            // Delete
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("X")) {
                removeId = entry->id;
            }

            ImGui::PopID();
        }

        // Remove entry if requested (after iteration)
        if (!removeId.empty()) {
            m_engine.removeEntry(removeId);
            syncRecordFromEngine();
        }

        ImGui::EndTable();
    }

    // Category subtotal
    f64 catEstimated = 0.0;
    f64 catActual = 0.0;
    for (size_t idx : categoryIndices) {
        catEstimated += allEntries[idx].estimatedTotal;
        catActual += allEntries[idx].actualTotal;
    }
    if (!categoryIndices.empty()) {
        ImGui::Text("Subtotal: Est $%.2f | Act $%.2f", catEstimated, catActual);
    }

    // Add forms for specific categories
    if (category == CostCat::Labor) {
        renderAddLaborForm();
    } else if (category == CostCat::Overhead) {
        renderAddOverheadForm();
    } else if (category == CostCat::Material) {
        renderAddMaterialForm();
    }

    ImGui::Spacing();
}

void CostingPanel::renderAddLaborForm() {
    const auto& style = ImGui::GetStyle();
    float fieldW = ImGui::CalcTextSize("MMMMMMMMMMMM").x + style.FramePadding.x * 2;
    float numW = ImGui::CalcTextSize("0000.00").x + style.FramePadding.x * 2;

    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputText("##LaborName", m_laborName, sizeof(m_laborName));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Labor item name");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(numW);
    ImGui::InputFloat("$/hr##LaborRate", &m_laborRate, 0, 0, "%.2f");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(numW);
    ImGui::InputFloat("hrs##LaborHrs", &m_laborHours, 0, 0, "%.1f");

    ImGui::SameLine();
    if (ImGui::SmallButton("Add Labor")) {
        if (m_laborName[0] != '\0') {
            auto entry = CostingEngine::createLaborEntry(
                m_laborName,
                static_cast<f64>(m_laborRate),
                static_cast<f64>(m_laborHours));
            m_engine.addEntry(std::move(entry));
            syncRecordFromEngine();

            // Reset
            std::memset(m_laborName, 0, sizeof(m_laborName));
            m_laborRate = 25.0f;
            m_laborHours = 1.0f;
        }
    }
}

void CostingPanel::renderAddOverheadForm() {
    const auto& style = ImGui::GetStyle();
    float fieldW = ImGui::CalcTextSize("MMMMMMMMMMMM").x + style.FramePadding.x * 2;
    float numW = ImGui::CalcTextSize("0000.00").x + style.FramePadding.x * 2;

    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputText("##OhName", m_overheadName, sizeof(m_overheadName));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overhead item name");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(numW);
    ImGui::InputFloat("$##OhAmt", &m_overheadAmount, 0, 0, "%.2f");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputText("Notes##OhNotes", m_overheadNotes, sizeof(m_overheadNotes));

    ImGui::SameLine();
    if (ImGui::SmallButton("Add Overhead")) {
        if (m_overheadName[0] != '\0') {
            auto entry = CostingEngine::createOverheadEntry(
                m_overheadName,
                static_cast<f64>(m_overheadAmount),
                m_overheadNotes);
            m_engine.addEntry(std::move(entry));
            syncRecordFromEngine();

            // Reset
            std::memset(m_overheadName, 0, sizeof(m_overheadName));
            m_overheadAmount = 0.0f;
            std::memset(m_overheadNotes, 0, sizeof(m_overheadNotes));
        }
    }
}

void CostingPanel::renderAddMaterialForm() {
    const auto& style = ImGui::GetStyle();
    float fieldW = ImGui::CalcTextSize("MMMMMMMMMMMM").x + style.FramePadding.x * 2;
    float numW = ImGui::CalcTextSize("0000.00").x + style.FramePadding.x * 2;
    float unitW = ImGui::CalcTextSize("board foot").x + style.FramePadding.x * 2;

    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputText("##MatName", m_materialName, sizeof(m_materialName));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Material name");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(numW);
    ImGui::InputFloat("$##MatPrice", &m_materialPrice, 0, 0, "%.2f");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(numW);
    ImGui::InputFloat("Qty##MatQty", &m_materialQty, 0, 0, "%.1f");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(unitW);
    ImGui::InputText("Unit##MatUnit", m_materialUnit, sizeof(m_materialUnit));

    ImGui::SameLine();
    if (ImGui::SmallButton("Add Material")) {
        if (m_materialName[0] != '\0') {
            auto entry = CostingEngine::createMaterialEntry(
                m_materialName,
                0, // No DB reference for manual entry
                "",
                static_cast<f64>(m_materialPrice),
                static_cast<f64>(m_materialQty),
                m_materialUnit);
            m_engine.addEntry(std::move(entry));
            syncRecordFromEngine();

            // Reset
            std::memset(m_materialName, 0, sizeof(m_materialName));
            m_materialPrice = 0.0f;
            m_materialQty = 1.0f;
            std::snprintf(m_materialUnit, sizeof(m_materialUnit), "sheet");
        }
    }
}

void CostingPanel::renderCostingSummary() {
    auto totals = m_engine.categoryTotals();

    // Per-category subtotals
    for (const auto& ct : totals) {
        ImGui::Text("  %s: Est $%.2f | Act $%.2f (%d items)",
                     categoryDisplayName(ct.category),
                     ct.estimatedTotal, ct.actualTotal, ct.entryCount);
    }

    ImGui::Spacing();
    ImGui::Separator();

    f64 estTotal = m_engine.totalEstimated();
    f64 actTotal = m_engine.totalActual();
    f64 var = m_engine.variance();

    ImGui::Text("Estimated Total: $%.2f", estTotal);
    ImGui::Text("Actual Total:    $%.2f", actTotal);

    // Variance with color coding
    if (var <= 0.0) {
        // Under budget or on budget -- green
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.3f, 1.0f),
                            "Variance:        $%.2f (under budget)", var);
    } else {
        // Over budget -- red
        ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f),
                            "Variance:       +$%.2f (over budget)", var);
    }
}

void CostingPanel::syncEngineFromRecord() {
    std::vector<CostingEntry> entries;
    for (const auto& item : m_editBuffer.items) {
        CostingEntry ce;
        ce.name = item.name;
        ce.id = (item.id > 0) ? std::to_string(item.id) : "";
        ce.quantity = item.quantity;
        ce.unitRate = item.rate;
        ce.estimatedTotal = item.total;

        // Map old CostCategory enum to new string category
        switch (item.category) {
        case CostCategory::Material:
            ce.category = CostCat::Material;
            break;
        case CostCategory::Labor:
            ce.category = CostCat::Labor;
            break;
        case CostCategory::Tool:
            ce.category = CostCat::Tooling;
            break;
        case CostCategory::Other:
            ce.category = CostCat::Overhead;
            break;
        }

        entries.push_back(std::move(ce));
    }
    m_engine.setEntries(std::move(entries));
}

void CostingPanel::syncRecordFromEngine() {
    std::vector<CostItem> items;
    for (const auto& ce : m_engine.entries()) {
        CostItem item;
        item.name = ce.name;
        item.quantity = ce.quantity;
        item.rate = ce.unitRate;
        item.total = ce.estimatedTotal;
        item.notes = ce.notes;

        // Map string category back to enum
        if (ce.category == CostCat::Material) {
            item.category = CostCategory::Material;
        } else if (ce.category == CostCat::Labor) {
            item.category = CostCategory::Labor;
        } else if (ce.category == CostCat::Tooling) {
            item.category = CostCategory::Tool;
        } else {
            item.category = CostCategory::Other;
        }

        items.push_back(std::move(item));
    }
    m_editBuffer.items = std::move(items);
}

void CostingPanel::renderRateCategories() {
    if (!m_rateCatRepo) {
        return;
    }

    if (!ImGui::CollapsingHeader("Rate Categories", m_showRateCategories ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
        return;
    }

    // Determine project context
    i64 activeProjectId = 0;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_records.size())) {
        activeProjectId = m_editBuffer.projectId;
    }

    // Project volume input
    ImGui::Text("Project Volume (cubic units):");
    ImGui::SameLine();
    float volWidth = ImGui::CalcTextSize("00000000.00").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetNextItemWidth(volWidth);
    double volInput = m_projectVolumeCuUnits;
    if (ImGui::InputDouble("##ProjVol", &volInput, 0, 0, "%.2f")) {
        m_projectVolumeCuUnits = volInput;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Bounding volume of the project in cubic units.\nRate x Volume = Computed Cost");
    }

    ImGui::Spacing();

    // Rate categories table
    if (ImGui::BeginTable("RateCats",
                          5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Rate/cu unit", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("$00000.00__").x);
        ImGui::TableSetupColumn("Computed Cost", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("$00000.00__").x);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("Override__").x);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("Edit Delete__").x);
        ImGui::TableHeadersRow();

        f64 rateCategoryTotal = 0.0;
        int deleteRateIdx = -1;

        for (int i = 0; i < static_cast<int>(m_effectiveRates.size()); ++i) {
            auto& rate = m_effectiveRates[static_cast<size_t>(i)];
            f64 computedCost = rate.computeCost(m_projectVolumeCuUnits);
            rateCategoryTotal += computedCost;

            ImGui::PushID(1000 + i); // Offset to avoid ID collision with line items
            ImGui::TableNextRow();

            if (m_editingRateIndex == i) {
                // Inline edit mode
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##EditRName", m_editRateName, sizeof(m_editRateName));

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                ImGui::InputFloat("##EditRRate", &m_editRatePerCuUnit, 0, 0, "%.2f");

                ImGui::TableNextColumn();
                ImGui::Text("$%.2f", static_cast<double>(m_editRatePerCuUnit) * m_projectVolumeCuUnits);

                ImGui::TableNextColumn();
                ImGui::TextDisabled("--");

                ImGui::TableNextColumn();
                if (ImGui::SmallButton("OK")) {
                    rate.name = m_editRateName;
                    rate.ratePerCuUnit = static_cast<f64>(m_editRatePerCuUnit);
                    rate.notes = m_editRateNotes;
                    m_rateCatRepo->update(rate);
                    m_editingRateIndex = -1;
                    refreshRateCategories();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Cancel")) {
                    m_editingRateIndex = -1;
                }
            } else {
                // Display mode
                ImGui::TableNextColumn();
                ImGui::Text("%s", rate.name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("$%.2f", rate.ratePerCuUnit);

                ImGui::TableNextColumn();
                ImGui::Text("$%.2f", computedCost);

                ImGui::TableNextColumn();
                bool isProjectOverride = (rate.projectId > 0);
                if (isProjectOverride) {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Override");
                } else {
                    ImGui::TextDisabled("Global");
                }

                ImGui::TableNextColumn();
                if (ImGui::SmallButton("Edit")) {
                    m_editingRateIndex = i;
                    std::memset(m_editRateName, 0, sizeof(m_editRateName));
                    std::snprintf(m_editRateName, sizeof(m_editRateName), "%s", rate.name.c_str());
                    m_editRatePerCuUnit = static_cast<float>(rate.ratePerCuUnit);
                    std::memset(m_editRateNotes, 0, sizeof(m_editRateNotes));
                    std::snprintf(m_editRateNotes, sizeof(m_editRateNotes), "%s", rate.notes.c_str());
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Del")) {
                    deleteRateIdx = i;
                }
            }

            ImGui::PopID();
        }

        // Delete rate if requested
        if (deleteRateIdx >= 0 && deleteRateIdx < static_cast<int>(m_effectiveRates.size())) {
            m_rateCatRepo->remove(m_effectiveRates[static_cast<size_t>(deleteRateIdx)].id);
            m_editingRateIndex = -1;
            refreshRateCategories();
        }

        // Add new rate row
        ImGui::PushID(2000);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##NewRName", m_newRateName, sizeof(m_newRateName));

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputFloat("##NewRRate", &m_newRatePerCuUnit, 0, 0, "%.2f");

        ImGui::TableNextColumn();
        ImGui::Text("$%.2f", static_cast<double>(m_newRatePerCuUnit) * m_projectVolumeCuUnits);

        ImGui::TableNextColumn();
        // Override checkbox for project context
        if (activeProjectId > 0) {
            ImGui::Checkbox("##Override", &m_newRateIsOverride);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Insert as project-specific override");
            }
        } else {
            ImGui::TextDisabled("Global");
        }

        ImGui::TableNextColumn();
        if (ImGui::SmallButton("Add")) {
            if (m_newRateName[0] != '\0') {
                RateCategory newCat;
                newCat.name = m_newRateName;
                newCat.ratePerCuUnit = static_cast<f64>(m_newRatePerCuUnit);
                newCat.notes = m_newRateNotes;
                newCat.projectId = (activeProjectId > 0 && m_newRateIsOverride) ? activeProjectId : 0;

                m_rateCatRepo->insert(newCat);

                // Reset input fields
                std::memset(m_newRateName, 0, sizeof(m_newRateName));
                m_newRatePerCuUnit = 0.0f;
                std::memset(m_newRateNotes, 0, sizeof(m_newRateNotes));
                m_newRateIsOverride = false;

                refreshRateCategories();
            }
        }

        ImGui::PopID();

        ImGui::EndTable();

        // Total from rate categories
        ImGui::Spacing();
        ImGui::Text("Rate Category Total: $%.2f", rateCategoryTotal);
    }
}

void CostingPanel::refreshRateCategories() {
    if (!m_rateCatRepo) {
        return;
    }

    i64 activeProjectId = 0;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_records.size())) {
        activeProjectId = m_editBuffer.projectId;
    }

    if (activeProjectId > 0) {
        m_effectiveRates = m_rateCatRepo->getEffectiveRates(activeProjectId);
    } else {
        m_effectiveRates = m_rateCatRepo->findGlobal();
    }
}

void CostingPanel::selectRecord(i64 recordId) {
    // Refresh list to ensure we have latest data
    if (m_repo) {
        m_records = m_repo->findAll();
    }

    // Find the record by ID
    for (int i = 0; i < static_cast<int>(m_records.size()); ++i) {
        if (m_records[static_cast<size_t>(i)].id == recordId) {
            m_selectedIndex = i;
            m_isNewRecord = false;
            m_editBuffer = m_records[static_cast<size_t>(i)];
            std::memset(m_editName, 0, sizeof(m_editName));
            std::snprintf(
                m_editName, sizeof(m_editName), "%s", m_editBuffer.name.c_str());
            std::memset(m_editNotes, 0, sizeof(m_editNotes));
            std::snprintf(
                m_editNotes, sizeof(m_editNotes), "%s", m_editBuffer.notes.c_str());
            m_editTaxRate = static_cast<float>(m_editBuffer.taxRate);
            m_editDiscountRate = static_cast<float>(m_editBuffer.discountRate);
            syncEngineFromRecord();
            refreshRateCategories();
            return;
        }
    }
}

void CostingPanel::recalculateEditBuffer() {
    m_editBuffer.subtotal = 0.0;
    for (auto& item : m_editBuffer.items) {
        item.total = item.quantity * item.rate;
        m_editBuffer.subtotal += item.total;
    }
    m_editBuffer.taxAmount = m_editBuffer.subtotal * m_editBuffer.taxRate / 100.0;
    m_editBuffer.discountAmount = m_editBuffer.subtotal * m_editBuffer.discountRate / 100.0;
    m_editBuffer.total = m_editBuffer.subtotal + m_editBuffer.taxAmount -
                         m_editBuffer.discountAmount;
}

} // namespace dw
