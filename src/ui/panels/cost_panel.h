#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "../../core/database/cost_repository.h"
#include "panel.h"

namespace dw {

class CostRepository;

// Cost estimation panel
class CostPanel : public Panel {
  public:
    explicit CostPanel(CostRepository& repo);
    ~CostPanel() override = default;

    void render() override;

    // Load/save
    void newEstimate();
    void loadEstimate(i64 id);
    void saveEstimate();

    // Callbacks
    using EstimateSavedCallback = std::function<void(i64)>;
    void setEstimateSavedCallback(EstimateSavedCallback cb) { m_onSaved = std::move(cb); }

  private:
    void renderToolbar();
    void renderEstimateInfo();
    void renderItemsTable();
    void renderTotals();
    void renderEstimateList();

    void addItem();
    void removeItem(size_t index);
    void recalculate();

    CostRepository& m_repo;
    CostEstimate m_estimate;
    bool m_modified = false;

    // Editor state
    char m_newItemName[128] = "";
    int m_newItemCategory = 0;
    float m_newItemQty = 1.0f;
    float m_newItemRate = 0.0f;

    // Estimate name editor
    char m_estimateName[128] = "";

    // List view
    std::vector<CostEstimate> m_allEstimates;
    bool m_showList = true;

    EstimateSavedCallback m_onSaved;
};

} // namespace dw
