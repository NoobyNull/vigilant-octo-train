#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "../../core/database/cost_repository.h"
#include "panel.h"

namespace dw {

// Cost estimation panel for creating and managing project cost estimates
class CostPanel : public Panel {
  public:
    explicit CostPanel(CostRepository* repo);
    ~CostPanel() override = default;

    void render() override;

  private:
    void renderToolbar();
    void renderEstimateList();
    void renderEstimateEditor();
    void renderNewItemRow();

    // Recalculate edit buffer totals from items
    void recalculateEditBuffer();

    CostRepository* m_repo;
    std::vector<CostEstimate> m_estimates;
    int m_selectedIndex = -1;

    // Edit buffer for selected estimate
    CostEstimate m_editBuffer;
    char m_editName[256]{};
    char m_editNotes[1024]{};

    // New item input buffer
    char m_newItemName[256]{};
    int m_newItemCategory = 0;
    float m_newItemQty = 1.0f;
    float m_newItemRate = 0.0f;
    char m_newItemNotes[256]{};

    // Tax/discount edit fields
    float m_editTaxRate = 0.0f;
    float m_editDiscountRate = 0.0f;

    bool m_isNewEstimate = false;
};

} // namespace dw
