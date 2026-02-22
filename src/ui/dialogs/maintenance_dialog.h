#pragma once

#include <functional>

#include "core/library/library_manager.h"
#include "ui/dialogs/dialog.h"

namespace dw {

class MaintenanceDialog : public Dialog {
  public:
    MaintenanceDialog();
    ~MaintenanceDialog() override = default;

    void render() override;

    using RunCallback = std::function<MaintenanceReport()>;
    void setOnRun(RunCallback cb) { m_onRun = std::move(cb); }

  private:
    enum class State { Confirm, Running, Done };
    State m_state = State::Confirm;
    MaintenanceReport m_report;
    RunCallback m_onRun;
};

} // namespace dw
