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

    // Real-time commands (thread-safe, bypass normal queue)
    void feedHold();
    void cycleStart();
    void softReset();
    void setFeedOverride(int percent); // 10-200
    void setRapidOverride(int percent); // 25, 50, or 100
    void setSpindleOverride(int percent); // 10-200
    void jogCancel();
    void unlock(); // $X

    // Status
    const MachineStatus& lastStatus() const { return m_lastStatus; }
    StreamProgress streamProgress() const;

    // Parse a GRBL status report string (public for testing)
    static MachineStatus parseStatusReport(const std::string& report);
    static MachineState parseState(const std::string& stateStr);

  private:
    void ioThreadFunc();
    void processResponse(const std::string& line);
    void sendNextLines();
    void requestStatus();

    MainThreadQueue* m_mtq;
    SerialPort m_port;
    CncCallbacks m_callbacks;

    // IO thread
    std::thread m_ioThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    // Streaming state (protected by m_streamMutex)
    std::mutex m_streamMutex;
    std::vector<std::string> m_program;       // All lines to send
    int m_sendIndex = 0;                       // Next line to send
    int m_ackIndex = 0;                        // Next line awaiting ack
    std::deque<int> m_sentLengths;             // Lengths of in-flight lines (for buffer tracking)
    int m_bufferUsed = 0;                      // Bytes currently in GRBL's RX buffer
    std::atomic<bool> m_streaming{false};
    std::atomic<bool> m_held{false};           // Feed hold active
    int m_errorCount = 0;

    // Status polling
    MachineStatus m_lastStatus;
    std::chrono::steady_clock::time_point m_lastStatusQuery;
    std::chrono::steady_clock::time_point m_streamStartTime;

    static constexpr int STATUS_POLL_MS = 200; // 5 Hz
};

} // namespace dw
