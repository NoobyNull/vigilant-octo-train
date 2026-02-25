#include "preflight_check.h"
#include "cnc_controller.h"

namespace dw {

std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected)
{
    std::vector<PreflightIssue> issues;

    // Error checks (block streaming)
    if (!ctrl.isConnected()) {
        issues.push_back({PreflightIssue::Error, "Not connected to CNC controller"});
    }

    if (ctrl.lastStatus().state == MachineState::Alarm) {
        issues.push_back({PreflightIssue::Error,
                          "Machine is in ALARM state -- clear alarm first ($X or power cycle)"});
    }

    if (ctrl.isInErrorState()) {
        issues.push_back({PreflightIssue::Error,
                          "Previous streaming error not acknowledged"});
    }

    if (ctrl.isStreaming()) {
        issues.push_back({PreflightIssue::Error, "A job is already running"});
    }

    // Warning checks (informational, don't block streaming)
    if (!hasToolSelected) {
        issues.push_back({PreflightIssue::Warning,
                          "No tool selected -- feed rate recommendations unavailable"});
    }

    if (!hasMaterialSelected) {
        issues.push_back({PreflightIssue::Warning,
                          "No material selected -- cutting parameters not validated"});
    }

    return issues;
}

} // namespace dw
