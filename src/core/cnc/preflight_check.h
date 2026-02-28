#pragma once

#include <string>
#include <vector>

#include "../types.h"

namespace dw {

class CncController;

namespace gcode {
struct MachineProfile;
}

struct PreflightIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string message;
};

// Run pre-flight checks before streaming. Returns empty vector if all clear.
// Errors block streaming; warnings are informational only.
// Optional params for soft limit check: boundsMin/Max from G-code analysis,
// profile with maxTravel fields. Pass nullptr to skip soft limit check.
std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected,
    const Vec3* boundsMin = nullptr,
    const Vec3* boundsMax = nullptr,
    const gcode::MachineProfile* profile = nullptr
);

} // namespace dw
