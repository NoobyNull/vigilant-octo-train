#pragma once

#include <string>
#include <vector>

namespace dw {

class CncController;

struct PreflightIssue {
    enum Severity { Error, Warning };
    Severity severity;
    std::string message;
};

// Run pre-flight checks before streaming. Returns empty vector if all clear.
// Errors block streaming; warnings are informational only.
std::vector<PreflightIssue> runPreflightChecks(
    const CncController& ctrl,
    bool hasToolSelected,
    bool hasMaterialSelected
);

} // namespace dw
