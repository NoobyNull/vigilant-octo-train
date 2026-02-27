#include "preflight_check.h"

#include <cmath>
#include <string>

#include "cnc_controller.h"
#include "../config/config.h"
#include "../gcode/machine_profile.h"

namespace dw {

std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected,
    const Vec3* boundsMin,
    const Vec3* boundsMax,
    const gcode::MachineProfile* profile)
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

    // Soft limit pre-check: compare G-code bounds against machine travel
    if (boundsMin && boundsMax && profile &&
        Config::instance().getSafetySoftLimitCheckEnabled()) {
        float jobSizeX = boundsMax->x - boundsMin->x;
        float jobSizeY = boundsMax->y - boundsMin->y;

        bool exceeds = false;
        std::string msg = "Job may exceed machine travel: ";
        if (jobSizeX > profile->maxTravelX) {
            msg += "X(" + std::to_string(static_cast<int>(jobSizeX)) + ">" +
                   std::to_string(static_cast<int>(profile->maxTravelX)) + "mm) ";
            exceeds = true;
        }
        if (jobSizeY > profile->maxTravelY) {
            msg += "Y(" + std::to_string(static_cast<int>(jobSizeY)) + ">" +
                   std::to_string(static_cast<int>(profile->maxTravelY)) + "mm) ";
            exceeds = true;
        }
        if (std::abs(boundsMin->z) > profile->maxTravelZ ||
            std::abs(boundsMax->z) > profile->maxTravelZ) {
            msg += "Z(depth exceeds " +
                   std::to_string(static_cast<int>(profile->maxTravelZ)) + "mm) ";
            exceeds = true;
        }
        if (exceeds) {
            issues.push_back({PreflightIssue::Warning, msg});
        }
    }

    return issues;
}

} // namespace dw
