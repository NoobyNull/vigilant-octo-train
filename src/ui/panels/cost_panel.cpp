#include "ui/panels/cost_panel.h"

#include <cstdio>
#include <cstring>

#include "ui/icons.h"
#include "ui/widgets/toast.h"

namespace dw {

static const char* kCategoryNames[] = {"Material", "Labor", "Tool", "Other"};
static constexpr int kCategoryCount = 4;

CostPanel::CostPanel(CostRepository* repo) : Panel("Cost Estimator"), m_repo(repo) {
    if (m_repo) {
        m_estimates = m_repo->findAll();
    }
}

void CostPanel::render() {
    if (!m_open) {
        return;
    }

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    renderToolbar();
    ImGui::Separator();

    // Two-column layout: list (left) | editor (right)
    float listWidth = 220.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("EstimateList", ImVec2(listWidth, avail.y), true)) {
        renderEstimateList();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("EstimateEditor", ImVec2(0, avail.y), true)) {
        if (m_selectedIndex >= 0) {
            renderEstimateEditor();
        } else {
            ImGui::TextDisabled("Select or create an estimate");
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void CostPanel::renderToolbar() {
    if (ImGui::Button(Icons::Add)) {
        // Create new estimate
        m_editBuffer = CostEstimate{};
        m_editBuffer.name = "New Estimate";
        std::memset(m_editName, 0, sizeof(m_editName));
        std::snprintf(m_editName, sizeof(m_editName), "%s", m_editBuffer.name.c_str());
        std::memset(m_editNotes, 0, sizeof(m_editNotes));
        m_editTaxRate = 0.0f;
        m_editDiscountRate = 0.0f;
        m_isNewEstimate = true;
        m_selectedIndex = -1; // Will be set after insert
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("New Estimate");
    }

    ImGui::SameLine();

    bool hasSelection =
        m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_estimates.size());
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button(Icons::Delete)) {
        if (hasSelection) {
            i64 id = m_estimates[static_cast<size_t>(m_selectedIndex)].id;
            if (m_repo->remove(id)) {
                ToastManager::instance().show(ToastType::Success, "Estimate Deleted");
                m_estimates = m_repo->findAll();
                m_selectedIndex = -1;
                m_isNewEstimate = false;
            } else {
                ToastManager::instance().show(ToastType::Error, "Delete Failed");
            }
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Delete Estimate");
    }

    ImGui::SameLine();

    if (ImGui::Button(Icons::Refresh)) {
        if (m_repo) {
            m_estimates = m_repo->findAll();
            m_selectedIndex = -1;
            m_isNewEstimate = false;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh");
    }
}

void CostPanel::renderEstimateList() {
    for (int i = 0; i < static_cast<int>(m_estimates.size()); ++i) {
        const auto& est = m_estimates[static_cast<size_t>(i)];
        char label[128];
        std::snprintf(label, sizeof(label), "%s\n$%.2f", est.name.c_str(), est.total);

        bool selected = (i == m_selectedIndex && !m_isNewEstimate);
        if (ImGui::Selectable(label, selected, 0, ImVec2(0, ImGui::GetTextLineHeight() * 2.5f))) {
            m_selectedIndex = i;
            m_isNewEstimate = false;
            m_editBuffer = est;
            std::memset(m_editName, 0, sizeof(m_editName));
            std::snprintf(m_editName, sizeof(m_editName), "%s", est.name.c_str());
            std::memset(m_editNotes, 0, sizeof(m_editNotes));
            std::snprintf(m_editNotes, sizeof(m_editNotes), "%s", est.notes.c_str());
            m_editTaxRate = static_cast<float>(est.taxRate);
            m_editDiscountRate = static_cast<float>(est.discountRate);
        }
    }

    // Show new estimate entry if creating
    if (m_isNewEstimate) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        ImGui::Selectable("(New Estimate)", true);
        ImGui::PopStyleColor();
    }
}

void CostPanel::renderEstimateEditor() {
    // Name input
    ImGui::Text("Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##EstName", m_editName, sizeof(m_editName));

    ImGui::Spacing();

    // Line items table
    ImGui::Text("Line Items:");
    if (ImGui::BeginTable("CostItems", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Rate", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("##Del", ImGuiTableColumnFlags_WidthFixed, 30.0f);
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
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputFloat("Tax %", &m_editTaxRate, 0, 0, "%.2f");
    ImGui::SameLine(0, 20.0f);
    ImGui::SetNextItemWidth(100.0f);
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
    ImGui::InputTextMultiline("##EstNotes", m_editNotes, sizeof(m_editNotes), ImVec2(-1, 60));

    ImGui::Spacing();

    // Save button
    std::string saveLabel = std::string(Icons::Save) + " Save";
    if (ImGui::Button(saveLabel.c_str(), ImVec2(100, 0))) {
        m_editBuffer.name = m_editName;
        m_editBuffer.notes = m_editNotes;

        bool ok = false;
        if (m_isNewEstimate) {
            auto newId = m_repo->insert(m_editBuffer);
            ok = newId.has_value();
            if (ok) {
                m_isNewEstimate = false;
            }
        } else {
            ok = m_repo->update(m_editBuffer);
        }

        if (ok) {
            ToastManager::instance().show(ToastType::Success, "Estimate Saved");
            m_estimates = m_repo->findAll();
            // Re-select by ID
            for (int i = 0; i < static_cast<int>(m_estimates.size()); ++i) {
                if (m_estimates[static_cast<size_t>(i)].name == m_editBuffer.name) {
                    m_selectedIndex = i;
                    m_editBuffer = m_estimates[static_cast<size_t>(i)];
                    break;
                }
            }
        } else {
            ToastManager::instance().show(ToastType::Error, "Save Failed");
        }
    }
}

void CostPanel::renderNewItemRow() {
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

void CostPanel::recalculateEditBuffer() {
    m_editBuffer.subtotal = 0.0;
    for (auto& item : m_editBuffer.items) {
        item.total = item.quantity * item.rate;
        m_editBuffer.subtotal += item.total;
    }
    m_editBuffer.taxAmount = m_editBuffer.subtotal * m_editBuffer.taxRate / 100.0;
    m_editBuffer.discountAmount = m_editBuffer.subtotal * m_editBuffer.discountRate / 100.0;
    m_editBuffer.total =
        m_editBuffer.subtotal + m_editBuffer.taxAmount - m_editBuffer.discountAmount;
}

} // namespace dw
