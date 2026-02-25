#include "cnc_controller.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "../threading/main_thread_queue.h"
#include "../utils/log.h"

namespace dw {

CncController::CncController(MainThreadQueue* mtq) : m_mtq(mtq) {}

CncController::~CncController() {
    disconnect();
}

bool CncController::connect(const std::string& device, int baudRate) {
    disconnect();

    if (!m_port.open(device, baudRate))
        return false;

    // Soft-reset to get a clean state
    m_port.writeByte(cnc::CMD_SOFT_RESET);
    m_port.drain();

    m_running = true;
    m_connected = false;
    m_consecutiveTimeouts = 0;
    m_statusPending = false;
    m_pendingRtCommands.store(0, std::memory_order_relaxed);
    m_ioThread = std::thread(&CncController::ioThreadFunc, this);

    return true;
}

void CncController::disconnect() {
    m_running = false;
    m_streaming = false;
    m_errorState = false;

    if (m_ioThread.joinable())
        m_ioThread.join();

    // Clear any pending commands
    m_pendingRtCommands.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(m_overrideMutex);
        m_pendingOverrides.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_cmdStringMutex);
        m_pendingStringCmds.clear();
    }
    m_consecutiveTimeouts = 0;
    m_statusPending = false;

    if (m_port.isOpen()) {
        m_port.close();
        m_connected = false;

        if (m_mtq && m_callbacks.onConnectionChanged)
            m_mtq->enqueue([cb = m_callbacks.onConnectionChanged]() { cb(false, ""); });
    }
}

void CncController::startStream(const std::vector<std::string>& lines) {
    if (m_errorState) {
        log::error("CNC", "Cannot start stream while in error state -- call acknowledgeError() first");
        if (m_mtq && m_callbacks.onError) {
            m_mtq->enqueue([cb = m_callbacks.onError]() {
                cb("Cannot start new job: previous streaming error must be acknowledged first");
            });
        }
        return;
    }
    std::lock_guard<std::mutex> lock(m_streamMutex);
    m_program = lines;
    m_sendIndex = 0;
    m_ackIndex = 0;
    m_sentLengths.clear();
    m_bufferUsed = 0;
    m_errorCount = 0;
    m_held = false;
    m_streamStartTime = std::chrono::steady_clock::now();
    m_streaming = true;
}

void CncController::acknowledgeError() {
    m_errorState = false;
    log::info("CNC", "Streaming error acknowledged by operator");
}

void CncController::stopStream() {
    m_streaming = false;
    // Send feed hold + soft reset to stop the machine
    feedHold();
}

// --- Real-time commands (UI thread safe — all routed through IO thread) ---

void CncController::feedHold() {
    m_pendingRtCommands.fetch_or(RT_FEED_HOLD, std::memory_order_release);
    m_held = true;
}

void CncController::cycleStart() {
    m_pendingRtCommands.fetch_or(RT_CYCLE_START, std::memory_order_release);
    m_held = false;
}

void CncController::softReset() {
    m_pendingRtCommands.fetch_or(RT_SOFT_RESET, std::memory_order_release);
    m_streaming = false;
    m_held = false;
    m_errorState = false; // Explicit reset clears error state
    {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        m_sentLengths.clear();
        m_bufferUsed = 0;
    }
    // drain() will be called by the IO thread after dispatching the reset
}

void CncController::setFeedOverride(int percent) {
    OverrideCmd cmd;
    cmd.bytes.push_back(cnc::CMD_FEED_100_PERCENT);
    int diff = percent - 100;
    while (diff >= 10)  { cmd.bytes.push_back(cnc::CMD_FEED_PLUS_10);  diff -= 10; }
    while (diff <= -10) { cmd.bytes.push_back(cnc::CMD_FEED_MINUS_10); diff += 10; }
    while (diff > 0)    { cmd.bytes.push_back(cnc::CMD_FEED_PLUS_1);   diff--; }
    while (diff < 0)    { cmd.bytes.push_back(cnc::CMD_FEED_MINUS_1);  diff++; }
    {
        std::lock_guard<std::mutex> lock(m_overrideMutex);
        m_pendingOverrides.push_back(std::move(cmd));
    }
}

void CncController::setRapidOverride(int percent) {
    OverrideCmd cmd;
    if (percent <= 25)
        cmd.bytes.push_back(cnc::CMD_RAPID_25_PERCENT);
    else if (percent <= 50)
        cmd.bytes.push_back(cnc::CMD_RAPID_50_PERCENT);
    else
        cmd.bytes.push_back(cnc::CMD_RAPID_100_PERCENT);
    {
        std::lock_guard<std::mutex> lock(m_overrideMutex);
        m_pendingOverrides.push_back(std::move(cmd));
    }
}

void CncController::setSpindleOverride(int percent) {
    OverrideCmd cmd;
    cmd.bytes.push_back(cnc::CMD_SPINDLE_100_PERCENT);
    int diff = percent - 100;
    while (diff >= 10)  { cmd.bytes.push_back(cnc::CMD_SPINDLE_PLUS_10);  diff -= 10; }
    while (diff <= -10) { cmd.bytes.push_back(cnc::CMD_SPINDLE_MINUS_10); diff += 10; }
    while (diff > 0)    { cmd.bytes.push_back(cnc::CMD_SPINDLE_PLUS_1);   diff--; }
    while (diff < 0)    { cmd.bytes.push_back(cnc::CMD_SPINDLE_MINUS_1);  diff++; }
    {
        std::lock_guard<std::mutex> lock(m_overrideMutex);
        m_pendingOverrides.push_back(std::move(cmd));
    }
}

void CncController::jogCancel() {
    m_pendingRtCommands.fetch_or(RT_JOG_CANCEL, std::memory_order_release);
}

void CncController::unlock() {
    std::lock_guard<std::mutex> lock(m_cmdStringMutex);
    m_pendingStringCmds.push_back("$X\n");
}

void CncController::sendCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(m_cmdStringMutex);
    m_pendingStringCmds.push_back(cmd + "\n");
}

StreamProgress CncController::streamProgress() const {
    StreamProgress p;
    // Read atomic/shared state — approximate is fine for UI display
    p.totalLines = static_cast<int>(m_program.size());
    p.ackedLines = m_ackIndex;
    p.errorCount = m_errorCount;

    auto now = std::chrono::steady_clock::now();
    p.elapsedSeconds = std::chrono::duration<f32>(now - m_streamStartTime).count();
    return p;
}

// --- IO Thread ---

void CncController::ioThreadFunc() {
    log::info("CNC", "IO thread started");

    // Wait for controller banner or probe with status query.
    // Classic Grbl sends a banner on reset; FluidNC may not.
    std::string version;
    for (int i = 0; i < 50; ++i) { // 5 seconds max
        auto line = m_port.readLine(100);
        if (!line)
            continue;
        // Classic Grbl: "Grbl 1.1h ['$' for help]"
        // FluidNC:      "[MSG:INFO: FluidNC v3.7.x ...]"
        // grblHAL:      "GrblHAL 1.1f ..."
        if (line->find("Grbl") != std::string::npos ||
            line->find("grbl") != std::string::npos ||
            line->find("FluidNC") != std::string::npos) {
            version = *line;
            break;
        }
    }

    // If no banner, probe with '?' status query — FluidNC responds without a banner
    if (version.empty()) {
        m_port.write("?\n");
        for (int i = 0; i < 20; ++i) { // 2 seconds
            auto line = m_port.readLine(100);
            if (line && line->size() > 1 && (*line)[0] == '<') {
                // Got a valid Grbl status response like "<Idle|MPos:...>"
                version = "FluidNC (compatible)";
                break;
            }
        }
    }

    if (version.empty()) {
        log::error("CNC", "No compatible controller detected");
        m_running = false;
        if (m_mtq && m_callbacks.onConnectionChanged)
            m_mtq->enqueue([cb = m_callbacks.onConnectionChanged]() { cb(false, ""); });
        return;
    }

    m_connected = true;
    log::infof("CNC", "Connected: %s", version.c_str());
    if (m_mtq && m_callbacks.onConnectionChanged) {
        m_mtq->enqueue(
            [cb = m_callbacks.onConnectionChanged, ver = version]() { cb(true, ver); });
    }

    m_lastStatusQuery = std::chrono::steady_clock::now();

    while (m_running) {
        // Dispatch any pending commands from UI thread (feed hold, cycle start, etc.)
        dispatchPendingCommands();

        // Read responses
        auto line = m_port.readLine(20);

        // Check SerialPort connection state for hardware-level disconnect
        if (m_port.connectionState() == ConnectionState::Disconnected ||
            m_port.connectionState() == ConnectionState::Error) {
            log::error("CNC", "Serial port reports disconnected");
            handleDisconnect();
            break;
        }

        if (line) {
            m_consecutiveTimeouts = 0;
            if (m_mtq && m_callbacks.onRawLine) {
                m_mtq->enqueue(
                    [cb = m_callbacks.onRawLine, l = *line]() { cb(l, false); });
            }
            processResponse(*line);
        } else {
            // No data -- check for consecutive timeout disconnect detection
            if (m_statusPending) {
                m_consecutiveTimeouts++;
                if (m_consecutiveTimeouts >= MAX_CONSECUTIVE_TIMEOUTS) {
                    log::error("CNC", "No response to status queries -- connection lost");
                    handleDisconnect();
                    break;
                }
            }
        }

        // Status polling at 5Hz
        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastStatusQuery).count();
        if (elapsed >= STATUS_POLL_MS) {
            requestStatus();
            m_lastStatusQuery = now;
        }

        // Character-counting: send more lines if buffer has room
        if (m_streaming && !m_held) {
            sendNextLines();
        }
    }

    m_connected = false;
    log::info("CNC", "IO thread stopped");
}

void CncController::processResponse(const std::string& line) {
    if (line.empty())
        return;

    // Status report: <Idle|MPos:0.000,0.000,0.000|...>
    if (line.front() == '<' && line.back() == '>') {
        m_lastStatus = parseStatusReport(line);
        m_statusPending = false;
        m_consecutiveTimeouts = 0;
        if (m_mtq && m_callbacks.onStatusUpdate) {
            m_mtq->enqueue(
                [cb = m_callbacks.onStatusUpdate, st = m_lastStatus]() { cb(st); });
        }
        return;
    }

    // Alarm
    if (line.find("ALARM:") == 0) {
        int code = 0;
        try {
            code = std::stoi(line.substr(6));
        } catch (...) {}
        std::string desc = alarmDescription(code);
        if (m_mtq && m_callbacks.onAlarm) {
            m_mtq->enqueue([cb = m_callbacks.onAlarm, code, desc]() { cb(code, desc); });
        }
        // Stop streaming on alarm
        m_streaming = false;
        return;
    }

    // "ok" or "error:N" — ack for a sent line
    if (line == "ok" || line.find("error:") == 0) {
        std::lock_guard<std::mutex> lock(m_streamMutex);

        if (!m_sentLengths.empty()) {
            m_bufferUsed -= m_sentLengths.front();
            m_sentLengths.pop_front();
        }

        LineAck ack;
        ack.lineIndex = m_ackIndex;
        ack.ok = (line == "ok");

        if (!ack.ok) {
            try {
                ack.errorCode = std::stoi(line.substr(6));
            } catch (...) {}
            ack.errorMessage = errorDescription(ack.errorCode);
            m_errorCount++;

            if (m_streaming) {
                // CRITICAL SAFETY: Issue soft reset to flush GRBL's RX buffer.
                // Without this, buffered commands continue executing in potentially
                // incorrect machine state after the error.
                m_pendingRtCommands.fetch_or(RT_SOFT_RESET, std::memory_order_release);

                // Capture error details before clearing state
                StreamingError streamErr;
                streamErr.lineIndex = ack.lineIndex;
                streamErr.errorCode = ack.errorCode;
                streamErr.errorMessage = ack.errorMessage;
                if (ack.lineIndex >= 0 && ack.lineIndex < static_cast<int>(m_program.size()))
                    streamErr.failedLine = m_program[ack.lineIndex];
                streamErr.linesInFlight = static_cast<int>(m_sentLengths.size());

                // Stop streaming and clear buffer accounting
                m_streaming = false;
                m_held = false;
                m_sentLengths.clear();
                m_bufferUsed = 0;

                // Enter error state -- requires acknowledgment before new operations
                m_errorState = true;

                // Notify UI with detailed error report
                if (m_mtq && m_callbacks.onStreamingError) {
                    m_mtq->enqueue([cb = m_callbacks.onStreamingError, streamErr]() {
                        cb(streamErr);
                    });
                }

                // Also fire the line ack callback so UI can track the specific line
                if (m_mtq && m_callbacks.onLineAcked) {
                    m_mtq->enqueue([cb = m_callbacks.onLineAcked, ack]() { cb(ack); });
                }
                return; // Don't process further -- stream is terminated
            }
        }

        m_ackIndex++;

        if (m_mtq && m_callbacks.onLineAcked) {
            m_mtq->enqueue([cb = m_callbacks.onLineAcked, ack]() { cb(ack); });
        }

        // Check if stream complete
        if (m_streaming && m_ackIndex >= static_cast<int>(m_program.size())) {
            m_streaming = false;
        }

        // Post progress update
        if (m_mtq && m_callbacks.onProgressUpdate) {
            auto prog = streamProgress();
            m_mtq->enqueue([cb = m_callbacks.onProgressUpdate, prog]() { cb(prog); });
        }
        return;
    }

    // GRBL messages like [MSG:...], [GC:...], etc.
    if (line.front() == '[') {
        if (m_mtq && m_callbacks.onError && line.find("[MSG:") == 0) {
            std::string msg = line.substr(5);
            if (!msg.empty() && msg.back() == ']')
                msg.pop_back();
            m_mtq->enqueue([cb = m_callbacks.onError, msg]() { cb(msg); });
        }
    }
}

void CncController::sendNextLines() {
    std::lock_guard<std::mutex> lock(m_streamMutex);

    while (m_sendIndex < static_cast<int>(m_program.size())) {
        const std::string& line = m_program[m_sendIndex];

        // Character counting: each line uses (length + 1) bytes in GRBL buffer (for the \n)
        int lineLen = static_cast<int>(line.size()) + 1;

        if (m_bufferUsed + lineLen > cnc::RX_BUFFER_SIZE)
            break; // Buffer full, wait for acks

        std::string toSend = line + "\n";
        if (!m_port.write(toSend))
            break;

        if (m_mtq && m_callbacks.onRawLine) {
            m_mtq->enqueue(
                [cb = m_callbacks.onRawLine, l = line]() { cb(l, true); });
        }

        m_sentLengths.push_back(lineLen);
        m_bufferUsed += lineLen;
        m_sendIndex++;
    }
}

void CncController::requestStatus() {
    m_port.writeByte(cnc::CMD_STATUS_QUERY);
    m_statusPending = true;
}

void CncController::dispatchPendingCommands() {
    // 1. Dispatch single-byte real-time commands (atomic, no lock needed)
    uint32_t pending = m_pendingRtCommands.exchange(0, std::memory_order_acquire);

    // Soft reset has highest priority — after reset, don't send anything else
    if (pending & RT_SOFT_RESET) {
        m_port.writeByte(cnc::CMD_SOFT_RESET);
        m_port.drain();
        return;
    }
    if (pending & RT_FEED_HOLD)   m_port.writeByte(cnc::CMD_FEED_HOLD);
    if (pending & RT_CYCLE_START) m_port.writeByte(cnc::CMD_CYCLE_START);
    if (pending & RT_JOG_CANCEL)  m_port.writeByte(0x85);

    // 2. Dispatch override byte sequences (atomically per override)
    {
        std::lock_guard<std::mutex> lock(m_overrideMutex);
        for (const auto& cmd : m_pendingOverrides) {
            for (u8 b : cmd.bytes)
                m_port.writeByte(b);
        }
        m_pendingOverrides.clear();
    }

    // 3. Dispatch string commands (e.g., $X unlock)
    {
        std::lock_guard<std::mutex> lock(m_cmdStringMutex);
        for (const auto& s : m_pendingStringCmds)
            m_port.write(s);
        m_pendingStringCmds.clear();
    }
}

void CncController::handleDisconnect() {
    m_connected = false;
    bool wasStreaming = m_streaming.exchange(false);
    m_held = false;

    // Clear streaming state
    {
        std::lock_guard<std::mutex> lock(m_streamMutex);
        m_sentLengths.clear();
        m_bufferUsed = 0;
    }

    // Notify UI of disconnect
    if (m_mtq && m_callbacks.onConnectionChanged)
        m_mtq->enqueue([cb = m_callbacks.onConnectionChanged]() {
            cb(false, "");
        });

    if (wasStreaming && m_mtq && m_callbacks.onError)
        m_mtq->enqueue([cb = m_callbacks.onError]() {
            cb("Connection lost during streaming -- job aborted. Manual reconnect required.");
        });
}

// --- Static parsers ---

MachineState CncController::parseState(const std::string& stateStr) {
    if (stateStr == "Idle") return MachineState::Idle;
    if (stateStr == "Run") return MachineState::Run;
    if (stateStr == "Hold" || stateStr.find("Hold:") == 0) return MachineState::Hold;
    if (stateStr == "Jog") return MachineState::Jog;
    if (stateStr == "Alarm") return MachineState::Alarm;
    if (stateStr == "Door" || stateStr.find("Door:") == 0) return MachineState::Door;
    if (stateStr == "Check") return MachineState::Check;
    if (stateStr == "Home") return MachineState::Home;
    if (stateStr == "Sleep") return MachineState::Sleep;
    return MachineState::Unknown;
}

MachineStatus CncController::parseStatusReport(const std::string& report) {
    MachineStatus status;

    // Format: <State|MPos:x,y,z|WPos:x,y,z|FS:feed,speed|Ov:f,r,s|Pn:XYZ>
    if (report.size() < 3)
        return status;

    // Strip < and >
    std::string inner = report.substr(1, report.size() - 2);

    // Split by |
    std::vector<std::string> fields;
    std::istringstream stream(inner);
    std::string field;
    while (std::getline(stream, field, '|'))
        fields.push_back(field);

    if (fields.empty())
        return status;

    // First field is state
    status.state = parseState(fields[0]);

    // Parse remaining fields
    for (size_t i = 1; i < fields.size(); ++i) {
        const auto& f = fields[i];
        auto colonPos = f.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string key = f.substr(0, colonPos);
        std::string val = f.substr(colonPos + 1);

        auto parseVec3 = [](const std::string& s) -> Vec3 {
            Vec3 v{0.0f};
            if (sscanf(s.c_str(), "%f,%f,%f", &v.x, &v.y, &v.z) != 3) {
                // Try 2-axis
                sscanf(s.c_str(), "%f,%f", &v.x, &v.y);
            }
            return v;
        };

        if (key == "MPos") {
            status.machinePos = parseVec3(val);
        } else if (key == "WPos") {
            status.workPos = parseVec3(val);
        } else if (key == "WCO") {
            // Work coordinate offset — derive WPos from MPos - WCO
            Vec3 wco = parseVec3(val);
            status.workPos = status.machinePos - wco;
        } else if (key == "FS" || key == "F") {
            if (key == "FS") {
                sscanf(val.c_str(), "%f,%f", &status.feedRate, &status.spindleSpeed);
            } else {
                sscanf(val.c_str(), "%f", &status.feedRate);
            }
        } else if (key == "Ov") {
            sscanf(val.c_str(), "%d,%d,%d", &status.feedOverride, &status.rapidOverride,
                   &status.spindleOverride);
        } else if (key == "Pn") {
            status.inputPins = 0; // Clear all pins first
            for (char c : val) {
                switch (c) {
                case 'X': status.inputPins |= cnc::PIN_X_LIMIT; break;
                case 'Y': status.inputPins |= cnc::PIN_Y_LIMIT; break;
                case 'Z': status.inputPins |= cnc::PIN_Z_LIMIT; break;
                case 'P': status.inputPins |= cnc::PIN_PROBE; break;
                case 'D': status.inputPins |= cnc::PIN_DOOR; break;
                case 'H': status.inputPins |= cnc::PIN_HOLD; break;
                case 'R': status.inputPins |= cnc::PIN_RESET; break;
                case 'S': status.inputPins |= cnc::PIN_START; break;
                }
            }
        }
    }

    return status;
}

} // namespace dw
