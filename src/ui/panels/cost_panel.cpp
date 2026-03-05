#include "ui/panels/cost_panel.h"

#include <cstdio>
#include <cstring>

#include "ui/icons.h"
#include "ui/widgets/toast.h"

namespace dw {

static const char* kCategoryNames[] = {"Material", "Labor", "Tool", "Other"};
static constexpr int kCategoryCount = 4;

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
        if (m_selectedIndex >= 0) {
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

    // Line items table
    ImGui::Text("Line Items:");
    if (ImGui::BeginTable("CostItems",
                          6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Material__").x);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("0000.00").x);
        ImGui::TableSetupColumn("Rate", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("$00000.00").x);
        ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("$00000.00").x);
        ImGui::TableSetupColumn("##Del", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("X__").x);
        ImGui::TableHeadersRow();

        int deleteIdx = -1;
        for (int i = 0; i < static_cast<int>(m_editBuffer.items.size()); ++i) {
            auto& item = m_editBuffer.items[static_cast<size_t>(i)];
            ImGui::PushID(i);
            ImGui::TableNextRow();

            // Name
            ImGui::TableNextColumn();
            char itemName[256];
            std::snprintf(itemName, sizeof(itemName), "%s", item.name.c_str());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##ItemName", itemName, sizeof(itemName))) {
                item.name = itemName;
            }

            // Category combo
            ImGui::TableNextColumn();
            int catIdx = static_cast<int>(item.category);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##Cat", &catIdx, kCategoryNames, kCategoryCount)) {
                item.category = static_cast<CostCategory>(catIdx);
            }

            // Quantity
            ImGui::TableNextColumn();
            float qty = static_cast<float>(item.quantity);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##Qty", &qty, 0, 0, "%.2f")) {
                item.quantity = static_cast<f64>(qty);
                item.total = item.quantity * item.rate;
            }

            // Rate
            ImGui::TableNextColumn();
            float rate = static_cast<float>(item.rate);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputFloat("##Rate", &rate, 0, 0, "%.2f")) {
                item.rate = static_cast<f64>(rate);
                item.total = item.quantity * item.rate;
            }

            // Total (read-only)
            ImGui::TableNextColumn();
            ImGui::Text("$%.2f", item.total);

            // Delete button
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("X")) {
                deleteIdx = i;
            }

            ImGui::PopID();
        }

        // Remove item if delete was clicked
        if (deleteIdx >= 0) {
            m_editBuffer.items.erase(m_editBuffer.items.begin() + deleteIdx);
        }

        // Add item row
        renderNewItemRow();

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Tax and discount
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("100.00%").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputFloat("Tax %", &m_editTaxRate, 0, 0, "%.2f");
    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("100.00%").x + ImGui::GetStyle().FramePadding.x * 2);
    ImGui::InputFloat("Discount %", &m_editDiscountRate, 0, 0, "%.2f");

    // Calculate totals
    m_editBuffer.taxRate = static_cast<f64>(m_editTaxRate);
    m_editBuffer.discountRate = static_cast<f64>(m_editDiscountRate);
    recalculateEditBuffer();

    ImGui::Spacing();
    ImGui::Separator();

    // Totals display
    ImGui::Text("Subtotal:  $%.2f", m_editBuffer.subtotal);
    if (m_editBuffer.taxAmount > 0.001) {
        ImGui::Text("Tax:       $%.2f", m_editBuffer.taxAmount);
    }
    if (m_editBuffer.discountAmount > 0.001) {
        ImGui::Text("Discount: -$%.2f", m_editBuffer.discountAmount);
    }
    ImGui::Text("Total:     $%.2f", m_editBuffer.total);

    ImGui::Spacing();

    // Notes
    ImGui::Text("Notes:");
    ImGui::InputTextMultiline("##RecNotes", m_editNotes, sizeof(m_editNotes), ImVec2(-1, 60));

    ImGui::Spacing();

    // Save button
    std::string saveLabel = std::string(Icons::Save) + " Save";
    float saveBtnW = ImGui::CalcTextSize(saveLabel.c_str()).x + ImGui::GetStyle().FramePadding.x * 4;
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

void CostingPanel::renderNewItemRow() {
    ImGui::TableNextRow();

    // Name
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##NewName", m_newItemName, sizeof(m_newItemName));

    // Category
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##NewCat", &m_newItemCategory, kCategoryNames, kCategoryCount);

    // Qty
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##NewQty", &m_newItemQty, 0, 0, "%.2f");

    // Rate
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputFloat("##NewRate", &m_newItemRate, 0, 0, "%.2f");

    // Total preview
    ImGui::TableNextColumn();
    ImGui::Text("$%.2f", static_cast<double>(m_newItemQty) * static_cast<double>(m_newItemRate));

    // Add button
    ImGui::TableNextColumn();
    if (ImGui::SmallButton("+")) {
        if (m_newItemName[0] != '\0') {
            CostItem item;
            item.name = m_newItemName;
            item.category = static_cast<CostCategory>(m_newItemCategory);
            item.quantity = static_cast<f64>(m_newItemQty);
            item.rate = static_cast<f64>(m_newItemRate);
            item.total = item.quantity * item.rate;
            m_editBuffer.items.push_back(item);

            // Reset new item fields
            std::memset(m_newItemName, 0, sizeof(m_newItemName));
            m_newItemCategory = 0;
            m_newItemQty = 1.0f;
            m_newItemRate = 0.0f;
        }
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
