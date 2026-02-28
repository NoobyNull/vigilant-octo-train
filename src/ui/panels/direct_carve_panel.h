#pragma once

#include <string>
#include <vector>

#include "panel.h"

#include "core/carve/model_fitter.h"
#include "core/carve/toolpath_types.h"
#include "core/cnc/cnc_tool.h"
#include "core/cnc/cnc_types.h"
#include "core/mesh/vertex.h"
#include "core/types.h"

namespace dw {

class CncController;
class FileDialog;
class GCodePanel;
class ToolDatabase;

namespace carve {
class CarveJob;
}

// Direct Carve wizard panel -- step-by-step guided workflow
// for streaming 2.5D toolpaths directly from STL models.
class DirectCarvePanel : public Panel {
  public:
    DirectCarvePanel();
    ~DirectCarvePanel() override;

    void render() override;

    // Dependencies
    void setCncController(CncController* cnc);
    void setToolDatabase(ToolDatabase* db);
    void setCarveJob(carve::CarveJob* job);
    void setFileDialog(FileDialog* dlg);
    void setGCodePanel(GCodePanel* gcp);

    // Callbacks
    void onConnectionChanged(bool connected);
    void onStatusUpdate(const MachineStatus& status);
    void onModelLoaded(const std::vector<Vertex>& vertices,
                       const std::vector<u32>& indices,
                       const Vec3& boundsMin,
                       const Vec3& boundsMax);

  private:
    enum class Step {
        MachineCheck,  // Verify connection, homing, profile
        ModelFit,      // Scale, position, depth adjustment
        ToolSelect,    // Review/accept tool recommendations
        MaterialSetup, // Material selection, feeds confirmation
        Preview,       // Toolpath preview with time estimate
        OutlineTest,   // Run perimeter trace at safe Z
        ZeroConfirm,   // Verify zero position
        Commit,        // Final confirmation
        Running        // Job in progress
    };

    static constexpr int STEP_COUNT = 9;

    // Step rendering (one method per step)
    void renderMachineCheck();
    void renderModelFit();
    void renderToolSelect();
    void renderMaterialSetup();
    void renderPreview();
    void renderOutlineTest();
    void renderZeroConfirm();
    void renderCommit();
    void renderRunning();

    // Navigation
    void renderStepIndicator();
    void renderNavButtons();
    bool canAdvance() const;
    void advanceStep();
    void retreatStep();

    // Validation
    bool validateMachineReady() const;

    // Step label helper
    static const char* stepLabel(Step step);

    // State
    Step m_currentStep = Step::MachineCheck;
    CncController* m_cnc = nullptr;
    ToolDatabase* m_toolDb = nullptr;
    carve::CarveJob* m_carveJob = nullptr;
    FileDialog* m_fileDialog = nullptr;
    GCodePanel* m_gcodePanel = nullptr;
    MachineStatus m_machineStatus;
    bool m_cncConnected = false;

    // Machine check state
    bool m_safeZConfirmed = false;

    // Model data (set via onModelLoaded)
    std::vector<Vertex> m_vertices;
    std::vector<u32> m_indices;
    bool m_modelLoaded = false;
    Vec3 m_modelBoundsMin{0.0f};
    Vec3 m_modelBoundsMax{0.0f};

    // Per-step state (populated as wizard progresses)
    carve::FitParams m_fitParams;
    carve::ToolpathConfig m_toolpathConfig;
    carve::StockDimensions m_stock;
    carve::ModelFitter m_fitter;

    // Tool selection state
    VtdbToolGeometry m_finishTool;
    VtdbToolGeometry m_clearTool;
    bool m_finishingToolSelected = false;
    bool m_clearToolSelected = false;

    // Material state
    bool m_materialSelected = false;

    // Preview state
    bool m_toolpathGenerated = false;
    f32 m_previewZoom = 1.0f;
    bool m_showFinishing = true;
    bool m_showClearing = true;
    u32 m_overlayTexture = 0;
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;

    // Outline test state
    bool m_outlineCompleted = false;
    bool m_outlineSkipped = false;
    bool m_outlineRunning = false;
    int m_outlineCmdIndex = 0;

    // Zero confirm state
    bool m_zeroConfirmed = false;

    // Commit state
    bool m_commitConfirmed = false;

    // Running state
    enum class RunState { Active, Paused, Completed, Aborted };
    RunState m_runState = RunState::Active;
    int m_runCurrentLine = 0;
    int m_runTotalLines = 0;
    float m_runElapsedSec = 0.0f;
    std::string m_runCurrentPass;

    // Model/tool info for summary card
    std::string m_modelName;
    std::string m_materialName;

    // G-code export
    void showExportDialog();

    // Long-press abort tracking
    float m_abortHoldTime = 0.0f;
    bool m_abortHolding = false;

    // Helper: format time as "Xm Ys"
    static std::string formatTime(f32 seconds);
};

} // namespace dw
