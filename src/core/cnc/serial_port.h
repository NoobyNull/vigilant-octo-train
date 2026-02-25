#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"

namespace dw {

// Serial port connection state
enum class ConnectionState {
    Closed,        // Port not open
    Connected,     // Port open and healthy
    Disconnected,  // Device removed (POLLHUP/POLLERR or consecutive timeouts)
    Error          // Unrecoverable error
};

// Lightweight POSIX serial port wrapper for GRBL communication
class SerialPort {
  public:
    SerialPort() = default;
    ~SerialPort();

    // Non-copyable, movable
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&& other) noexcept;
    SerialPort& operator=(SerialPort&& other) noexcept;

    // Open a serial port (e.g. "/dev/ttyUSB0") at the given baud rate, 8N1
    bool open(const std::string& device, int baudRate);

    // Close the port (safe to call if already closed)
    void close();

    // Check if the port is currently open
    bool isOpen() const { return m_fd >= 0; }

    // Write a string (appends newline for gcode lines)
    bool write(const std::string& data);

    // Write a single byte (for GRBL real-time commands — no newline)
    bool writeByte(u8 byte);

    // Read a line (up to '\n') with timeout in milliseconds. Returns nullopt on timeout/error.
    std::optional<std::string> readLine(int timeoutMs);

    // Flush input/output buffers (useful after soft-reset)
    void drain();

    // Get the device path
    const std::string& device() const { return m_device; }

    // Get connection state (thread-safe read)
    ConnectionState connectionState() const { return m_connectionState; }

  private:
    int m_fd = -1;
    std::string m_device;
    std::string m_readBuffer; // Accumulates partial reads between readLine calls
    ConnectionState m_connectionState = ConnectionState::Closed;
};

// Scan for available serial ports (/dev/ttyUSB*, /dev/ttyACM*)
std::vector<std::string> listSerialPorts();

} // namespace dw
