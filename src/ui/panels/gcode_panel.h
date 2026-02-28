#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <glad/gl.h>

#include "../../core/gcode/gcode_analyzer.h"
#include "../../core/gcode/gcode_parser.h"
#include "../../core/gcode/machine_profile.h"
#include "../../core/cnc/cnc_types.h"
#include "../../core/database/job_repository.h"
#include "../../render/camera.h"
#include "../../render/framebuffer.h"
#include "../../render/renderer.h"
#include "../dialogs/machine_profile_dialog.h"
#include "panel.h"

namespace dw {

class FileDialog;
class GCodeRepository;
class ProjectManager;
class CncController;
class ToolDatabase;

// Panel mode — View (with optional simulation overlay), Send (real CNC)
enum class GCodePanelMode { View, Send };

// Simulation playback state
enum class SimState { Stopped, Playing, Paused };

// Console line entry
struct ConsoleLine {
    std::string text;
    enum Type { Sent, Ok, Error, Status, Info } type = Info;
};

// G-code viewer / simulator / sender panel
class GCodePanel : public Panel {
  public:
    GCodePanel();
    ~GCodePanel() override;

    void render() override;

    // Dependency injection
    void setFileDialog(FileDialog* dialog) { m_fileDialog = dialog; }
    void setGCodeRepository(GCodeRepository* repo) { m_gcodeRepo = repo; }
    void setProjectManager(ProjectManager* pm) { m_projectManager = pm; }
    void setCncController(CncController* ctrl) { m_cnc = ctrl; }
    void setJobRepository(JobRepository* repo) { m_jobRepo = repo; }
    void setToolDatabase(ToolDatabase* db) { m_toolDatabase = db; }

    // Load G-code from file
    bool loadFile(const std::string& path);
    void clear();
    bool hasGCode() const { return m_program.commands.size() > 0; }

    // Get raw G-code lines for resume-from-line feature
    std::vector<std::string> getRawLines() const;

    // Get toolpath bounds (from analyzed statistics)
    Vec3 boundsMin() const { return m_stats.boundsMin; }
    Vec3 boundsMax() const { return m_stats.boundsMax; }

    // Called by application_wiring to push GRBL events into the panel
    void onGrblConnected(bool connected, const std::string& version);
    void onGrblStatus(const MachineStatus& status);
    void onGrblLineAcked(const LineAck& ack);
    void onGrblProgress(const StreamProgress& progress);
    void onGrblAlarm(int code, const std::string& desc);
    void onGrblError(const std::string& message);
    void onGrblRawLine(const std::string& line, bool isSent);

    // Update simulation (call each frame with delta time)
    void updateSimulation(float dt);

  private:
    // Render sections
    void renderToolbar();
    void renderRecentFiles();
    void renderModeTabs();
    void renderMachineProfileSelector();
    void renderStatistics();
    void renderPathView();
    void renderZClipSlider();
    void renderConnectionBar();
    void renderMachineStatus();
    void renderPlaybackControls();
    void renderProgressBar();
    void renderFeedOverride();
    void renderConsole();
    void renderSimulationControls();
    void renderJobHistory();

    // 3D path geometry
    void buildPathGeometry();
    void destroyPathGeometry();
    void handlePathInput();

    // Re-run analyzer with current profile and rebuild sim times
    void reanalyze();

    // Helpers
    void addConsoleLine(const std::string& text, ConsoleLine::Type type);
    void buildSendProgram();

    FileDialog* m_fileDialog = nullptr;

    gcode::Program m_program;
    gcode::Statistics m_stats;
    std::string m_filePath;

    // View settings
    float m_currentLayer = 0.0f;
    float m_maxLayer = 100.0f;
    bool m_showRapid = false;     // G0 rapid moves
    bool m_showCut = true;        // G1 mostly-XY cutting moves
    bool m_showPlunge = true;     // G1 Z-descending moves
    bool m_showRetract = true;    // G1 Z-ascending moves
    bool m_colorByTool = false;   // Color segments by T-code tool number

    // Tool color palette (8 colors, wraps via modulo)
    static constexpr int NUM_TOOL_COLORS = 8;
    static Vec3 toolColor(int toolNum) {
        static const Vec3 palette[] = {
            {0.2f, 0.6f, 1.0f},  // T1: Blue
            {1.0f, 0.3f, 0.3f},  // T2: Red
            {0.3f, 0.9f, 0.3f},  // T3: Green
            {1.0f, 0.7f, 0.1f},  // T4: Orange
            {0.8f, 0.3f, 0.9f},  // T5: Purple
            {0.1f, 0.9f, 0.9f},  // T6: Cyan
            {0.9f, 0.9f, 0.2f},  // T7: Yellow
            {1.0f, 0.5f, 0.7f},  // T8: Pink
        };
        int idx = (toolNum > 0 ? toolNum - 1 : 0) % NUM_TOOL_COLORS;
        return palette[idx];
    }

    // 3D rendering pipeline
    Renderer m_pathRenderer;
    Camera m_pathCamera;
    Framebuffer m_pathFramebuffer;
    int m_pathViewWidth = 1;
    int m_pathViewHeight = 1;
    bool m_pathDirty = true;
    bool m_needsCameraFit = true;

    // Path geometry — single VBO with grouped segments
    GLuint m_pathVAO = 0;
    GLuint m_pathVBO = 0;
    u32 m_rapidStart = 0, m_rapidCount = 0;
    u32 m_cutStart = 0, m_cutCount = 0;
    u32 m_plungeStart = 0, m_plungeCount = 0;
    u32 m_retractStart = 0, m_retractCount = 0;

    // Simulation overlay — dynamic VBO for progress visualization
    GLuint m_simVAO = 0;
    GLuint m_simVBO = 0;

    // Per-tool draw groups (used when m_colorByTool is true)
    struct ToolGroup {
        u32 start = 0;
        u32 count = 0;
        int toolNumber = 0;
    };
    std::vector<ToolGroup> m_toolGroups;

    // Previous filter state to detect changes
    bool m_prevShowRapid = false;
    bool m_prevShowCut = true;
    bool m_prevShowPlunge = true;
    bool m_prevShowRetract = true;
    float m_prevLayer = 0.0f;

    // Persistence
    GCodeRepository* m_gcodeRepo = nullptr;
    JobRepository* m_jobRepo = nullptr;
    ProjectManager* m_projectManager = nullptr;
    i64 m_currentGCodeId = -1;

    // Job history
    i64 m_activeJobId = -1;
    int m_lastProgressSave = 0; // Last acked line when progress was saved
    bool m_showJobHistory = false;
    std::vector<JobRecord> m_jobHistoryCache;
    bool m_jobHistoryDirty = true;

    // Mode
    GCodePanelMode m_mode = GCodePanelMode::View;

    // Tool database
    ToolDatabase* m_toolDatabase = nullptr;

    // --- Sender state ---
    CncController* m_cnc = nullptr;
    bool m_cncConnected = false;
    std::string m_cncVersion;
    MachineStatus m_machineStatus;
    StreamProgress m_streamProgress;
    int m_feedOverridePercent = 100;

    // Serial port selection
    std::vector<std::string> m_availablePorts;
    size_t m_selectedPort = 0;
    size_t m_selectedBaud = 3; // Index into baud rate list (115200)
    static constexpr int BAUD_RATES[] = {9600, 19200, 38400, 57600, 115200, 230400};
    static constexpr int NUM_BAUD_RATES = 6;

    // Console
    std::deque<ConsoleLine> m_consoleLines;
    bool m_consoleAutoScroll = true;
    bool m_consoleOpen = false;
    static constexpr size_t MAX_CONSOLE_LINES = 500;

    // Progress highlighting — which line index was last acked
    int m_lastAckedLine = -1;

    // --- Simulation state ---
    SimState m_simState = SimState::Stopped;
    float m_simTime = 0.0f;          // Accumulated simulation time (seconds)
    float m_simSpeed = 1.0f;         // Playback speed multiplier
    float m_simTotalTime = 0.0f;     // Total program time (seconds)
    size_t m_simSegmentIndex = 0;    // Current segment being animated
    float m_simSegmentProgress = 0.0f; // 0..1 progress within current segment

    // Precomputed segment times from trapezoidal planner
    std::vector<float> m_segmentTimes;          // Per-segment time (seconds)
    std::vector<float> m_segmentTimeCumulative; // Cumulative sum for binary search

    // Job completion flash timer
    float m_jobFlashTimer = 0.0f;

    // Machine profile editor dialog
    MachineProfileDialog m_profileDialog;

    // G-code search/goto (EXT-12)
    char m_searchBuf[128] = "";
    int m_searchResultLine = -1;
    bool m_searchActive = false;
    int m_gotoLine = 0;
    int m_scrollToLine = -1;

    void renderSearchBar();
    void findNext();
    void gotoLineNumber(int lineNum);
};

} // namespace dw
