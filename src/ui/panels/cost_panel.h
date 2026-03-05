#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "../../core/database/cost_repository.h"
#include "../../core/database/rate_category_repository.h"
#include "panel.h"

namespace dw {

// Project costing panel for creating and managing cost records
class CostingPanel : public Panel {
  public:
    explicit CostingPanel(CostRepository* repo, RateCategoryRepository* rateCatRepo = nullptr);
    ~CostingPanel() override = default;

    void render() override;

    // Navigate to and select a specific record by ID
    void selectRecord(i64 recordId);

  private:
    void renderToolbar();
    void renderRecordList();
    void renderRecordEditor();
    void renderNewItemRow();
    void renderRateCategories();

    // Recalculate edit buffer totals from items
    void recalculateEditBuffer();

    // Reload effective rates from DB based on current context
    void refreshRateCategories();

    CostRepository* m_repo;
    RateCategoryRepository* m_rateCatRepo;
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

    // Rate category state
    std::vector<RateCategory> m_effectiveRates;
    f64 m_projectVolumeCuUnits = 0.0;
    char m_newRateName[256]{};
    float m_newRatePerCuUnit = 0.0f;
    char m_newRateNotes[256]{};
    bool m_showRateCategories = true;
    int m_editingRateIndex = -1;      // Which rate row is being edited (-1 = none)
    char m_editRateName[256]{};
    float m_editRatePerCuUnit = 0.0f;
    char m_editRateNotes[256]{};
    bool m_newRateIsOverride = false;  // For project context: insert as project override?
};

} // namespace dw
