#pragma once

#include <string>
#include <vector>

#include "byte_stream.h"

namespace dw {

// Lightweight POSIX serial port wrapper for GRBL communication
class SerialPort : public IByteStream {
  public:
    SerialPort() = default;
    ~SerialPort() override;

    // Non-copyable, movable
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&& other) noexcept;
    SerialPort& operator=(SerialPort&& other) noexcept;

    // Open a serial port (e.g. "/dev/ttyUSB0") at the given baud rate, 8N1
    // (Transport-specific — not part of IByteStream)
    bool open(const std::string& device, int baudRate);

    // IByteStream interface
    void close() override;
    bool isOpen() const override { return m_fd >= 0; }
    bool write(const std::string& data) override;
    bool writeByte(u8 byte) override;
    std::optional<std::string> readLine(int timeoutMs) override;
    void drain() override;
    const std::string& device() const override { return m_device; }
    ConnectionState connectionState() const override { return m_connectionState; }

  private:
    int m_fd = -1;
    std::string m_device;
    std::string m_readBuffer; // Accumulates partial reads between readLine calls
    ConnectionState m_connectionState = ConnectionState::Closed;
};

// Scan for available serial ports (/dev/ttyUSB*, /dev/ttyACM*)
std::vector<std::string> listSerialPorts();

} // namespace dw
