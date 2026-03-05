#include "ui/panels/cost_panel.h"

#include <cstdio>
#include <cstring>

#include "ui/icons.h"
#include "ui/widgets/toast.h"

namespace dw {

static const char* kCategoryNames[] = {"Material", "Labor", "Tool", "Other"};
static constexpr int kCategoryCount = 4;

CostingPanel::CostingPanel(CostRepository* repo) : Panel("Project Costing"), m_repo(repo) {
    if (m_repo) {
        m_records = m_repo->findAll();
    }
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
