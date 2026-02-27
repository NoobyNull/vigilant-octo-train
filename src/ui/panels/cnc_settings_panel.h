#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_types.h"
#include "../../core/cnc/grbl_settings.h"
#include "panel.h"

namespace dw {

class CncController;
class FileDialog;

// CNC firmware settings panel -- view, edit, backup/restore GRBL $ settings.
// Receives raw lines via onRawLine callback to capture $$ responses.
// Provides both a grouped settings view and a focused machine tuning tab.
class CncSettingsPanel : public Panel {
  public:
    CncSettingsPanel();
    ~CncSettingsPanel() override = default;

    void render() override;

    // Dependencies
    void setCncController(CncController* cnc) { m_cnc = cnc; }
    void setFileDialog(FileDialog* fd) { m_fileDialog = fd; }

    // Callbacks (called on main thread via MainThreadQueue)
    void onConnectionChanged(bool connected, const std::string& version);
    void onRawLine(const std::string& line, bool isSent);
    void onStatusUpdate(const MachineStatus& status);

    // Access settings for profile sync
    const GrblSettings& settings() const { return m_settings; }
    bool hasSettings() const { return !m_settings.empty(); }

    // EditBuffer needs to be public for helper functions
    struct EditBuffer {
        int id = 0;
        char buf[32] = "";
        bool active = false;
    };

  private:
    // Rendering sub-sections
    void renderToolbar();
    void renderSettingsTab();
    void renderTuningTab();
    void renderSafetyTab();
    void renderRawTab();
    void renderDiffDialog();

    // Actions
    void requestSettings();
    void applySettingToGrbl(int id, float value);
    void writeAllModified();
    void backupToFile();
    void restoreFromFile();

    // State
    CncController* m_cnc = nullptr;
    FileDialog* m_fileDialog = nullptr;
    GrblSettings m_settings;
    MachineState m_machineState = MachineState::Unknown;
    bool m_connected = false;
    bool m_collecting = false;  // True while collecting $$ response lines

    // Editing state
    std::map<int, EditBuffer> m_editBuffers;

    // Diff dialog state (for restore preview)
    bool m_showDiffDialog = false;
    GrblSettings m_restoreSettings;
    std::vector<std::pair<GrblSetting, GrblSetting>> m_diffEntries;

    // Write queue for sequential EEPROM-safe writes
    struct WriteQueueItem {
        int id;
        float value;
    };
    std::vector<WriteQueueItem> m_writeQueue;
    int m_writeIndex = 0;
    bool m_writing = false;
    float m_writeTimer = 0.0f;
    static constexpr float WRITE_DELAY_SEC = 0.05f; // 50ms between EEPROM writes

    // Tab selection
    int m_activeTab = 0; // 0 = Settings, 1 = Tuning
};

} // namespace dw
