#pragma once

#include <deque>
#include <string>
#include <vector>

#include "../../core/cnc/cnc_types.h"
#include "panel.h"

struct ImGuiInputTextCallbackData; // Forward declaration

namespace dw {

class CncController;

// MDI (Manual Data Input) console panel — send G-code commands, view responses.
class CncConsolePanel : public Panel {
  public:
    CncConsolePanel();
    ~CncConsolePanel() override = default;

    void render() override;

    void setCncController(CncController* cnc) { m_cnc = cnc; }
    void onConnectionChanged(bool connected, const std::string& version);

    // Called for every raw line from GRBL (both sent and received)
    void onRawLine(const std::string& line, bool isSent);
    void onError(const std::string& message);
    void onAlarm(int code, const std::string& desc);

    // Source of a console message
    enum class MessageSource { MDI, JOB, MACRO, SYS };

    // Typed addLine for callers who know the source
    void addLine(const std::string& text, int type, MessageSource source);

    // Console line type for addLine callers
    struct ConsoleLine {
        std::string text;
        enum Type { Sent, Received, Error, Info } type;
        MessageSource source = MessageSource::SYS;
    };

  private:
    static int inputCallback(ImGuiInputTextCallbackData* data);

    CncController* m_cnc = nullptr;
    bool m_connected = false;

    // Input
    char m_inputBuf[256] = "";
    bool m_focusInput = false;

    // Command history
    std::vector<std::string> m_history;
    int m_historyPos = -1;
    static constexpr size_t MAX_HISTORY = 100;
    std::deque<ConsoleLine> m_lines;
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;
    static constexpr size_t MAX_LINES = 500;
};

} // namespace dw
