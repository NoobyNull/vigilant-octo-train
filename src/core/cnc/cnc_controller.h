#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "serial_port.h"
#include "cnc_types.h"

namespace dw {

class MainThreadQueue;

// GRBL CNC controller — manages serial communication, character-counting streaming,
// status polling, and real-time command injection on a dedicated IO thread.
class CncController {
  public:
    explicit CncController(MainThreadQueue* mtq);
    ~CncController();

    // Non-copyable, non-movable
    CncController(const CncController&) = delete;
    CncController& operator=(const CncController&) = delete;

    // Connection
    bool connect(const std::string& device, int baudRate = 115200);
    void disconnect();
    bool isConnected() const { return m_connected.load(); }

    // Set event callbacks (call before connect)
    void setCallbacks(const CncCallbacks& cb) { m_callbacks = cb; }

    // Streaming
    void startStream(const std::vector<std::string>& lines);
    void stopStream();
    bool isStreaming() const { return m_streaming.load(); }

    // Real-time commands (thread-safe — all routed through IO thread)
    void feedHold();
    void cycleStart();
    void softReset();
    void setFeedOverride(int percent); // 10-200
    void setRapidOverride(int percent); // 25, 50, or 100
    void setSpindleOverride(int percent); // 10-200
    void jogCancel();
    void unlock(); // $X
    void sendCommand(const std::string& cmd); // Queue arbitrary G-code/system command

    // Configurable status polling interval (50-200ms)
    void setStatusPollMs(int ms) { m_statusPollMs = ms; }

    // Status
    const MachineStatus& lastStatus() const { return m_lastStatus; }
    StreamProgress streamProgress() const;

    // Error state — set after streaming error, requires acknowledgment
    bool isInErrorState() const { return m_errorState.load(); }
    void acknowledgeError();

    // Tool change — M6 detected during streaming, pauses for operator acknowledgment
    bool isToolChangePending() const { return m_toolChangePending.load(); }
    void acknowledgeToolChange();

    // Parse a GRBL status report string (public for testing)
    static MachineStatus parseStatusReport(const std::string& report);
    static MachineState parseState(const std::string& stateStr);

  private:
    void ioThreadFunc();
    void processResponse(const std::string& line);
    void sendNextLines();
    void requestStatus();
    void dispatchPendingCommands();
    void handleDisconnect();

    MainThreadQueue* m_mtq;
    SerialPort m_port;
    CncCallbacks m_callbacks;

    // IO thread
    std::thread m_ioThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    // Thread-safe command dispatch: UI thread -> IO thread
    // Real-time single-byte commands use atomic bitmask (lock-free)
    enum RtCommand : uint32_t {
        RT_FEED_HOLD    = 1 << 0,
        RT_CYCLE_START  = 1 << 1,
        RT_SOFT_RESET   = 1 << 2,
        RT_JOG_CANCEL   = 1 << 3,
    };
    std::atomic<uint32_t> m_pendingRtCommands{0};

    // Override commands need multiple bytes, use mutex-protected queue
    struct OverrideCmd {
        std::vector<u8> bytes;
    };
    std::mutex m_overrideMutex;
    std::vector<OverrideCmd> m_pendingOverrides;

    // String commands (e.g., $X unlock) also queued
    std::mutex m_cmdStringMutex;
    std::vector<std::string> m_pendingStringCmds;

    // Streaming state (protected by m_streamMutex)
    std::mutex m_streamMutex;
    std::vector<std::string> m_program;       // All lines to send
    int m_sendIndex = 0;                       // Next line to send
    int m_ackIndex = 0;                        // Next line awaiting ack
    std::deque<int> m_sentLengths;             // Lengths of in-flight lines (for buffer tracking)
    int m_bufferUsed = 0;                      // Bytes currently in GRBL's RX buffer
    std::atomic<bool> m_streaming{false};
    std::atomic<bool> m_held{false};           // Feed hold active
    std::atomic<bool> m_errorState{false};     // Streaming error requires acknowledgment
    std::atomic<bool> m_toolChangePending{false}; // M6 pause waiting for operator ack
    int m_errorCount = 0;

    // Status polling and disconnect detection
    MachineStatus m_lastStatus;
    std::chrono::steady_clock::time_point m_lastStatusQuery;
    std::chrono::steady_clock::time_point m_streamStartTime;
    int m_consecutiveTimeouts = 0;
    bool m_statusPending = false;

    int m_statusPollMs = 200; // Default 5 Hz, configurable via Config
    static constexpr int MAX_CONSECUTIVE_TIMEOUTS = 10; // ~2 seconds of no response
};

} // namespace dw
