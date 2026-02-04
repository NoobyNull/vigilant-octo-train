// Digital Workshop - Workspace Implementation

#include "app/workspace.h"

namespace dw {

void Workspace::setFocusedMesh(std::shared_ptr<Mesh> mesh) {
    m_focusedMesh = std::move(mesh);
}

void Workspace::clearFocusedMesh() {
    m_focusedMesh.reset();
}

void Workspace::setFocusedGCode(std::shared_ptr<GCodeFile> gcode) {
    m_focusedGCode = std::move(gcode);
}

void Workspace::clearFocusedGCode() {
    m_focusedGCode.reset();
}

void Workspace::setFocusedCutPlan(std::shared_ptr<CutPlan> plan) {
    m_focusedCutPlan = std::move(plan);
}

void Workspace::clearFocusedCutPlan() {
    m_focusedCutPlan.reset();
}

void Workspace::clearAll() {
    m_focusedMesh.reset();
    m_focusedGCode.reset();
    m_focusedCutPlan.reset();
}

} // namespace dw
