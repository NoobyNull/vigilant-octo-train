#pragma once

#include <string>
#include <vector>

#include "../../core/cnc/cnc_types.h"
#include "../../core/cnc/macro_manager.h"
#include "panel.h"

namespace dw {

class CncController;

// CNC Macro panel -- displays user and built-in macros with run, edit, delete,
// and reorder controls. Sends macro G-code lines via CncController::sendCommand().
class CncMacroPanel : public Panel {
  public:
    CncMacroPanel();
    ~CncMacroPanel() override = default;

    void render() override;

    // Dependencies
    void setCncController(CncController* cnc) { m_cnc = cnc; }
    void setMacroManager(MacroManager* mgr) { m_macroManager = mgr; }

    // Callbacks (called on main thread via MainThreadQueue)
    void onConnectionChanged(bool connected, const std::string& version);
    void onStatusUpdate(const MachineStatus& status);

  private:
    void renderMacroList();
    void renderEditArea();
    void executeMacro(const Macro& macro);
    void refreshMacros();

    CncController* m_cnc = nullptr;
    MacroManager* m_macroManager = nullptr;

    // Cached macro list
    std::vector<Macro> m_macros;
    bool m_needsRefresh = true;

    // Connection / state
    bool m_connected = false;
    MachineState m_machineState = MachineState::Unknown;
    bool m_streaming = false;

    // Edit state
    int m_editingId = -1;      // -1 = not editing, 0 = new macro, >0 = existing
    char m_editName[128] = "";
    char m_editGcode[4096] = "";
    char m_editShortcut[64] = "";
    bool m_editIsNew = false;

    // Execution state
    int m_executingId = -1;

    // Drag-and-drop reorder state
    int m_dragSourceIdx = -1;
};

} // namespace dw
