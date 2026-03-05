#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "../../core/database/cost_repository.h"
#include "panel.h"

namespace dw {

// Project costing panel for creating and managing cost records
class CostingPanel : public Panel {
  public:
    explicit CostingPanel(CostRepository* repo);
    ~CostingPanel() override = default;

    void render() override;

    // Navigate to and select a specific record by ID
    void selectRecord(i64 recordId);

  private:
    void renderToolbar();
    void renderRecordList();
    void renderRecordEditor();
    void renderNewItemRow();

    // Recalculate edit buffer totals from items
    void recalculateEditBuffer();

    CostRepository* m_repo;
    std::vector<CostingRecord> m_records;
    int m_selectedIndex = -1;

    // Edit buffer for selected record
    CostingRecord m_editBuffer;
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

    bool m_isNewRecord = false;
};

} // namespace dw
