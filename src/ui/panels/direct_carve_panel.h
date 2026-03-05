#pragma once

#include <functional>
#include <string>
#include <vector>

#include "panel.h"

#include "core/carve/model_fitter.h"
#include "core/carve/tool_recommender.h"
#include "core/carve/toolpath_types.h"
#include "core/cnc/cnc_tool.h"
#include "core/cnc/cnc_types.h"
#include "core/materials/material.h"
#include "core/mesh/vertex.h"
#include "core/types.h"

namespace dw {

class CncController;
class FileDialog;
class GCodePanel;
class MaterialManager;
class ProjectManager;
class ToolDatabase;
class ToolboxRepository;

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
    void setToolboxRepository(ToolboxRepository* repo);
    void setCarveJob(carve::CarveJob* job);
    void setFileDialog(FileDialog* dlg);
    void setGCodePanel(GCodePanel* gcp);
    void setMaterialManager(MaterialManager* mgr);
    void setProjectManager(ProjectManager* pm);
    void setOpenToolBrowserCallback(std::function<void()> cb);

    // Callbacks
    void onConnectionChanged(bool connected);
    void onStatusUpdate(const MachineStatus& status);
    void onModelLoaded(const std::vector<Vertex>& vertices,
                       const std::vector<u32>& indices,
                       const Vec3& boundsMin,
                       const Vec3& boundsMax,
                       const std::string& modelName = "",
                       const Path& modelSourcePath = "");

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

    // Tool selection helpers
    void renderToolLibraryPicker();
    void renderManualToolEntry();

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
    ToolboxRepository* m_toolboxRepo = nullptr;
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
    bool m_toolLibraryLoaded = false;
    std::vector<VtdbToolGeometry> m_libraryTools;  // Currently displayed tool list
    std::vector<VtdbToolGeometry> m_toolboxTools;  // My Toolbox subset
    std::vector<VtdbToolGeometry> m_allTools;      // Full library
    bool m_showAllTools = false;                   // false = My Toolbox, true = All Tools
    int m_selectedLibToolIdx = -1;
    bool m_useManualTool = false;
    // Tool recommendation state
    carve::RecommendationResult m_recommendation;
    bool m_recommendationRun = false;
    int m_selectedClearIdx = -1;
    // Manual tool entry fields
    int m_manualToolType = 0;   // 0=BallNose, 1=VBit, 2=EndMill, 3=TaperedBallNose
    f32 m_manualDiameter = 3.175f;  // 1/8" default
    f32 m_manualAngle = 90.0f;
    f32 m_manualTipRadius = 1.5875f;
    int m_manualFlutes = 2;

    // Material state
    MaterialManager* m_materialMgr = nullptr;
    std::vector<MaterialRecord> m_materialList;
    bool m_materialListLoaded = false;
    int m_selectedMaterialIdx = -1;
    bool m_materialSelected = false;

    // Heightmap cache state
    bool m_hmInitAttempted = false;   // Prevents re-triggering auto-load every frame
    bool m_hmFileMissing = false;     // Shows locate/regen/cancel UI
    bool m_heightmapSaved = false;    // Tracks whether auto-save fired
    bool m_hmRegenConfirm = false;    // Regenerate confirmation popup open
    std::string m_hmMissingPath;      // Path of the missing .dwhm file

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

    // Project directory support
    ProjectManager* m_projectManager = nullptr;
    std::function<void()> m_openToolBrowser;
    Path m_modelSourcePath;

    // Project-aware save helpers
    void saveHeightmapToProject();
    void saveImageToProject();
    void saveGCodeToProject();

    // G-code export (fallback FileDialog)
    void showExportDialog();

    // Long-press abort tracking
    float m_abortHoldTime = 0.0f;
    bool m_abortHolding = false;

    // Helper: format time as "Xm Ys"
    static std::string formatTime(f32 seconds);
};

} // namespace dw
