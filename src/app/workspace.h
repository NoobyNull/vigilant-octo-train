#pragma once

// Digital Workshop - Workspace
// Central state manager for the application's "focused object" design.
// Panels observe and act on the currently focused model, g-code, or cut plan.

#include <memory>

namespace dw {

// Forward declarations - these will be implemented in core modules
class Mesh;
class GCodeFile;
class CutPlan;

class Workspace {
public:
    Workspace() = default;
    ~Workspace() = default;

    // Focused model (3D mesh)
    void setFocusedMesh(std::shared_ptr<Mesh> mesh);
    auto getFocusedMesh() const -> std::shared_ptr<Mesh> { return m_focusedMesh; }
    auto hasFocusedMesh() const -> bool { return m_focusedMesh != nullptr; }
    void clearFocusedMesh();

    // Focused G-code file
    void setFocusedGCode(std::shared_ptr<GCodeFile> gcode);
    auto getFocusedGCode() const -> std::shared_ptr<GCodeFile> { return m_focusedGCode; }
    auto hasFocusedGCode() const -> bool { return m_focusedGCode != nullptr; }
    void clearFocusedGCode();

    // Focused cut plan (2D optimizer result)
    void setFocusedCutPlan(std::shared_ptr<CutPlan> plan);
    auto getFocusedCutPlan() const -> std::shared_ptr<CutPlan> { return m_focusedCutPlan; }
    auto hasFocusedCutPlan() const -> bool { return m_focusedCutPlan != nullptr; }
    void clearFocusedCutPlan();

    // Clear all focused objects
    void clearAll();

private:
    std::shared_ptr<Mesh> m_focusedMesh;
    std::shared_ptr<GCodeFile> m_focusedGCode;
    std::shared_ptr<CutPlan> m_focusedCutPlan;
};

} // namespace dw
