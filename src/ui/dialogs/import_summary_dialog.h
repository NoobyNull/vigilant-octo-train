#pragma once

// Digital Workshop - Import Summary Dialog
// Modal dialog displaying batch import results with duplicate and error details.

#include "core/import/import_task.h"
#include "ui/dialogs/dialog.h"

namespace dw {

class ImportSummaryDialog : public Dialog {
  public:
    ImportSummaryDialog();
    ~ImportSummaryDialog() override = default;

    // Open the dialog with import summary data
    void open(const ImportBatchSummary& summary);

    // Render the dialog (called each frame by UIManager)
    void render() override;

  private:
    ImportBatchSummary m_summary;
};

} // namespace dw
